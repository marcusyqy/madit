#ifndef _SHADER_COMPILER_H_
#define _SHADER_COMPILER_H_

#include <shaderc/shaderc.h>
#include "mb/mb_str.h"

typedef struct ShadercInfo {
  shaderc_compiler_t compiler;
  shaderc_compile_options_t options;
} ShadercInfo;

ShadercInfo shader_compiler_create(void);
void shader_compiler_destroy(ShadercInfo *sc);
mb_StringView shader_compiler_read_glsl_and_compile_to_spv(mb_Arena *arena, ShadercInfo *sc, mb_StringView file_name, shaderc_shader_kind kind);

#endif // _SHADER_COMPILER_H_
