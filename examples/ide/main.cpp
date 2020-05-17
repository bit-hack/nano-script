#include <stdio.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl2.h"
#include "imgui/imgui_impl_sdl.h"

#include "imgui/TextEditor.h"

#include "../lib_compiler/ccml.h"
#include "../lib_compiler/errors.h"
#include "../lib_compiler/codegen.h"
#include "../lib_compiler/parser.h"
#include "../lib_compiler/disassembler.h"

SDL_Window *window = nullptr;
SDL_GLContext gl_context = nullptr;

TextEditor editor;

std::string text = R"(
function main()
  return 0
end
)";

bool running = true;

ccml::program_t program;


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

  window = SDL_CreateWindow("CCML testbed", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, 800, 600, window_flags);
  if (!window) {
    return false;
  }

  gl_context = SDL_GL_CreateContext(window);
  if (!gl_context) {
    return false;
  }

  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1);

  return true;
}

void app_setup() {

  editor.SetLanguageDefinition(TextEditor::LanguageDefinition::CCML());

  editor.SetPalette(TextEditor::GetDarkPalette());
  editor.SetText(text);
}

void compile(std::string &str) {
  using namespace ccml;

  program.reset();

  ccml_t c{program};

  TextEditor::ErrorMarkers markers;

  source_manager_t sources;
  sources.load_from_string(str.c_str());

  error_t error;
  if (!c.build(sources, error)) {
    std::string err = std::to_string(error.line.file) + ":" +
                      std::to_string(error.line.line) + " " + error.error;

    auto pair = std::pair<int, std::string>(
      error.line.line, err);

    markers.insert(pair);
  }

  editor.SetErrorMarkers(markers);
}

void visit_program() {
  ImGui::Begin("Program Viewer");

  if (ImGui::CollapsingHeader("Globals")) {
  }

  if (ImGui::CollapsingHeader("Functions")) {
    for (const auto &f : program.functions()) {
      ImGui::Indent(16.f);

      if (ImGui::CollapsingHeader(f.name().c_str())) {
        ImGui::Indent(16.f);

        if (!f.args_.empty()) {
          if (ImGui::CollapsingHeader("Arguments")) {
            ImGui::Indent(16.f);
            for (const auto &a : f.args_) {
              ImGui::Indent(2.f);
              ImGui::Text("%2d: %s", a.offset_, a.name_.c_str());
            }
            ImGui::Unindent(16.f);
          }
        }

        if (!f.locals_.empty()) {
          if (ImGui::CollapsingHeader("Locals")) {
            ImGui::Indent(16.f);
            for (const auto &a : f.locals_) {
              ImGui::Indent(2.f);
              ImGui::Text("%2d: %s", a.offset_, a.name_.c_str());
            }
            ImGui::Unindent(16.f);
          }
        }

        if (ImGui::CollapsingHeader("Byte Code")) {
          ImGui::Indent(16.f);
          ccml::disassembler_t dis;

          std::string out;
          const uint8_t *ptr = program.data() + f.code_start_;
          uint32_t loc = f.code_start_;

          for (; loc < f.code_end_;) {
            int offs = dis.disasm(ptr, out);
            if (offs <= 0) {
              break;
            }
            loc += offs;
            ptr += offs;
            ImGui::Text("%03d  %s\n", loc, out.c_str());
          }

          ImGui::Unindent(16.f);
        }

        ImGui::Unindent(16.f);
      }

      ImGui::Unindent(16.f);
    }
  }

  ImGui::End();
}

void app_frame() {

  auto cpos = editor.GetCursorPosition();
  ImGui::Begin("Code Editor", nullptr,
               ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);

  ImGui::SetWindowSize(ImVec2(256, 256), ImGuiCond_FirstUseEver);

  if (ImGui::BeginMenuBar()) {

    if (ImGui::BeginMenu("Run")) {
      if (ImGui::MenuItem("Compile")) {
        auto code = editor.GetText();
        compile(code);
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {

      if (ImGui::MenuItem("Undo", "Ctrl-Z", nullptr,
                          editor.CanUndo()))
        editor.Undo();
      if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, editor.CanRedo()))
        editor.Redo();

      ImGui::Separator();

      if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
        editor.Copy();
      if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr,
                          editor.HasSelection()))
        editor.Cut();
      if (ImGui::MenuItem("Delete", "Del", nullptr,
                          editor.HasSelection()))
        editor.Delete();
      if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr,
                          ImGui::GetClipboardText() != nullptr))
        editor.Paste();

      ImGui::Separator();

      if (ImGui::MenuItem("Select all", nullptr, nullptr))
        editor.SetSelection(TextEditor::Coordinates(),
                            TextEditor::Coordinates(editor.GetTotalLines(), 0));

      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }

#if 0
  ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s ", cpos.mLine + 1,
              cpos.mColumn + 1, editor.GetTotalLines(),
              editor.IsOverwrite() ? "Ovr" : "Ins",
              editor.CanUndo() ? "*" : " ",
              editor.GetLanguageDefinition().mName.c_str());
#endif

  editor.Render("Code Editor");

  ImGui::End();
}

void poll_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT) {
      running = false;
    }
  }
}

int main(int argc, char **argv) {

  if (!init_sdl()) {
    return 1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL2_Init();

  app_setup();

  while (running) {

    poll_events();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    app_frame();
    visit_program();

    // Rendering
    ImGui::Render();
    ImGuiIO io = ImGui::GetIO();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(.1f, .2f, .3f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }

  // Cleanup
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
