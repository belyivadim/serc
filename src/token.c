#include "token.h"

const char *token_kind_to_cstr(TokenKind kind) {
  switch (kind) {
    case TOK_LEFT_PAREN: return "TOK_LEFT_PAREN";
    case TOK_RIGHT_PAREN: return "TOK_RIGHT_PAREN";
    case TOK_LEFT_BRACE: return "TOK_LEFT_BRACE";
    case TOK_RIGHT_BRACE: return "TOK_RIGHT_BRACE";
    case TOK_LEFT_BRACKET: return "TOK_LEFT_BRACKET";
    case TOK_RIGHT_BRACKET: return "TOK_RIGHT_BRACKET";
    case TOK_COMMA: return "TOK_COMMA";
    case TOK_SEMICOLON: return "TOK_SEMICOLON";
    case TOK_STAR: return "TOK_STAR";
    case TOK_IDENTIFIER: return "TOK_IDENTIFIER";
    case TOK_CHAR: return "TOK_CHAR";
    case TOK_CONST: return "TOK_CONST";
    case TOK_DOUBLE: return "TOK_DOUBLE";
    case TOK_FLOAT: return "TOK_FLOAT";
    case TOK_LONG: return "TOK_LONG";
    case TOK_INT: return "TOK_INT";
    case TOK_SHORT: return "TOK_SHORT";
    case TOK_STRUCT: return "TOK_STRUCT";
    case TOK_TYPEDEF: return "TOK_TYPEDEF";
    case TOK_VOID: return "TOK_VOID";
    case TOK_UNSIGNED: return "TOK_UNSIGNED";
    case TOK_OTHER: return "TOK_OTHER";
    case TOK_EOF: return "TOK_EOF";
    default: return "Unknown TokenKind";
  }
}
