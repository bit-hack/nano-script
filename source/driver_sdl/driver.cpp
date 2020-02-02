#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <memory>

#define _SDL_main_h
#include <SDL/SDL.h>

#include "../lib/ccml.h"
#include "../lib/codegen.h"
#include "../lib/disassembler.h"
#include "../lib/errors.h"
#include "../lib/lexer.h"
#include "../lib/parser.h"
#include "../lib/vm.h"

namespace {

uint32_t tick_mark = 0;

struct global_t {
  SDL_Surface *screen_ = nullptr;
  uint32_t rgb_ = 0xffffff;
  uint32_t width_, height_;
  std::unique_ptr<uint32_t[]> video_;
} global;

const char *load_file(const char *path) {
  FILE *fd = fopen(path, "rb");
  if (!fd)
    return nullptr;

  fseek(fd, 0, SEEK_END);
  size_t size = size_t(ftell(fd));
  fseek(fd, 0, SEEK_SET);
  if (size <= 0) {
    fclose(fd);
    return nullptr;
  }

  char *source = new char[size + 1];
  if (fread(source, 1, size, fd) != size) {
    fclose(fd);
    delete[] source;
    return nullptr;
  }
  source[size] = '\0';

  fclose(fd);
  return source;
}

static inline uint32_t xorshift32() {
  static uint32_t x = 12345;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

void vm_rand(ccml::thread_t &t, int32_t) {
  // new unsigned int
  const int32_t x = (xorshift32() & 0x7fffff);
  // return value
  t.stack().push(t.gc().new_int(x));
}

void vm_cls(ccml::thread_t &t, int32_t) {
  uint32_t *v = global.video_.get();
  for (uint32_t y = 0; y < global.height_; ++y) {
    for (uint32_t x = 0; x < global.width_; ++x) {
      v[x] = 0x0;
    }
    v += global.width_;
  }
  // return value
  t.stack().push(t.gc().new_none());
}

void vm_sleep(ccml::thread_t &t, int32_t) {
  ccml::value_t *val = t.stack().pop();
  if (val->is_number()) {
    tick_mark = SDL_GetTicks() + val->as_int();
    t.halt();
  }
  // return value
  t.stack().push(t.gc().new_none());
}

void vm_video(ccml::thread_t &t, int32_t) {
  using namespace ccml;
  const value_t *h = t.stack().pop();
  const value_t *w = t.stack().pop();
  if (w->is_int() && h->is_int()) {
    global.width_ = (uint32_t)w->v;
    global.height_ = (uint32_t)h->v;
    global.video_.reset(new uint32_t[(uint32_t)(w->v * h->v)]);
    memset(global.video_.get(), 0, w->v * h->v * sizeof(uint32_t));
    global.screen_ =
        SDL_SetVideoMode((uint32_t)(w->v * 3), (uint32_t)(h->v * 3), 32, 0);
    // return value
    t.stack().push(t.gc().new_int(global.screen_ != nullptr));
  } else {
    t.stack().push(t.gc().new_int(0));
  }
}

void vm_setrgb(ccml::thread_t &t, int32_t) {
  using namespace ccml;
  const value_t *b = t.stack().pop();
  const value_t *g = t.stack().pop();
  const value_t *r = t.stack().pop();
  if (r->is_number() && g->is_number() && b->is_number()) {
    const int32_t ir = r->as_int();
    const int32_t ig = g->as_int();
    const int32_t ib = b->as_int();
    global.rgb_ = ((ir & 0xff) << 16) | ((ig & 0xff) << 8) | (ib & 0xff);
  }
  // return code
  t.stack().push(t.gc().new_none());
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

void vm_circle(ccml::thread_t &t, int32_t) {
  using namespace ccml;

  const value_t *r = t.stack().pop();
  const value_t *py = t.stack().pop();
  const value_t *px = t.stack().pop();
  t.stack().push(t.gc().new_none());

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

void vm_line(ccml::thread_t &t, int32_t) {
  using namespace ccml;
  const value_t *y1 = t.stack().pop();
  const value_t *x1 = t.stack().pop();
  const value_t *y0 = t.stack().pop();
  const value_t *x0 = t.stack().pop();
  t.stack().push(t.gc().new_none());
  if (x0->is_int() && y0->is_int() && x1->is_int() && y1->is_int()) {
    line(x0->integer(), y0->integer(), x1->integer(), y1->integer());
  }
}

void vm_keydown(ccml::thread_t &t, int32_t) {
  using namespace ccml;
  const value_t *key = t.stack().pop();
  if (!key->is_string()) {
    t.stack().push(t.gc().new_none());
    return;
  }
  const uint8_t *keys = SDL_GetKeyState(nullptr);
  const char *s = key->string();
  if (strcmp(s, "up") == 0) {
    const uint8_t state = keys[SDLK_UP];
    t.stack().push(t.gc().new_int(state ? 1 : 0));
    return;
  }
  if (strcmp(s, "down") == 0) {
    const uint8_t state = keys[SDLK_DOWN];
    t.stack().push(t.gc().new_int(state ? 1 : 0));
    return;
  }
  if (strcmp(s, "left") == 0) {
    const uint8_t state = keys[SDLK_LEFT];
    t.stack().push(t.gc().new_int(state ? 1 : 0));
    return;
  }
  if (strcmp(s, "right") == 0) {
    const uint8_t state = keys[SDLK_RIGHT];
    t.stack().push(t.gc().new_int(state ? 1 : 0));
    return;
  }
  if (strcmp(s, "escape") == 0) {
    t.stack().push(t.gc().new_int(keys[SDLK_ESCAPE] ? 1 : 0));
    return;
  }
  if (strcmp(s, "space") == 0) {
    t.stack().push(t.gc().new_int(keys[SDLK_SPACE] ? 1 : 0));
    return;
  }
  t.stack().push(t.gc().new_none());
}

void vm_plot(ccml::thread_t &t, int32_t) {
  using namespace ccml;

  const value_t *y = t.stack().pop();
  const value_t *x = t.stack().pop();
  // return value
  t.stack().push(t.gc().new_none());
  if (x->is_number() && y->is_number()) {
    plot(x->as_int(), y->as_int());
  }
}

void vm_flip(ccml::thread_t &t, int32_t) {
  using namespace ccml;

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

    SDL_Flip(screen);
  }
  // return value
  t.stack().push(t.gc().new_none());
}

void on_error(const ccml::error_t &error) {
  fprintf(stderr, "line:%d - %s\n", error.line, error.error.c_str());
  fflush(stderr);
  exit(1);
}
} // namespace

int main(int argc, char **argv) {
  using namespace ccml;

  if (SDL_Init(SDL_INIT_VIDEO)) {
    return 1;
  }

  ccml_t ccml;
  ccml.add_function("cls", vm_cls, 0);
  ccml.add_function("rand", vm_rand, 0);
  ccml.add_function("video", vm_video, 2);
  ccml.add_function("plot", vm_plot, 2);
  ccml.add_function("flip", vm_flip, 0);
  ccml.add_function("setrgb", vm_setrgb, 3);
  ccml.add_function("circle", vm_circle, 3);
  ccml.add_function("line", vm_line, 4);
  ccml.add_function("sleep", vm_sleep, 1);
  ccml.add_function("keydown", vm_keydown, 1);

  if (argc <= 1) {
    return -1;
  }
  const char *source = load_file(argv[1]);
  if (!source) {
    fprintf(stderr, "unable to load input");
    return -1;
  }

  SDL_WM_SetCaption(argv[1], nullptr);

  ccml::error_t error;
  if (!ccml.build(source, error)) {
    on_error(error);
    return -2;
  }

  const function_t *func = ccml.find_function("main");
  if (!func) {
    fprintf(stderr, "unable to locate function 'main'\n");
    exit(1);
  }

  ccml::vm_t vm{ccml};
  ccml::thread_t thread{ccml, vm};

  if (!thread.init()) {
    fprintf(stderr, "failed while executing @init\n");
  }

  if (!thread.prepare(*func, 0, nullptr)) {
    fprintf(stderr, "unable prepare function 'main'\n");
    exit(1);
  }

  bool trace = false;
  bool active = true;
  while (active) {

    if (SDL_GetTicks() > tick_mark) {
      if (!thread.resume(1024, trace)) {
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

  if (thread.has_error()) {
    const thread_error_t err = thread.error();
    if (err != thread_error_t::e_success) {
      int32_t line = thread.source_line();
      printf("runtime error %d\n", int32_t(err));
      fprintf(stderr, "%s\n", get_thread_error(thread.error()));
      printf("source line %d\n", int32_t(line));
      const std::string &s = ccml.lexer().get_line(line);
      printf("%s\n", s.c_str());
    }
    return 1;
  }

  fflush(stdout);
  printf("exit: %d\n", 0);
  SDL_Quit();
  return 0;
}
