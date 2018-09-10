#pragma once
#include <cstring>
#include <string>
#include <array>
#include <map>

#include "ccml.h"
#include "token.h"
#include "instructions.h"


namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// the asm stream bridges the assembler and the code store
struct asm_stream_t {

  asm_stream_t(code_store_t &store)
    : store_(store)
    , end(store.end())
    , start(store.data())
    , ptr(store.data())
  {
  }

  bool write8(const uint8_t data) {
    if (end - ptr > 1) {
      *ptr = data;
      ptr += 1;
      return true;
    }
    return false;
  }

  bool write32(const uint32_t data) {
    if (end - ptr > 4) {
      memcpy(ptr, &data, 4);
      ptr += 4;
      return true;
    }
    return false;
  }

  const uint8_t *tail() const {
    return end;
  }

  size_t written() const {
    return ptr - start;
  }

  const uint8_t *data() const {
    return start;
  }

  size_t size() const {
    return end - start;
  }

  uint32_t pos() const {
    return ptr - start;
  }

  uint8_t *head(int32_t adjust) const {
    uint8_t *p = ptr + adjust;
    assert(p < end && p >= start);
    return p;
  }

  // set the line number for the current pc
  // if line is nullptr then current lexer line is used
  void set_line(lexer_t &lexer, const token_t *line);

  // add identifier
  void add_ident(const identifier_t &ident);

protected:
  code_store_t &store_;
  std::map<const uint8_t *, uint32_t> line_table_;

  const uint8_t *end;
  uint8_t *start;
  uint8_t *ptr;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
struct assembler_t {

  assembler_t(ccml_t &c, asm_stream_t &stream);

  // emit into the code stream
  void     emit(token_e, const token_t *t = nullptr);
  void     emit(instruction_e ins, const token_t *t = nullptr);
  int32_t *emit(instruction_e ins, int32_t v, const token_t *t = nullptr);

  // add an identifier for debug purposes
  void add_ident(const struct identifier_t &ident);

  // return the current output head
  int32_t pos() const;

  // return a reference to the last instructions operand
  int32_t &get_fixup();

  // reset any stored state
  void reset();

  // return the currently bound output stream
  asm_stream_t &stream;

protected:
  std::vector<uint32_t> scope_pc_;

  // write 8bits to the code stream
  void write8(const uint8_t v);

  // write 32bits to the code stream
  void write32(const int32_t v);

  ccml_t &ccml_;
};

} // namespace ccml
