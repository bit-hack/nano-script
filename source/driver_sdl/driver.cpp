#include <cstdio>
#include <memory>

#define _SDL_main_h
#include <SDL.h>

#include "../nanoscript.h"

#include "../lib_vm/vm.h"

#include "../lib_builtins/builtin.h"


namespace {

uint32_t tick_mark = 0;

struct global_t {
  SDL_Window *window_ = nullptr;
  SDL_Surface *screen_ = nullptr;
  uint32_t rgb_ = 0xffffff;
  uint32_t width_, height_;
  std::unique_ptr<uint32_t[]> video_;
} global;

static inline uint32_t xorshift32() {
  static uint32_t x = 12345;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

void vm_rand(nano::thread_t &t, int32_t) {
  // new unsigned int
  const int32_t x = (xorshift32() & 0x7fffff);
  // return value
  t.get_stack().push(t.gc().new_int(x));
}

void vm_cls(nano::thread_t &t, int32_t) {
  uint32_t *v = global.video_.get();
  for (uint32_t y = 0; y < global.height_; ++y) {
    for (uint32_t x = 0; x < global.width_; ++x) {
      v[x] = 0x0;
    }
    v += global.width_;
  }
  // return value
  t.get_stack().push(t.gc().new_none());
}

void vm_sleep(nano::thread_t &t, int32_t) {
  nano::value_t *val = t.get_stack().pop();
  if (val->is_number()) {
    tick_mark = SDL_GetTicks() + val->as_int();
    t.halt();
  }
  // return value
  t.get_stack().push(t.gc().new_none());
}

void vm_video(nano::thread_t &t, int32_t) {
  using namespace nano;
  const value_t *h = t.get_stack().pop();
  const value_t *w = t.get_stack().pop();

  bool ok = false;

  if (w->is_a<val_type_int>() && h->is_a<val_type_int>()) {
    global.width_ = (uint32_t)w->v;
    global.height_ = (uint32_t)h->v;
    global.video_.reset(new uint32_t[(uint32_t)(w->v * h->v)]);
    memset(global.video_.get(), 0, w->v * h->v * sizeof(uint32_t));

    global.window_ = SDL_CreateWindow("Nano Script",
                                      SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                                      global.width_ * 3,
                                      global.height_ * 3,
                                      0);
    if (global.window_) {
      global.screen_ = SDL_GetWindowSurface(global.window_);
    }
    if (!global.window_ || !global.screen_) {
      ok = true;
    }
  }

  // return value
  t.get_stack().push(t.gc().new_int(ok ? 1 : 0));
}

void vm_setrgb(nano::thread_t &t, int32_t) {
  using namespace nano;
  const value_t *b = t.get_stack().pop();
  const value_t *g = t.get_stack().pop();
  const value_t *r = t.get_stack().pop();
  if (r->is_number() && g->is_number() && b->is_number()) {
    const int32_t ir = r->as_int();
    const int32_t ig = g->as_int();
    const int32_t ib = b->as_int();
    global.rgb_ = ((ir & 0xff) << 16) | ((ig & 0xff) << 8) | (ib & 0xff);
  }
  // return code
  t.get_stack().push(t.gc().new_none());
}

static inline void plot(int32_t x, int32_t y) {
  if (auto *s = global.screen_) {
    (void)s;
    if (x >= 0 && x < (int32_t)global.width_) {
      if (y >= 0 && y < (int32_t)global.height_) {
        uint32_t *v = (uint32_t *)global.video_.get();
        v += y * global.width_;
        v += x;
        *v = global.rgb_;
      }
    }
  }
}

static inline void span(int32_t x0, int32_t x1, int32_t y0) {
  if (auto *s = global.screen_) {
    (void)s;
    if (y0 < 0 || y0 >= (int32_t)global.height_) {
      return;
    }
    x0 = std::max<int32_t>(x0, 0);
    x1 = std::min<int32_t>(x1, global.width_);
    uint32_t *v = global.video_.get();
    v += global.width_ * y0;
    const uint32_t *e = v + x1;
    v += x0;
    for (; v < e; ++v) {
      *v = global.rgb_;
    }
  }
}

void vm_circle(nano::thread_t &t, int32_t) {
  using namespace nano;

  const value_t *r = t.get_stack().pop();
  const value_t *py = t.get_stack().pop();
  const value_t *px = t.get_stack().pop();
  t.get_stack().push(t.gc().new_none());

  if (!py->is_number() || !px->is_number() || !r->is_number()) {
    return;
  }

  const int32_t xC = px->as_int();
  const int32_t yC = py->as_int();
  const int32_t radius = r->as_int();

  int32_t p = 1 - radius;
  int32_t x = 0;
  int32_t y = radius;
  span(xC - y, xC + y, yC);
  while (x++ <= y) {
    if (p < 0) {
      p += 2 * x + 1;
    } else {
      p += 2 * (x - y) + 1;
      y--;
    }
    span(xC - x, xC + x, yC + y);
    span(xC - x, xC + x, yC - y);
    span(xC - y, xC + y, yC + x);
    span(xC - y, xC + y, yC - x);
  }
}

static inline void line(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
  bool yLonger = false;
  int32_t x = x0, y = y0;
  int32_t incrementVal, endVal;
  int32_t shortLen = y1 - y0, longLen = x1 - x0;
  if (abs(shortLen) > abs(longLen)) {
    std::swap(shortLen, longLen);
    yLonger = true;
  }
  endVal = longLen;
  if (longLen < 0) {
    incrementVal = -1;
    longLen = -longLen;
  } else {
    incrementVal = 1;
  }
  const int32_t decInc = (longLen == 0) ? 0 : (shortLen << 16) / longLen;
  if (yLonger) {
    for (int32_t i = 0, j = 0; i != endVal; i += incrementVal, j += decInc) {
      plot(x + (j >> 16), y + i);
    }
  } else {
    for (int32_t i = 0, j = 0; i != endVal; i += incrementVal, j += decInc) {
      plot(x + i, y + (j >> 16));
    }
  }
}

void vm_line(nano::thread_t &t, int32_t) {
  using namespace nano;
  const value_t *y1 = t.get_stack().pop();
  const value_t *x1 = t.get_stack().pop();
  const value_t *y0 = t.get_stack().pop();
  const value_t *x0 = t.get_stack().pop();
  t.get_stack().push(t.gc().new_none());
  if (x0->is_a<val_type_int>() &&
      y0->is_a<val_type_int>() &&
      x1->is_a<val_type_int>() &&
      y1->is_a<val_type_int>()) {
    line(x0->integer(), y0->integer(), x1->integer(), y1->integer());
  }
}

void vm_keydown(nano::thread_t &t, int32_t) {
  using namespace nano;
  const value_t *key = t.get_stack().pop();
  if (!key->is_a<val_type_string>()) {
    t.get_stack().push(t.gc().new_none());
    return;
  }

  const uint8_t *keys = SDL_GetKeyboardState(nullptr);
  const char *s = key->string();
  if (strcmp(s, "up") == 0) {
    const uint8_t state = keys[SDL_SCANCODE_UP];
    t.get_stack().push(t.gc().new_int(state ? 1 : 0));
    return;
  }
  if (strcmp(s, "down") == 0) {
    const uint8_t state = keys[SDL_SCANCODE_DOWN];
    t.get_stack().push(t.gc().new_int(state ? 1 : 0));
    return;
  }
  if (strcmp(s, "left") == 0) {
    const uint8_t state = keys[SDL_SCANCODE_LEFT];
    t.get_stack().push(t.gc().new_int(state ? 1 : 0));
    return;
  }
  if (strcmp(s, "right") == 0) {
    const uint8_t state = keys[SDL_SCANCODE_RIGHT];
    t.get_stack().push(t.gc().new_int(state ? 1 : 0));
    return;
  }
  if (strcmp(s, "escape") == 0) {
    t.get_stack().push(t.gc().new_int(keys[SDL_SCANCODE_ESCAPE] ? 1 : 0));
    return;
  }
  if (strcmp(s, "space") == 0) {
    t.get_stack().push(t.gc().new_int(keys[SDL_SCANCODE_SPACE] ? 1 : 0));
    return;
  }
  t.get_stack().push(t.gc().new_none());
}

void vm_plot(nano::thread_t &t, int32_t) {
  using namespace nano;

  const value_t *y = t.get_stack().pop();
  const value_t *x = t.get_stack().pop();
  // return value
  t.get_stack().push(t.gc().new_none());
  if (x->is_number() && y->is_number()) {
    plot(x->as_int(), y->as_int());
  }
}

void vm_flip(nano::thread_t &t, int32_t) {
  using namespace nano;

  const uint32_t w = global.width_;
  const uint32_t h = global.height_;

  const uint32_t p0 = 0;
  const uint32_t p1 = w * 3;
  const uint32_t p2 = p1 + p1;

  if (auto *screen = global.screen_) {

    uint32_t *s = (uint32_t *)global.video_.get();
    uint32_t *d = (uint32_t *)global.screen_->pixels;

    for (uint32_t y = 0; y < h; ++y) {
      for (uint32_t x = 0, j = 0; x < h; ++x, j += 3) {
        const uint32_t c = s[x];
        d[j + 0 + p0] = d[j + 1 + p0] = d[j + 2 + p0] = c;
        d[j + 0 + p1] = d[j + 1 + p1] = d[j + 2 + p1] = c;
        d[j + 0 + p2] = d[j + 1 + p2] = d[j + 2 + p2] = c;
      }
      s += global.width_;
      d += p1 * 3;
    }

    SDL_UpdateWindowSurface(global.window_);
  }
  // return value
  t.get_stack().push(t.gc().new_none());
}

void on_error(const nano::error_t &error) {
  fprintf(stderr, "file %d, line:%d - %s\n",
          error.line.file,
          error.line.line,
          error.error.c_str());
  fflush(stderr);
  exit(1);
}
} // namespace

int main(int argc, char **argv) {
  using namespace nano;

  if (SDL_Init(SDL_INIT_VIDEO)) {
    return 1;
  }

  // load the source
  source_manager_t sources;
  for (int i = 1; i < argc; ++i) {
    if (!sources.load(argv[i])) {
      fprintf(stderr, "unable to load input '%s'\n", argv[i]);
      return -2;
    }
  }
  if (sources.count() == 0) {
    fprintf(stderr, "no source files provided\n");
    return -1;
  }

  program_t program;

  // compile
  {
    nano_t nano(program);
    builtins_register(nano);
    nano.syscall_register("cls", 0);
    nano.syscall_register("rand", 0);
    nano.syscall_register("video", 2);
    nano.syscall_register("plot", 2);
    nano.syscall_register("flip", 0);
    nano.syscall_register("setrgb", 3);
    nano.syscall_register("circle", 3);
    nano.syscall_register("line", 4);
    nano.syscall_register("sleep", 1);
    nano.syscall_register("keydown", 1);

    nano::error_t error;
    if (!nano.build(sources, error)) {
      on_error(error);
      return -2;
    }
  }

  builtins_resolve(program);
  program.syscall_resolve("cls",     vm_cls);
  program.syscall_resolve("rand",    vm_rand);
  program.syscall_resolve("video",   vm_video);
  program.syscall_resolve("plot",    vm_plot);
  program.syscall_resolve("flip",    vm_flip);
  program.syscall_resolve("setrgb",  vm_setrgb);
  program.syscall_resolve("circle",  vm_circle);
  program.syscall_resolve("line",    vm_line);
  program.syscall_resolve("sleep",   vm_sleep);
  program.syscall_resolve("keydown", vm_keydown);

  const function_t *func = program.function_find("main");
  if (!func) {
    fprintf(stderr, "unable to locate function 'main'\n");
    exit(1);
  }

  nano::vm_t vm(program);

  if (!vm.call_init()) {
    fprintf(stderr, "failed while executing @init\n");
  }

  if (!vm.new_thread(*func, 0, nullptr)) {
    fprintf(stderr, "unable prepare function 'main'\n");
    exit(1);
  }

  bool active = true;
  while (active) {

    if (SDL_GetTicks() > tick_mark) {
      if (!vm.resume(1024)) {
        break;
      }
    }
    else {
      SDL_Delay(1);
    }

    // process SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        active = false;
        break;
      }
    }
  }

  if (vm.has_error()) {
    // XXX: FIXME
#if 0
    const thread_error_t err = thread.get_error();
    if (err != thread_error_t::e_success) {
      line_t line = thread.get_source_line();
      printf("runtime error %d\n", int32_t(err));
      fprintf(stderr, "%s\n", get_thread_error(thread.get_error()));
      printf("source line %d\n", int32_t(line.line));
      const std::string &s = sources.get_line(line);
      printf("%s\n", s.c_str());
    }
#endif
    return 1;
  }

  fflush(stdout);
  printf("exit: %d\n", 0);
  SDL_Quit();
  return 0;
}
