#include <errno.h>

#include "scanner.h"
#include "./lib/ds/logger.h"

#include "parser.h"


Parser parser;

static void advance() {
  parser.previous = parser.current;
  parser.current = scan_token();
}

static bool check(TokenKind kind) {
  return kind == parser.current.kind;
}

static bool match(TokenKind kind) {
  if (!check(kind)) return false;
  advance();
  return true;
}

#define should_match(_kind)\
  do {\
    if (!match((_kind))) {\
      logf_error("PARSER", "Expected token kind %s, but got %s\n", \
                 token_kind_to_cstr((_kind)), token_kind_to_cstr(parser.current.kind));\
      return false;\
    }\
  } while (0);

#define should_be(_kind)\
  do {\
    if (!check((_kind))) {\
      logf_error("PARSER", "Expected token kind %s, but got %s\n", \
                 token_kind_to_cstr((_kind)), token_kind_to_cstr(parser.current.kind));\
      return false;\
    }\
  } while (0);


static void error_at(const Token *token, const char *message) {
  StringView at = TOK_EOF == token->kind 
                              ? string_view_from_cstr("end")
                              : token->lexeme;

  logf_error("PARSER", "[line %d Error] at " string_view_farg ": %s\n", 
             token->line, string_view_expand(at), message);
}

static void error_at_current(const char *message) {
  error_at(&parser.current, message);
}

#define consume(token_kind, message)\
  do {\
    if (parser.current.kind == (token_kind)) {\
      advance();\
    } else {\
      error_at_current(message);\
      return false;\
    }\
  } while (0)


static bool process_annotation(const Token *annotation, AnnotationInfo *p_info) {
  assert(NULL != p_info);

  size_t len = annotation->lexeme.length;

  for (size_t i = 0; i < len;) {
    char c = annotation->lexeme.p_begin[i];
    switch (c) {
      case ' ':
      case '\t':
        ++i;
        break;

      case '@': {
        ++i;
        const char *word_begin = annotation->lexeme.p_begin + i;
        while (i < len && c != ' ' && c != '\t') {
          c = annotation->lexeme.p_begin[i];
          ++i;
        }
        StringView word = 
          string_view_from_cstr_slice(word_begin, 0, annotation->lexeme.p_begin + i - word_begin - (i < len));

        if (string_view_equals(&word, &string_view_from_cstr("array"))) {
          p_info->kind = ANN_ARRAY;
        } else if (ANN_ARRAY == p_info->kind && string_view_equals(&word, &string_view_from_cstr("size"))) {

          c = annotation->lexeme.p_begin[i];
          while (i < len && (c == ' ' || c == '\t')) {
            c = annotation->lexeme.p_begin[i];
            ++i;
          }

          const char *size_field_name_begin = annotation->lexeme.p_begin + i;
          c = annotation->lexeme.p_begin[i];
          while (i < len && c != ' ' && c != '\t') {
            c = annotation->lexeme.p_begin[i];
            ++i;
          }

          if (size_field_name_begin == annotation->lexeme.p_begin + i) {
            error_at(annotation, "name of @size field should be non empty");
            return false;
          }

          p_info->as.annotation_array.array_size_field_name = 
            string_view_from_cstr_slice(size_field_name_begin, 0, annotation->lexeme.p_begin + i - size_field_name_begin - (i < len));

        } else if (string_view_equals(&word, &string_view_from_cstr("omit"))) {
          p_info->kind = ANN_OMIT;
        } else if (string_view_equals(&word, &string_view_from_cstr("callback"))) {
          p_info->kind = ANN_CUSTOM_CALLBACK;
        } else if (ANN_CUSTOM_CALLBACK == p_info->kind && string_view_equals(&word, &string_view_from_cstr("s"))) {
          c = annotation->lexeme.p_begin[i];
          while (i < len && (c == ' ' || c == '\t')) {
            c = annotation->lexeme.p_begin[i];
            ++i;
          }

          const char *cb_ser_name = annotation->lexeme.p_begin + i;
          c = annotation->lexeme.p_begin[i];
          while (i < len && c != ' ' && c != '\t') {
            c = annotation->lexeme.p_begin[i];
            ++i;
          }

          if (cb_ser_name == annotation->lexeme.p_begin + i) {
            error_at(annotation, "name of @s (serialization function) should be non empty");
            return false;
          }

          p_info->as.annotation_custom_callback.cb_ser_name = 
            string_view_from_cstr_slice(cb_ser_name , 0, annotation->lexeme.p_begin + i - cb_ser_name - (i < len));

        } else if (ANN_CUSTOM_CALLBACK == p_info->kind && string_view_equals(&word, &string_view_from_cstr("d"))) {
          c = annotation->lexeme.p_begin[i];
          while (i < len && (c == ' ' || c == '\t')) {
            c = annotation->lexeme.p_begin[i];
            ++i;
          }

          const char *cb_deser_name = annotation->lexeme.p_begin + i;
          c = annotation->lexeme.p_begin[i];
          while (i < len && c != ' ' && c != '\t') {
            c = annotation->lexeme.p_begin[i];
            ++i;
          }

          if (cb_deser_name == annotation->lexeme.p_begin + i) {
            error_at(annotation, "name of @d (deserialization function) should be non empty");
            return false;
          }

          p_info->as.annotation_custom_callback.cb_deser_name = 
            string_view_from_cstr_slice(cb_deser_name , 0, annotation->lexeme.p_begin + i - cb_deser_name - (i < len));

        } else {
          error_at(annotation, "unknown annotation keyword after '@'");
          return false;
        }
      }
    }
  }

  // TODO: ann_info validation
  return true;
}



static bool validate_type_info(TypeInfo *p_info) {
#define NOT_VALID(err_msg) do {error_at_current((err_msg)); return false;} while(0)

  assert(NULL != p_info);


  if (p_info->base_type == TYPE_UNINITIALIZED) {
    NOT_VALID("Type specifier missing.");
  }

  if ((p_info->base_type == TYPE_INT && p_info->longness > 2)
    || (p_info->base_type == TYPE_DOUBLE && p_info->longness > 1)
    || (p_info->base_type != TYPE_INT && p_info->base_type != TYPE_DOUBLE && p_info->longness > 0)) {
    NOT_VALID("Longness of the type has been exceeded.");
  }

  if (p_info->base_type == TYPE_VOID) {
    if (p_info->pointer_info.indirections_count <= 0) {
      NOT_VALID("Incomplete type `void`.");
    }
  }

  return true;

#undef NOT_VALID
}

static bool parse_type_info(TypeInfo *p_info) {
  assert(NULL != p_info);

#define SET_BASE_TYPE(t) \
  do {\
    if (p_info->base_type != TYPE_UNINITIALIZED) {\
      error_at_current("cannot combine with previous declaration");\
      return false;\
    }\
    p_info->base_type = (t);\
  } while (0)


  while (!check(TOK_EOF)) {
    switch (parser.current.kind) {
      case TOK_CHAR: SET_BASE_TYPE(TYPE_CHAR); break;
      case TOK_SHORT: SET_BASE_TYPE(TYPE_SHORT); break;
      case TOK_INT: SET_BASE_TYPE(TYPE_INT); break;
      case TOK_DOUBLE: SET_BASE_TYPE(TYPE_DOUBLE); break;
      case TOK_FLOAT: SET_BASE_TYPE(TYPE_FLOAT); break;
      case TOK_VOID: SET_BASE_TYPE(TYPE_VOID); break; 

      case TOK_STRUCT: {
        SET_BASE_TYPE(TYPE_STRUCT); 
        advance();
        should_be(TOK_IDENTIFIER);
        p_info->struct_name = parser.current.lexeme;
        break; 
      }

      case TOK_LONG: {
        ++p_info->longness; 
        break;
      }
      case TOK_UNSIGNED: {
        p_info->is_unsigned = true; 
        break;
      }

      case TOK_CONST: {
        if (parser.previous.kind == TOK_STAR) {
          p_info->pointer_info.is_const[p_info->pointer_info.indirections_count - 1] = true;
          break;
        }

        p_info->is_const = true;
        break;
      }

      case TOK_STAR: {
        if (p_info->pointer_info.indirections_count++ == MAX_INDERECTION_LEVEL) {
          error_at_current("maximum level of indirection has been exceeded");
          return false;
        }
        break;
      }


      case TOK_IDENTIFIER: {
        if (parser.current.lexeme.length >= 1024) {
          error_at_current("Type name is too long.");
          return false;
        }

        char buff[1024] = {0};
        strncpy(buff, parser.current.lexeme.p_begin, parser.current.lexeme.length);

        TypeInfo *tmp = NULL;
        if (table_get(&parser.typedefs, buff, (void**)&tmp)) {
          *p_info = *tmp;
          break; // continue parsing the rest of the type
        }

        if (TYPE_UNINITIALIZED == p_info->base_type && p_info->longness > 0) p_info->base_type = TYPE_INT;
        return validate_type_info(p_info);
      }

      case TOK_ANNOTATION: break; // probably commented out annotation

      default:
        error_at_current("expect type");
        return false;
    }

    advance();
  }

  error_at_current("unexpeted end of file");
  return false;

#undef SET_BASE_TYPE
}

static bool handle_struct_body(StructInfo *out) {
  assert(NULL != out);

  unsigned int offset = 0;

  while (!check(TOK_EOF) && !check(TOK_RIGHT_BRACE)) {
    VarInfo var_info = {0};
    if (!parse_type_info(&var_info.type_info)) {
      return false;
    }

    var_info.name = parser.current.lexeme;
    var_info.offset = offset;
    offset += type_info_get_size(&var_info.type_info);

    advance();
    consume(TOK_SEMICOLON, "expect ';'");

    if (check(TOK_ANNOTATION)) {
      logf_trace("PARSER", "annotation %.*s\n", string_view_expand(parser.current.lexeme));
      process_annotation(&parser.current, &var_info.type_info.ann_info);
      advance();
    }

    vec_push(out->fields, var_info);

    // {
    //   // logging
    //   StringView base_type =
    //     TYPE_STRUCT == var_info.type_info.base_type
    //     ? var_info.type_info.struct_name 
    //     : string_view_from_cstr(base_type_to_cstr(var_info.type_info.base_type));

    //   logf_trace("PARSER", 
    //              "symbol " string_view_farg " with offset %d "
    //              " %shas base type " string_view_farg ", %slongness %d%s\n",
    //              string_view_expand(parser.current.lexeme), var_info.offset,
    //              var_info.type_info.is_const ? "is const, " : "",
    //              string_view_expand(base_type),
    //              var_info.type_info.pointer_info.indirections_count > 0 ? "is pointer, " : "",
    //              var_info.type_info.longness,
    //              var_info.type_info.is_unsigned ? " ,is unsigned" : "");

    //   for (unsigned int i = 0; i < var_info.type_info.pointer_info.indirections_count; ++i) {
    //     logf_trace("PARSER", "\t indirection %u is %s\n", 
    //                i + 1, var_info.type_info.pointer_info.is_const[i] ? "const" : "nonconst");
    //   }
    // }
  }

  consume(TOK_RIGHT_BRACE, "expect '}'");

  return true;
}

static bool handle_struct_definition(vec(StructInfo) *out) {
  assert(NULL != out);
  advance();
  if (match(TOK_EOF)) {
    error_at_current("unexpeted end of file");
    return false;
  }

  StringView struct_name = string_view_from_cstr("<anonymous>");

  if (!match(TOK_IDENTIFIER)) {
    should_match(TOK_LEFT_BRACE);
  } else {
    struct_name = parser.previous.lexeme;
    if (!match(TOK_LEFT_BRACE)) {
      return true; // not a struct definition
    }
  }

  StructInfo struct_info;
  struct_info_init(struct_info);
  logf_trace("PARSER", "====struct " string_view_farg " begin====\n", string_view_expand(struct_name));
  bool ok = handle_struct_body(&struct_info);
  // logf_trace("PARSER", "====struct " string_view_farg " end====\n", string_view_expand(struct_name));

  if (!ok) {
    struct_info_free(&struct_info);
    return false;
  }

  struct_info.name = struct_name;
  vec_push(*out, struct_info);
  return true;
}

static bool parse_iteration(vec(StructInfo) *out) {
  switch (parser.current.kind) {
    case TOK_STRUCT: {
      size_t out_count = vec_count(*out);
      if (!handle_struct_definition(out)) {
        return false;
      } 
      if (out_count < vec_count(*out)) { // if it was a struct definition
        consume(TOK_SEMICOLON, "expect ';' after struct definition");
      }
      break;
    }

    case TOK_TYPEDEF: {
      advance();
      TypeInfo *p_ti = a_callocate(1, sizeof(TypeInfo));

      if (TOK_STRUCT == parser.current.kind) {
        if (!handle_struct_definition(out)) {
          a_free(p_ti);
          return false;
        } 

        StructInfo *p_si = vec_back(*out);
        p_ti->struct_name = p_si->name;
        p_ti->base_type = TYPE_STRUCT;
      } else {
        if (!parse_type_info(p_ti)) {
          a_free(p_ti);
          return false;
        }
      }

      char *key = a_callocate(parser.current.lexeme.length, sizeof(char));
      strncpy(key, parser.current.lexeme.p_begin, parser.current.lexeme.length);
      if (!table_set(&parser.typedefs, key, p_ti)) {
        // TODO:
        log_error("PARSER", "Current implementation does not support not unique typedefs even in different source files.");
        a_free(p_ti);
        a_free(key);
        return false;
      }

      advance();
      consume(TOK_SEMICOLON, "expect ';' after typedef");
      break;
    }

    default: advance();
  }

  return true;
}

static bool typedefs_key_cmp(const char *lhs, const char *rhs) {
  return 0 == strcmp(lhs, rhs);
}

static void typedefs_free_cb(char *key, TypeInfo *value) {
  a_free(key);
  a_free(value);
}

bool parse(const char *source, vec(StructInfo) *out) {
  // TODO: parser initialization and destruction
  bool ret = true;
  scanner_init(source);
  table_init(&parser.typedefs, hash_cstr_default, 
             (KeyCmpFunc)typedefs_key_cmp, (FreeKeyValFunc)typedefs_free_cb);
  advance();

  do {
    if (!parse_iteration(out)) {
      ret = false;
      break;
    }
  } while (parser.current.kind != TOK_EOF);

  table_free(&parser.typedefs);
  return ret;
}

const char* base_type_to_cstr(BaseType t) {
  switch (t) {
    case TYPE_UNINITIALIZED: return "TYPE_UNINITIALIZED";
    case TYPE_CHAR: return "TYPE_CHAR";
    case TYPE_SHORT: return "TYPE_SHORT";
    case TYPE_INT: return "TYPE_INT";
    case TYPE_LONG: return "TYPE_LONG";
    case TYPE_FLOAT: return "TYPE_FLOAT";
    case TYPE_DOUBLE: return "TYPE_DOUBLE";
    case TYPE_VOID: return "TYPE_VOID";
    case TYPE_STRUCT: return "TYPE_STRUCT";
    default: return "<unknown base type>";
  }
}


size_t type_info_get_size(const TypeInfo *p_ti) {
  assert(NULL != p_ti);
  assert(TYPE_UNINITIALIZED != p_ti->base_type);

  if (p_ti->pointer_info.indirections_count > 0) return sizeof(void*);

  if (TYPE_STRUCT == p_ti->base_type) return 0;

  if (0 == p_ti->longness) {
    switch (p_ti->base_type) {
      case TYPE_CHAR: return sizeof(char);
      case TYPE_SHORT: return sizeof(short);
      case TYPE_INT: return sizeof(int);
      case TYPE_FLOAT: return sizeof(float);
      case TYPE_DOUBLE: return sizeof(double);

      default: break;
    }
  }

  return 0;
}
