#ifndef __SERC_CODEGEN_H__
#define __SERC_CODEGEN_H__

#include "./lib/ds/vec.h"
#include "parser.h"

/// Generates *.c and *.h files for serialization structs pointed by p_si
///
/// @param p_si: vector of structs to generate json serialization for
/// @param path: path to the directory where to save generated files
bool generate_json_serialization(const vec(StructInfo) const * p_si, const char *path);


#endif // !__SERC_CODEGEN_H__
