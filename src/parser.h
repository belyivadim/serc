#ifndef __SERC_PARSER_H__
#define __SERC_PARSER_H__

#include "common.h"
#include "token.h"
#include "./lib/ds/vec.h"
#include "./lib/ds/table.h"
#include "./serialization/primitives.h"

typedef struct {
  Token current;
  Token previous;
  Table typedefs;
} Parser;

// Enum to represent the base types
typedef enum {
  TYPE_UNINITIALIZED = 0,
  TYPE_CHAR,
  TYPE_SHORT,
  TYPE_INT,
  TYPE_LONG,
  TYPE_FLOAT,
  TYPE_DOUBLE,
  TYPE_VOID,
  TYPE_STRUCT 
} BaseType;



#define MAX_INDERECTION_LEVEL 4

// Struct representing pointer information
typedef struct {
  bool is_const[MAX_INDERECTION_LEVEL];
  unsigned int indirections_count;
} PointerInfo;

typedef enum {
  ANN_EMPTY,
  ANN_ARRAY,
  ANN_CUSTOM_CALLBACK,
  ANN_OMIT,
} AnnotationKind;

// typedef bool (*AnnCallbackSer)(Serializer *p_ser, const void *value);
// typedef bool (*AnnCallbackDeser)(Serializer *p_ser, void *value);

typedef struct {
  StringView array_size_field_name;
} AnnotationArray;

typedef struct {
  StringView cb_ser_name;
  StringView cb_deser_name;
} AnnotationCustomCallback;

typedef struct {
  AnnotationKind kind;
  union {
    AnnotationArray annotation_array;
    AnnotationCustomCallback annotation_custom_callback;
  } as;
} AnnotationInfo;

// Struct representing type information
typedef struct {
  AnnotationInfo ann_info;
  StringView struct_name; 
  PointerInfo pointer_info; 
  BaseType base_type;
  unsigned int longness; // number of long modifiers
  bool is_const;            
  bool is_unsigned;
} TypeInfo;

typedef struct {
  StringView name;
  TypeInfo type_info;
  unsigned int offset;
} VarInfo;

typedef struct {
  StringView name;
  vec(VarInfo) fields;
} StructInfo;

#define struct_info_init(si) do { (si).name = (StringView){0}; vec_alloc((si).fields); } while (0)
#define struct_info_free(p_si) do { (p_si)->name = (StringView){0}; vec_free((p_si)->fields); } while (0)

bool parse(const char *source, vec(StructInfo) *out);
const char* base_type_to_cstr(BaseType t);
size_t type_info_get_size(const TypeInfo *p_ti);


#endif // !__SERC_PARSER_H__
