#ifndef __SERC_TOKEN_H__
#define __SERC_TOKEN_H__

#include "common.h"
#include "./lib/ds/string_view.h"

typedef enum {
  // Single-character tokens.
  TOK_LEFT_PAREN, TOK_RIGHT_PAREN,
  TOK_LEFT_BRACE, TOK_RIGHT_BRACE,
  TOK_LEFT_BRACKET, TOK_RIGHT_BRACKET,
  TOK_COMMA, TOK_SEMICOLON, TOK_STAR,

  // Literals.
  TOK_IDENTIFIER,

  // Keywords.
  TOK_CHAR, TOK_CONST, TOK_DOUBLE, TOK_FLOAT,
  TOK_LONG, TOK_INT, TOK_SHORT, TOK_STRUCT,
  TOK_TYPEDEF, TOK_VOID, TOK_UNSIGNED,

  // Specials.
  TOK_EMPTY,
  TOK_ANNOTATION,
  TOK_OTHER,
  TOK_EOF

} TokenKind;

/// Represents the Token of source code lexeme
typedef struct {
  /// Kind of the Token
  TokenKind kind;

  StringView lexeme;

  /// Number of the line, on which lexeme represented by this token
  /// is located
  i32 line;
} Token;


const char *token_kind_to_cstr(TokenKind kind);

#endif // !__SERC_TOKEN_H__
