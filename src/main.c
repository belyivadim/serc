#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#include "parser.h"
#include "code_gen.h"
#include "./lib/ds/logger.h"
#include "./lib/ds/vec.h"
#include "./lib/ds/string_builder.h"
#include "./lib/ds/allocator.h"

#define _GNU_SOURCE

LogSeverity g_log_severity = LOG_ALL;


char *get_file_content(const char *path) {
  FILE *f = fopen(path, "r");
  if (NULL == f) return NULL;

  char *content = NULL;
  long size;
  if (-1 == fseek(f, 0, SEEK_END) || -1 == (size = ftell(f))) {
    goto cleanup_error;
  }

  content = (char*)a_allocate(size + 1);
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
  a_free(content);
  content = NULL;
cleanup:
  fclose(f);
  return content;
}


int main(int argc, char **argv) {
  if (argc < 2) {
    log_error("MAIN", "please provide *.c file");
    return 1;
  }

  allocator_init(1024 * 1024 * 1024);  // 1 GiB

  char *content = get_file_content(argv[1]);
  if (NULL == content) {
    logf_error("PARSER", "error reading file %s: %s\n", argv[1], strerror(errno));
    return 1;
  }

  vec(StructInfo) structs;
  vec_alloc(structs);

  bool ok = parse(content, &structs);
  if (!ok) {
    goto cleanup;
  }

  ok = generate_json_serialization((const vec(StructInfo) const *)&structs, NULL);
  if (!ok) {
    goto cleanup;
  }


cleanup:
  for (size_t i = 0; i < vec_count(structs); ++i) {
    struct_info_free(vec_at(structs, i));
  }
  vec_free(structs);
  a_free(content);

  allocator_finalize();

  return !ok;
}

bool cstr_ends_with(const char *str, const char *suffix) {
    size_t strLen = strlen(str);
    size_t suffixLen = strlen(suffix);

    if (strLen < suffixLen) {
        return false;
    }

    return strncmp(str + strLen - suffixLen, suffix, suffixLen) == 0;
}

bool get_dir_files(const char *dir_path, const char *extension, vec(StringBuilder) *out_paths) {
  DIR *dir = opendir(dir_path);
  if (!dir) {
    return false;
  }

  struct dirent *entry;
  while (NULL != (entry = readdir(dir))) {
    if (entry->d_type == DT_DIR
      && (0 != memcmp(entry->d_name, ".", 2) && 0 != memcmp(entry->d_name, "..", 3))) {
      StringBuilder path; string_builder_init(path);
      string_builder_append_cstr(&path, dir_path);
      string_builder_append_rune(&path, '/');
      string_builder_append_cstr(&path, entry->d_name);

      if (!get_dir_files(string_builder_get_cstr(&path), extension, out_paths)) {
        string_builder_free(path);
        return false;
      }

      string_builder_free(path);
    } else if (entry->d_type == DT_REG) {
      if (NULL != extension && !cstr_ends_with(entry->d_name, extension)) {
        continue;
      }

      StringBuilder path; string_builder_init(path);
      string_builder_append_cstr(&path, dir_path);
      string_builder_append_rune(&path, '/');
      string_builder_append_cstr(&path, entry->d_name);
      vec_push(*out_paths, path);
    } else if (entry->d_type == DT_LNK) {

      StringBuilder path; string_builder_init(path);
      string_builder_append_cstr(&path, dir_path);
      string_builder_append_rune(&path, '/');
      string_builder_append_cstr(&path, entry->d_name);

      struct stat linkTargetStat;
      if (stat(string_builder_get_cstr(&path), &linkTargetStat) == -1) {
        string_builder_free(path);
        return false;
      }

      if (S_ISREG(linkTargetStat.st_mode)) {
        if (NULL == extension || cstr_ends_with(entry->d_name, extension)) {
          vec_push(*out_paths, path);
        }
      } else if (S_ISDIR(linkTargetStat.st_mode)) {
        if (!get_dir_files(string_builder_get_cstr(&path), extension, out_paths)) {
          string_builder_free(path);
          return false;
        }

        string_builder_free(path);
      } 
    }
  }

  closedir(dir);
  return true;
}

int main2(int argc, char **argv) {
  if (argc < 2) {
    log_error("MAIN", "please provide *.c file or directory with *.c files");
    return 1;
  }

  allocator_init(1024 * 1024 * 1024);  // 1 GiB

  vec(StringBuilder) paths; vec_alloc(paths);
  const char *extension = ".c";
  bool ok = get_dir_files(argv[1], extension, &paths);

  if (!ok) {
    logf_error("MAIN", "get_dir_files failed: %s\n", strerror(errno));
  }

  size_t paths_count = vec_count(paths);
  logf_trace("MAIN", "%s consist of (filtered by extension '%s'):\n", argv[1], extension);
  for (size_t i = 0; i < paths_count; ++i) {
    logf_trace("MAIN", "\t\t%s\n", string_builder_get_cstr(vec_at(paths, i)));
    string_builder_free(paths[i]);
  }

  vec_free(paths);
  //bool result = parse(argv[1]);

  allocator_finalize();
  return  !ok;
}
