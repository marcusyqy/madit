#include "font.h"
#include "mb/mb_arena.h"

// @TODO: we should move this
#include <ft2build.h>
#include FT_FREETYPE_H

#define FONT_ATLAS_SIZE 2048

int font_test(void) {
  FT_Library ft;
  if(FT_Init_FreeType(&ft)) {
    printf("ERROR::FREETYPE: Could not init FreeType Library\n");
    return -1;
  }

  FT_Face face;
  if(FT_New_Face(ft, "data/fonts/arial.ttf", 0, &face)) {
    printf("ERROR::FREETYPE: Failed to load font\n");
    return -1;
  }

  return 0;
}

FontTextureAtlas font_load_into_texture_atlas(mb_StringView file_name) {
  mb_TempArena temp = mb_begin_temp_arena(mb_get_scratch_arena(0));

  mb_end_temp_arena(&temp);
  // return default
  return (FontTextureAtlas){0};
}
