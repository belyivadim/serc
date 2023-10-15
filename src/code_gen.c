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

  return true;
}

bool finalize_out_files_json(FILE *out_h, FILE *out_c) {
  assert(NULL != out_h);
  assert(NULL != out_c);

  fputs("#endif // !__SERC_JSON_H__\n", out_h);
  return true;
}

bool generate_json_for_struct(const StructInfo *p_si, FILE *out_h, FILE *out_c) {
  assert(NULL != p_si);
  assert(NULL != out_h);
  assert(NULL != out_c);

  fprintf(out_h, "bool serializer_%.*s_to_json(Serializer *p_ser, void *p_val);\n",
          string_view_expand(p_si->name));

  fprintf(out_c, "bool serializer_%.*s_to_json(Serializer *p_ser, void *p_val) {\n",
          string_view_expand(p_si->name));

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
