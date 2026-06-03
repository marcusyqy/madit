#include "shader_compiler.h"

#include "mb/mb_file.h"

ShadercInfo shader_compiler_create(void) {
  ShadercInfo sc = { .compiler = shaderc_compiler_initialize(), .options = shaderc_compile_options_initialize() };
  shaderc_compile_options_set_source_language(sc.options, shaderc_source_language_glsl);
  shaderc_compile_options_set_optimization_level(sc.options, shaderc_optimization_level_performance);
  shaderc_compile_options_set_generate_debug_info(sc.options);
  return sc;
}

void shader_compiler_destroy(ShadercInfo *sc) {
  shaderc_compile_options_release(sc->options);
  shaderc_compiler_release(sc->compiler);
}

mb_StringView shader_compiler_read_glsl_and_compile_to_spv(mb_Arena *arena, ShadercInfo *sc, mb_StringView file_name, shaderc_shader_kind kind) {
  mb_File file = mb_open_file(file_name, mb_FileOpenMode_Text);
  assert(file);

  mb_TempArena temp = mb_begin_temp_arena(mb_get_scratch_arena(arena));
  mb_StringView shader_str = mb_file_read_bytes(temp.arena, file);
  mb_StringView file_name_without_dir = mb_str_split_from_right_till(&file_name, '/');

  shaderc_compilation_result_t sc_result = shaderc_compile_into_spv(sc->compiler, shader_str.data, shader_str.count, kind, mb_str_to_cstr(temp.arena, file_name_without_dir), "main", sc->options);

  mb_StringView str_view = {0};
  if(shaderc_result_get_num_errors(sc_result) == 0) {
    str_view.count = shaderc_result_get_length(sc_result),
    str_view.data = mb_arena_push(arena, char, .count = str_view.count);
    MemoryCopy(str_view.data, shaderc_result_get_bytes(sc_result), str_view.count);
  } else {
    // @TODO: let someone outside handle it instead.
    // For now it is fine to just print it out.
    fprintf(stdout, "Error compiling shader: %s\n", shaderc_result_get_error_message(sc_result));
  }

  shaderc_result_release(sc_result);
  mb_end_temp_arena(&temp);
  mb_close_file(file);
  return str_view;
}



