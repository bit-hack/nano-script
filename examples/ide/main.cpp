#include <stdio.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl2.h"
#include "imgui/imgui_impl_sdl.h"

#include "imgui/TextEditor.h"

#include "../lib_compiler/nano.h"
#include "../lib_compiler/errors.h"
#include "../lib_compiler/codegen.h"
#include "../lib_compiler/parser.h"
#include "../lib_compiler/disassembler.h"

#include "../lib_vm/vm.h"
#include "../lib_vm/thread.h"
#include "../lib_vm/thread_error.h"

#include "../lib_builtins/builtin.h"


SDL_Window *g_window = nullptr;
SDL_GLContext g_glcontext = nullptr;
TextEditor g_editor;
bool g_running = true;

#if 1
std::string g_init_source = R"(
function main()
  var test[3] = 1, 2, 3
  return test[2]
end
)";
#else
std::string g_init_source = R"(
var seed = 12345

function rand()
  seed = seed * 1664525 + 1013904223
  return abs(seed) 
end

function main()
    var d[4]
    d[0] = 0
    d[1] = 1
    d[2] = 2
    d[3] = 3
    var i
    for (i = 0 to 4)
        var j = rand() % 4
        var t = d[j]
        d[j] = d[i]
        d[i] = t
    end
    print(d[0] + ", " + d[1] + ", " + d[2] + ", " + d[3])
end
)";
#endif

nano::program_t g_program;
std::unique_ptr<nano::vm_t> g_vm;
nano::thread_t *g_thread = nullptr;

enum run_option_t {
  RUN_COMPILE   = 1,
  RUN_CONTINUE  = 2,
  RUN_STEP_INST = 4,
  RUN_STEP_LINE = 8,
  RUN_STOP      = 16,
  RUN_RESTART   = 32,
};
uint32_t g_run_option = 0;

bool g_optimize = true;

// output lines from a running program
std::vector<std::string> g_output;

// the list of breakpoints
std::unordered_set<int> g_breakpoints;


void vm_print(nano::thread_t &t, int32_t) {
  using namespace nano;
  value_t *s = t.get_stack().pop();
  if (!s->is_a<val_type_string>()) {
    t.raise_error(thread_error_t::e_bad_argument);
  } else {
    g_output.push_back(s->string());
  }
  t.get_stack().push_int(0);
}

bool vm_handle_on_thread_error(nano::thread_t &t) {

  if (t.has_error()) {
    nano::thread_error_t error = t.get_error();
    (void)error;
    TextEditor::ErrorMarkers markers;
    const char *str = nano::get_thread_error(t.get_error());
    nano::line_t line = t.get_source_line();
    markers[line.line] = str;
    g_editor.SetErrorMarkers(markers);
  }

  g_thread = nullptr;
  return true;
}

bool vm_handle_on_thread_finish(nano::thread_t &t) {

  if (g_vm->finished()) {
    g_output.push_back("Finished after " + std::to_string(t.get_cycle_count()) + " cycles");
    const nano::value_t *ret = t.get_return_value();
    std::string ret_str = ret->to_string();
    g_output.push_back("Returned " + ret_str);
    g_run_option |= RUN_STOP;
  }

  g_thread = nullptr;
  return true;
}

bool init_sdl() {

  const int flags = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER;
  if (SDL_Init(flags) != 0) {
    printf("Error: %s\n", SDL_GetError());
    return false;
  }

  // Setup window
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  g_window = SDL_CreateWindow("Nano Script Testbed", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, 800, 600, window_flags);
  if (!g_window) {
    return false;
  }

  g_glcontext = SDL_GL_CreateContext(g_window);
  if (!g_glcontext) {
    return false;
  }

  SDL_GL_MakeCurrent(g_window, g_glcontext);
  SDL_GL_SetSwapInterval(1);

  return true;
}

void app_setup() {
  g_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::CCML());
  g_editor.SetPalette(TextEditor::GetDarkPalette());
  g_editor.SetText(g_init_source);
}

void gui_highlight_pc() {
  if (!g_thread) {
    return;
  }
  const int32_t line = g_thread->get_source_line().line;
  if (line < 0) {
    return;
  }
  if (!g_thread->finished()) {
    TextEditor::Coordinates c[2];
    c[0].mLine = line - 1;
    c[0].mColumn = 0;
    c[1].mLine = c[0].mLine;
    c[1].mColumn = 64;
    g_editor.SetSelection(c[0], c[1]);
  }
}

void lang_prepare() {

  // restart the program if needed
  if (g_run_option & RUN_RESTART) {
    g_thread = nullptr;
    g_vm.reset();
  }

  // if we are not going to run then skip this step
  if (0 == (g_run_option & (RUN_STEP_INST | RUN_STEP_LINE | RUN_CONTINUE))) {
    return;
  }

  // if we currently have a thread or a vm then no preparation to do
  if (g_vm || g_thread) {
    return;
  }

  g_output.push_back("Launching program");
  g_vm.reset(new nano::vm_t{g_program});
  g_vm->handlers.on_thread_error = vm_handle_on_thread_error;
  g_vm->handlers.on_thread_finish = vm_handle_on_thread_finish;

  if (!g_vm->call_init()) {
    g_output.push_back("Error when calling '@init' function!");
    return;
  }
  const nano::function_t *func = g_program.function_find("main");
  if (!func) {
    g_output.push_back("Unable to find 'main' function!");
    return;
  }

  g_thread = g_vm->new_thread(*func, 0, nullptr);
  if (!g_thread) {
    g_output.push_back("Error: unable to start thread!");
    return;
  }

  // since we are preparing treat this as our first step
  if (g_run_option & (RUN_STEP_INST | RUN_STEP_LINE)) {
    g_run_option &= ~(RUN_STEP_INST | RUN_STEP_LINE);
    gui_highlight_pc();
  }
}

void lang_run() {

  // check we have something to run
  if (0 == (g_run_option & (RUN_CONTINUE | RUN_STEP_INST | RUN_STEP_LINE))) {
    return;
  }

  if (!g_thread || !g_vm) {
    return;
  }

  for (const auto &b : g_breakpoints) {
    g_thread->breakpoint_add(nano::line_t{0, b});
  }

  if (g_thread->finished() || g_thread->has_error()) {
    g_output.push_back("Error: thread has terminated!");
    return;
  }

  if (g_run_option & RUN_STEP_INST) {
    if (!g_thread->step_inst()) {
      g_output.push_back("Error: thread.step_inst() returned false");
    }
  }

  if (g_run_option & RUN_STEP_LINE) {
    if (!g_thread->step_line()) {
      g_output.push_back("Error: thread.step_line() returned false");
    }
  }

  if (g_run_option & RUN_CONTINUE) {
    uint32_t max_cycles = 128 * 1024;
    if (!g_thread->resume(max_cycles)) {
      g_output.push_back("Error: thread.resume() returned false");
    }
  }

  if (g_thread) {
    gui_highlight_pc();
  }
}

void lang_compile() {
  using namespace nano;

  const std::string source = g_editor.GetText();

  if (0 == (g_run_option & RUN_COMPILE)) {
    return;
  }

  // clear out the VM before we tear down the program
  g_thread = nullptr;
  g_vm.reset();

  // wipe the program
  g_program.reset();

  nano_t c{g_program};
  builtins_register(c);
  c.syscall_register("print", 1);
  c.optimize = g_optimize;

  TextEditor::ErrorMarkers markers;

  source_manager_t sources;
  sources.load_from_string(source.c_str());

  error_t error;
  if (!c.build(sources, error)) {
    std::string err = std::to_string(error.line.file) + ":" +
                      std::to_string(error.line.line) + " " + error.error;

    auto pair = std::pair<int, std::string>(error.line.line, err);
    markers.insert(pair);

    g_output.push_back("Error: " + err);
  }
  else {
    g_output.push_back("Compile successful");
  }

  builtins_resolve(g_program);
  g_program.syscall_resolve("print", vm_print);

  g_editor.SetErrorMarkers(markers);
}

void gui_toggle_breakpoint() {

  TextEditor::Coordinates coord = g_editor.GetCursorPosition();
  int line = coord.mLine + 1;
  auto itt = g_breakpoints.find(line);
  if (itt == g_breakpoints.end()) {
    g_breakpoints.insert(line);
    if (g_thread) {
      g_thread->breakpoint_add(nano::line_t{0, line});
    }
  }
  else {
    if (g_thread) {
      g_thread->breakpoint_remove(nano::line_t{0, line});
    }
    g_breakpoints.erase(itt);
  }
  g_editor.SetBreakpoints(g_breakpoints);
}

void gui_print_value(const nano::value_t *v) {
  ImGui::Text("%s", v->to_string().c_str());
}

void gui_output() {
  ImGui::Begin("Output", nullptr, ImGuiWindowFlags_MenuBar);
  ImGui::SetWindowSize(ImVec2(256, 128), ImGuiCond_FirstUseEver);

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Log")) {
      if (ImGui::MenuItem("Clear")) {
        g_output.clear();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  for (const auto &l : g_output) {
    ImGui::Text(l.c_str());
  }

  ImGui::End();
}

void gui_program_inspector() {
  ImGui::Begin("Program Inspector");
  ImGui::SetWindowSize(ImVec2(380, 400), ImGuiCond_FirstUseEver);

  if (ImGui::CollapsingHeader("Options")) {
    ImGui::Indent(16.f);
    ImGui::Checkbox("Optimize", &g_optimize);
    ImGui::Unindent(16.f);
  }

  if (ImGui::CollapsingHeader("Strings")) {
    ImGui::Indent(16.f);
    for (const auto &s : g_program.strings()) {
      ImGui::PushID(&s);
      ImGui::Text("%s", s.c_str());
      ImGui::PopID();
    }
    ImGui::Unindent(16.f);
  }

  if (ImGui::CollapsingHeader("Line Table")) {
    ImGui::Indent(16.f);
    for (const auto &l : g_program.line_table()) {
      ImGui::PushID(&l);
      ImGui::Text("%03d -> file:%1d line:%03d", l.first, l.second.file,
                  l.second.line);
      ImGui::PopID();
    }
    ImGui::Unindent(16.f);
  }

  if (ImGui::CollapsingHeader("Globals")) {
    ImGui::Indent(16.f);
    for (const auto &a : g_program.globals()) {
      ImGui::PushID(&a);
      ImGui::Text("%2d: %s", a.offset_, a.name_.c_str());
      ImGui::PopID();
    }
    ImGui::Unindent(16.f);
  }

  if (ImGui::CollapsingHeader("Functions")) {
    for (const auto &f : g_program.functions()) {
      ImGui::PushID(&f);
      ImGui::Indent(16.f);

      if (ImGui::CollapsingHeader(f.name().c_str())) {
        ImGui::Indent(16.f);

        if (!f.args_.empty()) {
          if (ImGui::CollapsingHeader("Arguments")) {
            ImGui::Indent(16.f);
            for (const auto &a : f.args_) {
              ImGui::PushID(&a);
              ImGui::Text("%2d: %s", a.offset_, a.name_.c_str());
              ImGui::PopID();
            }
            ImGui::Unindent(16.f);
          }
        }

        if (!f.locals_.empty()) {
          if (ImGui::CollapsingHeader("Locals")) {
            ImGui::Indent(16.f);
            for (const auto &a : f.locals_) {
              ImGui::PushID(&a);
              ImGui::Text("%2d: %s", a.offset_, a.name_.c_str());
              ImGui::PopID();
            }
            ImGui::Unindent(16.f);
          }
        }

        if (ImGui::CollapsingHeader("Byte Code")) {
          ImGui::Indent(16.f);
          nano::disassembler_t dis;

          std::string out;
          const uint8_t *ptr = g_program.data() + f.code_start_;
          int32_t loc = f.code_start_;

          nano::line_t old_line = {-1, -1};

          for (; loc < f.code_end_;) {
            ImGui::PushID(loc);
            int offs = dis.disasm(ptr, out);
            if (offs > 0) {

              nano::line_t line = g_program.get_line(loc);
              if (line != old_line) {
                ImGui::Text("-- line %d\n", line.line);
                old_line = line;
              }

              ImGui::Text("%03d  %s\n", loc, out.c_str());
              loc += offs;
              ptr += offs;
            } else {
              ImGui::PopID();
              break;
            }
            ImGui::PopID();
          }

          ImGui::Unindent(16.f);
        }

        ImGui::Unindent(16.f);
      }

      ImGui::Unindent(16.f);
      ImGui::PopID();
    }
  }

  ImGui::End();
}

void gui_code_editor() {

  if (g_thread || g_vm) {
    g_editor.SetReadOnly(true);
  }
  else {
    g_editor.SetReadOnly(false);
  }

  auto cpos = g_editor.GetCursorPosition();
  ImGui::Begin("Code Editor", nullptr,
               ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
  ImGui::SetWindowSize(ImVec2(380, 400), ImGuiCond_FirstUseEver);

  if (ImGui::BeginMenuBar()) {

    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("serialize", "")) {
        if (g_program.serial_save("save.bin")) {
          g_output.push_back("serialized program");
        }
        else {
          g_output.push_back("error serializing program");
        }
      }

      if (ImGui::MenuItem("deserialize", "")) {
        if (g_program.serial_load("save.bin")) {
          g_output.push_back("deserialized program");
          nano::builtins_resolve(g_program);
          g_program.syscall_resolve("print", vm_print);
        }
        else {
          g_output.push_back("error deserializing program");
        }
      }

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Build")) {
      if (ImGui::MenuItem("Compile", "F7")) {
        g_run_option |= RUN_COMPILE;
      }

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {

      if (ImGui::MenuItem("Undo", "Ctrl-Z", nullptr,
                          g_editor.CanUndo()))
        g_editor.Undo();
      if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, g_editor.CanRedo()))
        g_editor.Redo();

      ImGui::Separator();

      if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, g_editor.HasSelection()))
        g_editor.Copy();
      if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr,
                          g_editor.HasSelection()))
        g_editor.Cut();
      if (ImGui::MenuItem("Delete", "Del", nullptr,
                          g_editor.HasSelection()))
        g_editor.Delete();
      if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr,
                          ImGui::GetClipboardText() != nullptr))
        g_editor.Paste();

      ImGui::Separator();

      if (ImGui::MenuItem("Select all", nullptr, nullptr))
        g_editor.SetSelection(TextEditor::Coordinates(),
                            TextEditor::Coordinates(g_editor.GetTotalLines(), 0));

      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }

  g_editor.Render("Code Editor");

  ImGui::End();
}

void gui_debug() {
  ImGui::Begin("Debugger", nullptr);
  ImGui::SetWindowSize(ImVec2(300, 128), ImGuiCond_FirstUseEver);

  if (ImGui::Button("Continue")) {
    g_run_option = RUN_CONTINUE;
  }
  ImGui::SameLine();
  if (ImGui::Button("Step Line")) {
    g_run_option = RUN_STEP_LINE;
  }
  ImGui::SameLine();
  if (ImGui::Button("Step Inst")) {
    g_run_option = RUN_STEP_INST;
  }
  ImGui::SameLine();
  if (ImGui::Button("Restart")) {
    g_run_option = RUN_RESTART;
  }
  ImGui::SameLine();
  if (ImGui::Button("Stop")) {
    g_run_option = RUN_STOP;
  }

  ImGui::Separator();

  ImGui::Text("PC: %d", g_thread ? g_thread->get_pc() : 0);

  // print the globals
  if (ImGui::CollapsingHeader("Globals")) {
    if (g_vm && g_thread) {
      for (const auto &g : g_program.globals()) {
        const nano::value_t *v = g_vm->globals()[g.offset_];
        ImGui::Text("%8s: %s", g.name_.c_str(), v->to_string().c_str());
      }
    }
  }

  // print a stack trace
  if (ImGui::CollapsingHeader("Value Stack")) {
    if (g_vm && g_thread) {
      const auto &vs = g_thread->get_stack();
      int32_t i = vs.head() - 1;
      for (; i >= 0; --i) {
        const nano::value_t *v = vs.get(i);
        ImGui::Text("%3d : %s", i, v->to_string().c_str());
      }
    }
  }

  // unwinder
  if (ImGui::CollapsingHeader("Unwind")) {
    if (g_vm && g_thread) {

      const auto &frames = g_thread->frames();
      const auto &stack = g_thread->get_stack();

      int32_t i = 0;
      for (auto itt = frames.rbegin(); itt != frames.rend(); ++itt, ++i) {
        const auto &frame = *itt;
        const nano::function_t *func = g_vm->program().function_find(frame.callee_);
        assert(func);

        ImGui::Text("frame %d: '%s'", i, func->name_.c_str());

        // print function arguments
        for (const auto &a : func->args_) {

          const int32_t index = frame.sp_ + a.offset_;
          const nano::value_t *v = stack.get(index);

          ImGui::Text("%8s: %s", a.name_.c_str(), v->to_string().c_str());
        }
        // print local variables
        for (const auto &l : func->locals_) {

          const int32_t index = frame.sp_ + l.offset_;
          const nano::value_t *v = stack.get(index);

          ImGui::Text("%8s: %s", l.name_.c_str(), v->to_string().c_str());
        }

        // exit on terminal frame
        if (frame.terminal_) {
          break;
        }
      }
    }
  }

  ImGui::End();
}

void poll_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT) {
      g_running = false;
    }
    if (event.type == SDL_KEYDOWN) {
      if (event.key.keysym.sym == SDLK_F5) {
        g_run_option |= RUN_CONTINUE;
      }
      if (event.key.keysym.sym == SDLK_F7) {
        g_run_option |= RUN_COMPILE;
      }
      if (event.key.keysym.sym == SDLK_F10) {
        g_run_option |= RUN_STEP_INST;
      }
      if (event.key.keysym.sym == SDLK_F11) {
        g_run_option |= RUN_STEP_LINE;
      }
      if (event.key.keysym.sym == SDLK_F9) {
        gui_toggle_breakpoint();
      }
    }
  }
}

int main(int argc, char **argv) {

  (void)argc;
  (void)argv;

  if (!init_sdl()) {
    return 1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(g_window, g_glcontext);
  ImGui_ImplOpenGL2_Init();

  app_setup();

  while (g_running) {

    poll_events();

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL2_NewFrame(g_window);
    ImGui::NewFrame();

    // update our windows
    gui_code_editor();
    gui_program_inspector();
    gui_output();
    gui_debug();

    // language
    lang_compile();
    lang_prepare();
    lang_run();

    if (g_run_option & RUN_STOP) {
      g_thread = nullptr;
      g_vm.reset();
    }

    g_run_option = 0;

    // 
    ImGui::Render();
    ImGuiIO io = ImGui::GetIO();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(.1f, .2f, .3f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(g_window);
  }

  // Cleanup
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(g_glcontext);
  SDL_DestroyWindow(g_window);
  SDL_Quit();

  return 0;
}
