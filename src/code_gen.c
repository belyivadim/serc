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
    case TYPE_CHAR: string_builder_append_cstr(&sb, "char_"); break;
    case TYPE_SHORT: string_builder_append_cstr(&sb, "short_"); break;
    case TYPE_INT: string_builder_append_cstr(&sb, "int_"); break;
    case TYPE_FLOAT: string_builder_append_cstr(&sb, "float_"); break;
    case TYPE_DOUBLE: string_builder_append_cstr(&sb, "double_"); break;
    default: 
      //assert(false && "Not reachable"); 
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


  // serialize function
  fprintf(out_c, "bool serializer_%.*s_to_json(Serializer *p_ser, const void *p_val) {\n",
          string_view_expand(p_si->name));
  {
    fputs("\tassert(NULL != p_val);\n\n", out_c);

    // cast void* to struct*
    fprintf(out_c, "\tconst " string_view_farg " *tmp = (const " string_view_farg "*)p_val;\n\n",
            string_view_expand(p_si->name), string_view_expand(p_si->name));

    // serialize fields
    vec_for_each(p_si->fields, i, {
                  StringBuilder type = type_info_get_ser_func_primitive_type(&p_si->fields[i].type_info);
                  fprintf(out_c, "\tSER_VALIDATE(serializer_json_start_field(p_ser, \"%.*s\"));\n", string_view_expand(p_si->fields[i].name));
                  fprintf(out_c, "\tSER_VALIDATE(serializer_%sto_json(p_ser, tmp->%.*s));\n", 
                          string_builder_get_cstr(&type), string_view_expand(p_si->fields[i].name));
                  fprintf(out_c, "\tSER_VALIDATE(serializer_json_end_field(p_ser));\n\n");
                  string_builder_free(type);
                 });

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
