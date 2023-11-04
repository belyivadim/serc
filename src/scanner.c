#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"
#include "./lib/ds/logger.h"

/// Represent the source code scanner
typedef struct {
  /// Pointer to the beginning of the token that is currently being parsed
  const char *start;

  /// Pointer to the current location in source code 
  const char *current;

  /// Number of the current line in source file
  i32 line;
} Scanner;

/// scanner singleton
Scanner scanner;

/// Checks if current position is at the end of the sequence ('\0')
///
/// @return bool, true if at the end
static bool is_at_end();

/// Advances current position by one
///
/// @return char, character at the position before advance
static char advance();

/// Creates a Token of passed kind with lexeme starting 
/// at the current position of the scanner 
/// and length is difference between current and start positions
///
/// @param kind: kind of the Token to be created
/// @return Token, newly created Token
static Token token_create(TokenKind kind);

/// Advances current position while it points to the whitespace symbol,
/// gracefully handles newline characters.
/// Also skips comments
///
/// @return Token: token of kind TOK_EMPTY or TOK_ANNOTATION
static Token skip_whitespace();

/// Peeks current character in the scanner
///
/// @return char
static char peek();

/// Peeks the next to current character, if current is the end of file,
/// function will return '\0'
///
/// @return char
static char peek_next();

/// Checks if the param char c is a digit
///
/// @param c: character to be checked
/// @return bool, true if it is
static bool is_digit(char c);

/// Checks if the param char c is an alpha or '_'
///
/// @param c: character to be checked
/// @return bool, true if it is
static bool is_alpha(char c);

/// Process identifier and return token with kind TOK_IDENTIFIER
/// if it is user defined identifier
/// or TOK_<keyword> if it is a keyword
///
/// @return Token
static Token process_identifier();

/// Determines kind of the identifier that is being parsed
///
/// @return TokenKind
static TokenKind identifier_kind();

/// Checks if the rest of the word pointed by scanner.current + start param
/// with the length of length param is equal to the rest param
/// if it is, return kind param,
/// otherwise return TOK_IDENTIFIER
///
/// @param start: offset to the beginning of the rest of word pointed by
///   scanner.current
/// @param length: length of the rest of word
/// @param rest: pointer to the rest of the keyword to compare with
/// @param kind: kind of token to return if rest of the word is matched
///   with the rest param
/// @return TokenKind, determined kind of token
static TokenKind check_keyword(i32 start, i32 length,
                              const char *rest, TokenKind kind);


void scanner_init(const char *source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

Token scan_token() {
  Token annotation = skip_whitespace();
  if (annotation.kind == TOK_ANNOTATION) {
    return annotation;
  }

  scanner.start = scanner.current;

  if (is_at_end()) return token_create(TOK_EOF);

  char c = advance();

  if (is_alpha(c)) return process_identifier();

  switch (c) {
    case '(': return token_create(TOK_LEFT_PAREN);
    case ')': return token_create(TOK_RIGHT_PAREN);
    case '{': return token_create(TOK_LEFT_BRACE);
    case '}': return token_create(TOK_RIGHT_BRACE);
    case ';': return token_create(TOK_SEMICOLON);
    case ',': return token_create(TOK_COMMA);
    case '*': return token_create(TOK_STAR);

    default: return token_create(TOK_OTHER);
  }
}


static bool is_at_end() {
  return *scanner.current == '\0';
}

static char advance() {
  return *scanner.current++;
}

static Token token_create(TokenKind kind) {
  return (Token) {
    .kind = kind,
    .lexeme = string_view_from_cstr_slice(scanner.start, 0, scanner.current - scanner.start),
    .line = scanner.line
  };
}

static Token skip_whitespace() {
  Token annotation = {.kind = TOK_EMPTY};
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;

      case '\n':
        ++scanner.line;
        advance();
        break;

      case '/':
        if (peek_next() == '/') {
          while (peek() != '\n' && !is_at_end()) {
            if ('`' == peek() && TOK_EMPTY == annotation.kind) {
              advance(); // `
              annotation.kind = TOK_ANNOTATION;
              annotation.line = scanner.line;
              const char *ann_begin_str = scanner.current;
              do {
                advance();
              } while ('`' != peek() && '\n' != peek() && !is_at_end());
              if ('`' != peek()) {
                logf_fatal("SCANNER", 1, "Missing closing '`' for annotation at the line %d\n", scanner.line);
              }
              annotation.lexeme = string_view_from_cstr_slice(ann_begin_str, 0, scanner.current - ann_begin_str);
            }
            advance();
          } 
        } else if (peek_next() == '*') {
          do { 
            advance(); 
            scanner.line += peek() == '\n';
          } while (!is_at_end() && !(peek() == '*' && peek_next() == '/'));
          if (!is_at_end()) 
            {
              advance();  // '*'
              advance();  // '/'
            }
        } else {
          return annotation;
        }
        break;

      default:
        return annotation;
    }
  }
}

static char peek() {
  return *scanner.current;
}

static char peek_next() {
  if (is_at_end()) return '\0';
  return scanner.current[1];
}

static bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') 
      || (c >= 'A' && c <= 'Z')
      || c == '_';
}

static Token process_identifier() {
  while (is_alpha(peek()) || is_digit(peek())) advance();

  return token_create(identifier_kind());
}

static TokenKind identifier_kind() {
  // void, char, short, int, long, unsigned, float, double
  // struct, const, typedef
  //
  // char, const, double, float, int, long, short, struct, typedef, void, unsigned
  switch (*scanner.start) {
    case 'c': 
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'h': return check_keyword(2, 2, "ar", TOK_CHAR);
          case 'o': return check_keyword(2, 3, "nst", TOK_CONST);
        }
      }
      break;

    case 'd': return check_keyword(1, 5, "ouble", TOK_DOUBLE);
    case 'f': return check_keyword(1, 4, "loat", TOK_FLOAT);
    case 'i': return check_keyword(1, 2, "nt", TOK_INT);
    case 'l': return check_keyword(1, 3, "ong", TOK_LONG);

    case 's': 
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'h': return check_keyword(2, 3, "ort", TOK_SHORT);
          case 't': return check_keyword(2, 4, "ruct", TOK_STRUCT);
        }
      }
      break;

    case 't': return check_keyword(1, 6, "ypedef", TOK_TYPEDEF);
    case 'v': return check_keyword(1, 3, "oid", TOK_VOID);
    case 'u': return check_keyword(1, 7, "nsigned", TOK_UNSIGNED);
  }

  return TOK_IDENTIFIER;
}

static TokenKind check_keyword(i32 start, i32 length,
                              const char *rest, TokenKind kind) {
  if (scanner.current - scanner.start == start + length
    && 0 == memcmp(scanner.start + start, rest, length)) {
    return kind;
  }

  return TOK_IDENTIFIER;
}

#ifdef SCANNER_MAIN

#include <stdlib.h>
char *get_file_content(const char *path) {
  FILE *f = fopen(path, "r");
  if (NULL == f) return NULL;

  char *content = NULL;
  long size;
  if (-1 == fseek(f, 0, SEEK_END) || -1 == (size = ftell(f))) {
    goto cleanup_error;
  }

  content = (char*)malloc(size + 1);
  if (NULL == content) {
    goto cleanup_error;
  }

  rewind(f);

  size_t bytes = 0;
  while (bytes != (size_t)size) {
    size_t read_bytes = fread(content + bytes, 1, size - bytes, f);
    if (0 == read_bytes) {
      goto cleanup_error;
    }
    bytes += read_bytes;
  }
  content[size] = '\0';

  goto cleanup;

cleanup_error:
  free(content);
  content = NULL;
cleanup:
  fclose(f);
  return content;
}


#include "./lib/ds/logger.h"
LogSeverity g_log_severity = LOG_ALL;



#include <sys/ioctl.h>
#include <unistd.h>

void get_cursor_position(int *x, int *y) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        if (x != NULL) *x = ws.ws_col;
        if (y != NULL) *y = ws.ws_row;
    }
}

void gotox(int x) {
    int current_y;
    get_cursor_position(&current_y, NULL);
    printf("\033[%d;%dH", current_y, x);
}

ssize_t getline(char **restrict lineptr, size_t *restrict n,
                FILE *restrict stream);

int main(int argc, char **argv)
{
  char *content = NULL;
  if (argc > 1)
    content = get_file_content(argv[1]);
  else {
    size_t content_size = 0;
    getline(&content, &content_size, stdin);
  }

  if (NULL == content) {
    return 1;
  }

  scanner_init(content);

  Token t;
  do {
    t = scan_token();
    printf("kind: %s", token_kind_to_cstr(t.kind));
    gotox(33);
    printf("line: %d", t.line);
    gotox(49);
    printf("'" string_view_farg "'\n", string_view_expand(t.lexeme));
    // printf("kind: %s,\t\tline: %d,\t\t\"" string_view_farg "\"\n", 
    //        token_kind_to_cstr(t.kind), t.line, string_view_expand(t.lexeme));
  } while (t.kind != TOK_EOF);

  free(content);
  return 0;
}

#endif // !SCANNER_MAIN
