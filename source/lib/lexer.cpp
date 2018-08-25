#include "lexer.h"
#include "errors.h"


namespace {

bool munch(const char *&s, const char *string) {
  const char *t = s;
  for (;; t++, string++) {
    if (*string == '\0') {
      s = t - 1;
      return true;
    }
    if (tolower(*t) != tolower(*string)) {
      return false;
    }
  }
}

bool is_alpha_numeric(const char t) {
  return (t >= 'a' && t <= 'z') ||
         (t >= 'A' && t <= 'Z') ||
         (t >= '0' && t <= '9') ||
         (t == '_');
}

bool is_numeric(const char t) {
  return (t >= '0' && t <= '9');
}

} // namespace

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
      push(TOK_EOL);
      continue;
    }

    // parse keywords
    switch (tolower(*s)) {
    case ('a'):
      if (munch(s, "and")) {
        push(TOK_AND);
        continue;
      }
      break;
    case ('e'):
      if (munch(s, "end")) {
        push(TOK_END);
        continue;
      } else if (munch(s, "else")) {
        push(TOK_ELSE);
        continue;
      }
      break;
    case ('f'):
      if (munch(s, "function")) {
        push(TOK_FUNC);
        continue;
      }
      break;
    case ('i'):
      if (munch(s, "if")) {
        push(TOK_IF);
        continue;
      }
      break;
    case ('n'):
      if (munch(s, "not")) {
        push(TOK_NOT);
        continue;
      }
      break;
    case ('o'):
      if (munch(s, "or")) {
        push(TOK_OR);
        continue;
      }
      break;
    case ('r'):
      if (munch(s, "return")) {
        push(TOK_RETURN);
        continue;
      }
      break;
    case ('v'):
      if (munch(s, "var")) {
        push(TOK_VAR);
        continue;
      }
      break;
    case ('w'):
      if (munch(s, "while")) {
        push(TOK_WHILE);
        continue;
      }
      break;
    case ('('):
      push(TOK_LPAREN);
      continue;
    case (')'):
      push(TOK_RPAREN);
      continue;
    case (','):
      push(TOK_COMMA);
      continue;
    case ('+'):
      push(TOK_ADD);
      continue;
    case ('-'):
      push(TOK_SUB);
      continue;
    case ('*'):
      push(TOK_MUL);
      continue;
    case ('/'):
      push(TOK_DIV);
      continue;
    case ('%'):
      push(TOK_MOD);
      continue;
    case ('\n'):
    {
      assert(s >= new_line_);
      lines_.push_back(std::string(new_line_, s - new_line_));
      new_line_ = s + 1;
      ++line_no_;
    }
      push(TOK_EOL);
      continue;
    case ('='):
      if (s[1] == '=') {
        push(TOK_EQ);
        ++s;
        continue;
      } else {
        push(TOK_ASSIGN);
        continue;
      }
      break;
    case ('<'):
      if (s[1] == '=') {
        push(TOK_LEQ);
        ++s;
        continue;
      } else {
        push(TOK_LT);
        continue;
      }
      break;
    case ('>'):
      if (s[1] == '=') {
        push(TOK_GEQ);
        ++s;
        continue;
      } else {
        push(TOK_GT);
        continue;
      }
      break;
    } // switch

    // try to parse number
    {
      const char *t = s;
      for (; is_numeric(*t); ++t) {
        // empty
      }
      if (t != s) {
        push_val(s, t);
        s = t - 1; // (-1 because of ++ in forloop)
        continue;
      }
    }

    // try to parse identifier
    {
      const char *t = s;
      for (; is_alpha_numeric(*t); ++t) {
        // empty
      }
      if (t != s) {
        push_ident(s, t);
        s = t - 1; // (-1 because of ++ in forloop)
        continue;
      }
    }

    // raise an error
    ccml_.errors().unexpected_character(line_no_, *s);

  } // while
  stream_.push(token_t{TOK_EOF, line_no_});
  return true;
}

void lexer_t::push(token_e tok) {
  stream_.push(token_t{tok, line_no_});
}

void lexer_t::push_ident(const char *start, const char *end) {
  stream_.push(token_t(std::string(start, end), line_no_));
}

void lexer_t::push_val(const char *s, const char *e) {
  int32_t v = 0;
  for (; s != e; ++s) {
    assert(is_numeric(*s));
    v = v * 10 + (*s - '0');
  }
  stream_.push(token_t(v, line_no_));
}

void lexer_t::reset() {
  stream_.reset();
}
