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
  long size = ftell(fd);
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

uint32_t xorshift32() {
  static uint32_t x = 12345;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

void vm_rand(ccml::thread_t &t) {
  const uint32_t x = xorshift32();
  // return value
  t.push(ccml::value_from_int(x));
}

void vm_video(ccml::thread_t &t) {
  using namespace ccml;

  const int32_t h = value_to_int(t.pop());
  const int32_t w = value_to_int(t.pop());

  global.width_ = w;
  global.height_ = h;
  global.video_.reset(new uint32_t[w * h]);
  global.screen_ = SDL_SetVideoMode(w*3, h*3, 32, 0);
  // return value
  t.push(ccml::value_from_int(global.screen_ != nullptr));
}

void vm_setrgb(ccml::thread_t &t) {
  using namespace ccml;

  const int32_t b = value_to_int(t.pop());
  const int32_t g = value_to_int(t.pop());
  const int32_t r = value_to_int(t.pop());
  global.rgb_ = ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
  // return code
  t.push(value_from_int(0));
}

void vm_plot(ccml::thread_t &t) {
  using namespace ccml;

  const int32_t y = value_to_int(t.pop());
  const int32_t x = value_to_int(t.pop());
  const int32_t w = global.width_;
  const int32_t h = global.height_;
  // return value
  t.push(value_from_int(0));
  // do the plot
  if (auto *s = global.screen_) {
    uint32_t *v = global.video_.get();
    if (x < 0 || y < 0 || x >= w || y >= h) {
      return;
    }
    v[x + y * w] = global.rgb_;
  }
}

void vm_flip(ccml::thread_t &t) {
  using namespace ccml;

  const uint32_t w = global.width_;
  const uint32_t h = global.height_;

  const uint32_t p0 = 0;
  const uint32_t p1 = w * 3;
  const uint32_t p2 = p1 + p1;

  if (auto *screen = global.screen_) {

    uint32_t *s = (uint32_t*)global.video_.get();
    uint32_t *d = (uint32_t*)global.screen_->pixels;

    for (int32_t y = 0; y < h; ++y) {
      for (int32_t x = 0, j = 0; x < h; ++x, j +=3) {
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
  t.push(value_from_int(0));
}

void on_error(const ccml::error_t &error) {
  fprintf(stderr, "line:%d - %s\n", error.line, error.error.c_str());
  fflush(stderr);
  exit(1);
}

}; // namespace

int main(int argc, char **argv) {
  using namespace ccml;

  if (SDL_Init(SDL_INIT_VIDEO)) {
    return 1;
  }

  ccml_t ccml;
  ccml.add_function("rand", vm_rand, 0);
  ccml.add_function("video", vm_video, 2);
  ccml.add_function("plot", vm_plot, 2);
  ccml.add_function("flip", vm_flip, 0);
  ccml.add_function("setrgb", vm_setrgb, 3);

  if (argc <= 1) {
    return -1;
  }
  const char *source = load_file(argv[1]);
  if (!source) {
    fprintf(stderr, "unable to load input");
    return -1;
  }

  error_t error;
  if (!ccml.build(source, error)) {
    on_error(error);
    return -2;
  }

  ccml.disassembler().disasm();

  const function_t *func = ccml.find_function("main");
  if (!func) {
    fprintf(stderr, "unable to locate function 'main'\n");
    exit(1);
  }

  int32_t res = 0;

  ccml::thread_t thread{ccml};
  if (!thread.prepare(*func, 0, nullptr)) {
    fprintf(stderr, "unable prepare function 'main'\n");
    exit(1);
  }

  bool trace = false;
  bool active = true;
  while (active && thread.resume(1024, trace)) {

    // process SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        active = false;
        break;
      }
    }
  }

  fflush(stdout);
  printf("exit: %d\n", res);
  SDL_Quit();
  return 0;
}
