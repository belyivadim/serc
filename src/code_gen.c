#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "code_gen.h"
#include "./lib/ds/logger.h"
#include "./lib/ds/string_builder.h"

bool initialize_out_files_json(FILE *out_h, FILE *out_c, const char *path) {
  assert(NULL != out_h);
  assert(NULL != out_c);

  fputs("#ifndef __SERC_JSON_H__\n", out_h);
  fputs("#define __SERC_JSON_H__\n", out_h);
  fputs("#include <stdbool.h>\n", out_h);
  fputs("#include <assert.h>\n", out_c);

  if (NULL == path) path = ".";

  fprintf(out_h, "#include \"%s/primitives.h\"\n\n", path);
  fprintf(out_c, "#include \"%s/json.h\"\n\n", path);

  fputs("#define SER_VALIDATE(call) do { if (!(call)) { return false; } } while (0)\n\n", out_c);

  return true;
}

bool finalize_out_files_json(FILE *out_h, FILE *out_c) {
  assert(NULL != out_h);
  assert(NULL != out_c);

  fputs("#endif // !__SERC_JSON_H__\n", out_h);
  return true;
}

static bool is_primitive_base_type(BaseType type) {
  return TYPE_CHAR == type || TYPE_SHORT == type || TYPE_INT == type
    || TYPE_FLOAT == type || TYPE_DOUBLE == type; //|| TYPE_VOID == type;
}


static StringBuilder type_info_get_ser_func_primitive_type(const TypeInfo *p_ti) {
  assert(NULL != p_ti);
  assert(TYPE_UNINITIALIZED != p_ti->base_type);

  StringBuilder sb; string_builder_init(sb);

  if (p_ti->is_unsigned) {
    string_builder_append_cstr(&sb, "u_");
  }

  for (unsigned int i = 0; i < p_ti->longness; ++i) {
    string_builder_append_cstr(&sb, "long_");
  }

  switch (p_ti->base_type) {
    case TYPE_CHAR: string_builder_append_cstr(&sb, "char"); break;
    case TYPE_SHORT: string_builder_append_cstr(&sb, "short"); break;
    case TYPE_INT: string_builder_append_cstr(&sb, "int"); break;
    case TYPE_FLOAT: string_builder_append_cstr(&sb, "float"); break;
    case TYPE_DOUBLE: string_builder_append_cstr(&sb, "double"); break;
    default: 
      assert(false && "Not reachable"); 
      break;
  }

  return sb;
}

static StringBuilder type_info_get_type_str(const TypeInfo *p_ti) {
  assert(NULL != p_ti);
  assert(TYPE_UNINITIALIZED != p_ti->base_type);

  StringBuilder sb; string_builder_init(sb);

  if (p_ti->is_const) {
    string_builder_append_cstr(&sb, "const ");
  }

  if (TYPE_STRUCT == p_ti->base_type) {
    string_builder_append_string_view(&sb, &p_ti->struct_name);
  } else {
    if (p_ti->is_unsigned) {
      string_builder_append_cstr(&sb, "unsigned ");
    }

    for (unsigned int i = 0; i < p_ti->longness; ++i) {
      string_builder_append_cstr(&sb, "long ");
    }

    switch (p_ti->base_type) {
      case TYPE_CHAR: string_builder_append_cstr(&sb, "char"); break;
      case TYPE_SHORT: string_builder_append_cstr(&sb, "short"); break;
      case TYPE_INT: string_builder_append_cstr(&sb, "int"); break;
      case TYPE_FLOAT: string_builder_append_cstr(&sb, "float"); break;
      case TYPE_DOUBLE: string_builder_append_cstr(&sb, "double"); break;
      case TYPE_VOID: string_builder_append_cstr(&sb, "void"); break;
      default: assert(false && "Not reachable"); break;
    }
  }

  string_builder_append_rune(&sb, ' ');

  for (unsigned int i = 0; i < p_ti->pointer_info.indirections_count; ++i) {
    string_builder_append_cstr(&sb, " *");
    if (p_ti->pointer_info.is_const[i]) {
      string_builder_append_cstr(&sb, " const ");
    }
  }

  return sb;
}

static bool generate_json_for_field(const VarInfo *field, FILE *out_c) {
  assert(NULL != field);
  assert(NULL != out_c);

  if (ANN_OMIT == field->type_info.ann_info.kind) {
    return true;
  }

  if (field->type_info.base_type == TYPE_VOID && field->type_info.ann_info.kind != ANN_CUSTOM_CALLBACK) {
    logf_error("CODE_GEN", "Field %.*s has base type void and no custom callback was provided.\n", field->name);
    log_error("CODE_GEN", "NOTE: to provide custom callback write annotation after the field `@callback @s <ser func name> @d <deser func name`");
    return false;
  }

  bool is_primitive = is_primitive_base_type(field->type_info.base_type);
  const char *field_prefix_str = "";
  unsigned int number_of_ptrs = field->type_info.pointer_info.indirections_count;

  if (is_primitive) {
    switch (number_of_ptrs) {
      case 1: field_prefix_str = "*"; break;
      case 2: field_prefix_str = "**"; break;
      case 3: field_prefix_str = "***"; break;
      case 4: field_prefix_str = "****"; break;
      default: break;
    }
  } else {
    if (number_of_ptrs > 0) {
      switch (number_of_ptrs) {
        case 2: field_prefix_str = "*"; break;
        case 3: field_prefix_str = "**"; break;
        case 4: field_prefix_str = "***"; break;
        default: break;
      }
    } else {
      field_prefix_str = "&";
    }
  }

  StringBuilder type;
  if (is_primitive) {
    type = type_info_get_ser_func_primitive_type(&field->type_info);
  } else {
    string_builder_init(type);
    string_builder_append_string_view(&type, &field->type_info.struct_name);
  }

  fprintf(out_c, "\tSER_VALIDATE(serializer_json_start_field(p_ser, \"%.*s\"));\n", 
          string_view_expand(field->name));

  switch (field->type_info.ann_info.kind) {
    case ANN_ARRAY: {
      fprintf(out_c, "\tSER_VALIDATE(serializer_json_start_array(p_ser));\n");
      fprintf(out_c, "\tfor (size_t i = 0; i < tmp->%.*s; ++i) {\n", 
              string_view_expand(field->type_info.ann_info.as.annotation_array.array_size_field_name));

      fprintf(out_c, "\t\tSER_VALIDATE(serializer_%s_to_json(p_ser, %stmp->%.*s + i));\n", 
              string_builder_get_cstr(&type), field_prefix_str, string_view_expand(field->name));

      fprintf(out_c, "\t}\n");
      fprintf(out_c, "\tSER_VALIDATE(serializer_json_end_array(p_ser));\n");
      break;
    }
    case ANN_CUSTOM_CALLBACK: {
      StringView cb_ser_name = field->type_info.ann_info.as.annotation_custom_callback.cb_ser_name;
      fprintf(out_c, "\tSER_VALIDATE(%.*s(p_ser, %stmp->%.*s));\n", 
              string_view_expand(cb_ser_name), field_prefix_str, string_view_expand(field->name));

      break;
    }
    case ANN_EMPTY: {
      fprintf(out_c, "\tSER_VALIDATE(serializer_%s_to_json(p_ser, %stmp->%.*s));\n", 
              string_builder_get_cstr(&type), field_prefix_str, string_view_expand(field->name));
      break;
    }

    default:
      assert(false && "not reachable");
  }

  fprintf(out_c, "\tSER_VALIDATE(serializer_json_end_field(p_ser));\n\n");

  string_builder_free(type);
  return true;
}

bool generate_json_for_struct(const StructInfo *p_si, FILE *out_h, FILE *out_c) {
  assert(NULL != p_si);
  assert(NULL != out_h);
  assert(NULL != out_c);

  fprintf(out_h, "bool serializer_%.*s_to_json(Serializer *p_ser, const void *p_val);\n",
          string_view_expand(p_si->name));

  // defining struct for serializing
  fprintf(out_c, "typedef struct " string_view_farg " {\n",
          string_view_expand(p_si->name));
  {
    // members
    vec_for_each(p_si->fields, i, {
                  StringBuilder type = type_info_get_type_str(&p_si->fields[i].type_info);
                  fprintf(out_c, "\t%s %.*s;\n", 
                          string_builder_get_cstr(&type), string_view_expand(p_si->fields[i].name));
                  string_builder_free(type);
                 });
  }
  fprintf(out_c, "} " string_view_farg ";\n", string_view_expand(p_si->name));

  // forward decl for all custom callbacks
  vec_for_each(p_si->fields, i, {
                if (p_si->fields[i].type_info.ann_info.kind == ANN_CUSTOM_CALLBACK) {
                  AnnotationCustomCallback acc = p_si->fields[i].type_info.ann_info.as.annotation_custom_callback;
                  fprintf(out_c, "bool %.*s(Serializer *p_ser, const void *value);\n", string_view_expand(acc.cb_ser_name));
                  fprintf(out_c, "bool %.*s(Serializer *p_ser, void *value);\n", string_view_expand(acc.cb_deser_name));
                }
               });


  // serialize function
  fprintf(out_c, "bool serializer_%.*s_to_json(Serializer *p_ser, const void *p_val) {\n",
          string_view_expand(p_si->name));
  {
    fputs("\tassert(NULL != p_val);\n\n", out_c);

    // cast void* to struct*
    fprintf(out_c, "\tconst " string_view_farg " *tmp = (const " string_view_farg "*)p_val;\n\n",
            string_view_expand(p_si->name), string_view_expand(p_si->name));

    // serialize fields
    vec_for_each(p_si->fields, i, { if (!generate_json_for_field(p_si->fields + i, out_c)) return false; });

    fputs("\treturn true;\n", out_c);
  }
  fputs("}\n\n", out_c);


  return true;
}


#define SERIALIZATION_DIR "./src/serialization/"

bool generate_json_serialization(const vec(StructInfo) const * p_si, const char *path) {
  assert(NULL != p_si);
  StringBuilder dotc; string_builder_init(dotc);
  StringBuilder doth; string_builder_init(doth);
  bool ok = true;

  if (NULL != path) {
    string_builder_append_cstr(&dotc, path);
    string_builder_append_cstr(&doth, path);
  } else {
    string_builder_append_cstr(&dotc, SERIALIZATION_DIR);
    string_builder_append_cstr(&doth, SERIALIZATION_DIR);
  }

  string_builder_append_cstr(&dotc, "json.c");
  string_builder_append_cstr(&doth, "json.h");

  FILE *out_c = fopen(string_builder_get_cstr(&dotc), "w+");
  if (NULL == out_c) {
    logf_error("CODE_GEN", "Could not open file %s: %s\n", string_builder_get_cstr(&dotc), strerror(errno));
    goto cleanup_error;
  }

  FILE *out_h = fopen(string_builder_get_cstr(&doth), "w+");
  if (NULL == out_h) {
    logf_error("CODE_GEN", "Could not open file %s: %s\n", string_builder_get_cstr(&doth), strerror(errno));
    goto cleanup_error;
  }

  if (!initialize_out_files_json(out_h, out_c, path)) {
    goto cleanup_error;
  }

  for (size_t i = 0; i < vec_count(*p_si); ++i) {
    if (!generate_json_for_struct(vec_at(*p_si, i), out_h, out_c)) {
      goto cleanup_error;
    }
  }

  if (!finalize_out_files_json(out_h, out_c)) {
    goto cleanup_error;
  }


  goto cleanup;

cleanup_error:
  ok = false;
  if (NULL != out_c) remove(string_builder_get_cstr(&dotc));
  if (NULL != out_h) remove(string_builder_get_cstr(&doth));

cleanup:
  if (NULL != out_c) fclose(out_c);
  if (NULL != out_h) fclose(out_h);
  string_builder_free(dotc);
  string_builder_free(doth);

  return ok;
}
