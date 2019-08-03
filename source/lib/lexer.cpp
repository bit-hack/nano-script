#include "lexer.h"
#include "errors.h"


using namespace ccml;

namespace {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
bool is_alpha(const char t) {
  return (t >= 'a' && t <= 'z') || (t >= 'A' && t <= 'Z') || (t >= '_');
}

bool is_numeric(const char t) {
  return (t >= '0' && t <= '9');
}

bool is_alpha_numeric(const char t) {
  return is_alpha(t) ||
         is_numeric(t);
}

bool munch(const char *&s, const char *string) {
  const char *t = s;
  for (;; t++, string++) {
    if (*string == '\0') {
      // avoid matching things like 'andy' as 'and'
      if (is_alpha_numeric(*t)) {
        return false;
      }
      s = t - 1;
      return true;
    }
    if (tolower(*t) != tolower(*string)) {
      return false;
    }
  }
}

} // namespace

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
bool lexer_t::lex(const char *s) {

  const char *new_line_ = s;
  line_no_ = 0;

  for (; *s; ++s) {

    // skip white space
    if (*s == ' ') {
      continue;
    }
    if (*s == '\r') {
      continue;
    }
    if (*s == '\t') {
      continue;
    }

    // consume a string
    if (*s == '"') {
      const char *b = s + 1;
      for (++s; ; ++s) {
        if (*s == '"') {
          break;
        }
        if (*s == '\n' || *s == '\0') {
          // raise an error
          lines_.push_back(std::string(new_line_, (s+1) - new_line_));
          ccml_.errors().string_quote_mismatch(line_no_, *s);
          return false;
        }
      }
      push_string_(b, s);
      continue;
    }

    // comments
    if (*s == '#') {
      for (; *s && *s != '\n'; ++s) {
        // empty
      }
      {
        assert(s >= new_line_);
        lines_.push_back(std::string(new_line_, s - new_line_));
        new_line_ = s + 1;
        ++line_no_;
      }
      push_(TOK_EOL);
      continue;
    }

    // parse keywords
    switch (tolower(*s)) {
    case 'a':
      if (munch(s, "and")) {
        push_(TOK_AND);
        continue;
      }
      break;
    case 'c':
      if (munch(s, "const")) {
        push_(TOK_CONST);
        continue;
      }
    case 'e':
      if (munch(s, "end")) {
        push_(TOK_END);
        continue;
      }
      if (munch(s, "else")) {
        push_(TOK_ELSE);
        continue;
      }
      break;
    case 'f':
      if (munch(s, "for")) {
        push_(TOK_FOR);
        continue;
      }
      if (munch(s, "function")) {
        push_(TOK_FUNC);
        continue;
      }
      break;
    case 'i':
      if (munch(s, "if")) {
        push_(TOK_IF);
        continue;
      }
      break;
    case 'n':
      if (munch(s, "not")) {
        push_(TOK_NOT);
        continue;
      }
      if (munch(s, "none")) {
        push_(TOK_NONE);
        continue;
      }
      break;
    case 'o':
      if (munch(s, "or")) {
        push_(TOK_OR);
        continue;
      }
      break;
    case 'r':
      if (munch(s, "return")) {
        push_(TOK_RETURN);
        continue;
      }
      break;
    case 't':
      if (munch(s, "to")) {
        push_(TOK_TO);
        continue;
      }
      break;
    case 'v':
      if (munch(s, "var")) {
        push_(TOK_VAR);
        continue;
      }
      break;
    case 'w':
      if (munch(s, "while")) {
        push_(TOK_WHILE);
        continue;
      }
      break;
    case '(':
      push_(TOK_LPAREN);
      continue;
    case ')':
      push_(TOK_RPAREN);
      continue;
    case '[':
      push_(TOK_LBRACKET);
      continue;
    case ']':
      push_(TOK_RBRACKET);
      continue;
    case ',':
      push_(TOK_COMMA);
      continue;
    case '+':
      push_(TOK_ADD);
      continue;
    case '-':
      push_(TOK_SUB);
      continue;
    case '*':
      push_(TOK_MUL);
      continue;
    case '/':
      push_(TOK_DIV);
      continue;
    case '%':
      push_(TOK_MOD);
      continue;
    case '\n':
      assert(s >= new_line_);
      lines_.push_back(std::string(new_line_, s - new_line_));
      new_line_ = s + 1;
      ++line_no_;
      push_(TOK_EOL);
      continue;
    case '=':
      if (s[1] == '=') {
        push_(TOK_EQ);
        ++s;
      } else {
        push_(TOK_ASSIGN);
      }
      continue;
    case '<':
      if (s[1] == '=') {
        push_(TOK_LEQ);
        ++s;
      } else {
        push_(TOK_LT);
      }
      continue;
    case '>':
      if (s[1] == '=') {
        push_(TOK_GEQ);
        ++s;
      } else {
        push_(TOK_GT);
      }
      continue;
    } // switch

    // try to parse number
    if (is_numeric(*s))
    {
      const char *t = s;
      for ( ;is_numeric(*t); ++t);
      if (*t == '.') {
        ++t;
        for ( ;is_numeric(*t); ++t);
      }

      if (t != s) {
        push_val_(s, t);
        s = t - 1; // (-1 because of ++ in forloop)
        continue;
      }
    }

    // try to parse identifier
    if (is_alpha(*s))
    {
      const char *t = s;
      for (; is_alpha_numeric(*t); ++t) {
        // empty
      }
      if (t != s) {
        push_ident_(s, t);
        s = t - 1; // (-1 because of ++ in forloop)
        continue;
      }
    }

    // raise an error
    lines_.push_back(std::string(new_line_, (s+1) - new_line_));
    ccml_.errors().unexpected_character(line_no_, *s);

  } // while

  lines_.push_back(std::string(new_line_, (s - new_line_)));
  stream_.push(token_t{TOK_EOF, line_no_});
  return true;
}

void lexer_t::push_(token_e tok) {
  stream_.push(token_t{tok, line_no_});
}

void lexer_t::push_ident_(const char *start, const char *end) {
  stream_.push(token_t(TOK_IDENT, std::string(start, end), line_no_));
}

void lexer_t::push_string_(const char *start, const char *end) {
  stream_.push(token_t(TOK_STRING, std::string(start, end), line_no_));
}

void lexer_t::push_val_(const char *s, const char *e) {
  int32_t v = 0;
  int32_t f = 0;
  for (; s != e; ++s) {
    if (*s == '.') {
      assert(f == 0);
      f = 1;
    }
    else {
      assert(is_numeric(*s));
      v = v * 10 + (*s - '0');
      f *= 10;
    }
  }
  if (f == 0) {
    stream_.push(token_t(v, line_no_));
  }
  else {
    const float x = float(v) / float(f);
    stream_.push(token_t(x, line_no_));
  }
}

void lexer_t::reset() {
  stream_.reset();
}
