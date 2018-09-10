#include <cstdarg>
#include <cstdio>

#include "errors.h"


using namespace ccml;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
void error_manager_t::on_error_(uint32_t line, const char *fmt, ...) {
  // generate the error message
  char buffer[1024] = {'\0'};
  va_list va;
  va_start(va, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, va);
  buffer[sizeof(buffer) - 1] = '\0';
  va_end(va);
  // throw an error
  error_t error{buffer, line};
  throw error;
}
