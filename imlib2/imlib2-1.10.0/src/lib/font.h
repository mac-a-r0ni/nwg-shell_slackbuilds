#ifndef FONT_H
#define FONT_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "object.h"
#include "types.h"

typedef struct _Imlib_Font {
   Imlib_Object_List   _list_data;
   char               *name;
   char               *file;
   int                 size;

   struct {
      FT_Face             face;
   } ft;

   Imlib_Hash         *glyphs;

   int                 usage;

   int                 references;

   /* using a double-linked list for the fallback chain */
   struct _Imlib_Font *fallback_prev;
   struct _Imlib_Font *fallback_next;
} ImlibFont;

typedef struct {
   FT_Glyph            glyph;
   FT_BitmapGlyph      glyph_out;
} Imlib_Font_Glyph;

#define IMLIB_GLYPH_NONE ((Imlib_Font_Glyph*) 1)        /* Glyph not found */

/* functions */

void                __imlib_font_init(void);
int                 __imlib_font_ascent_get(ImlibFont * fn);
int                 __imlib_font_descent_get(ImlibFont * fn);
int                 __imlib_font_max_ascent_get(ImlibFont * fn);
int                 __imlib_font_max_descent_get(ImlibFont * fn);
int                 __imlib_font_get_line_advance(ImlibFont * fn);
void                __imlib_font_add_font_path(const char *path);
void                __imlib_font_del_font_path(const char *path);
int                 __imlib_font_path_exists(const char *path);
char              **__imlib_font_list_font_path(int *num_ret);
char              **__imlib_font_list_fonts(int *num_ret);

ImlibFont          *__imlib_font_load_joined(const char *name);
void                __imlib_font_free(ImlibFont * fn);
int                 __imlib_font_insert_into_fallback_chain_imp(ImlibFont * fn,
                                                                ImlibFont *
                                                                fallback);
void                __imlib_font_remove_from_fallback_chain_imp(ImlibFont * fn);
int                 __imlib_font_cache_get(void);
void                __imlib_font_cache_set(int size);
void                __imlib_font_flush(void);
void                __imlib_font_modify_cache_by(ImlibFont * fn, int dir);
void                __imlib_font_modify_cache_by(ImlibFont * fn, int dir);
void                __imlib_font_flush_last(void);
ImlibFont          *__imlib_font_find(const char *name, int size);

void                __imlib_font_query_size(ImlibFont * fn, const char *text,
                                            int *w, int *h);
int                 __imlib_font_query_inset(ImlibFont * fn, const char *text);
void                __imlib_font_query_advance(ImlibFont * fn, const char *text,
                                               int *h_adv, int *v_adv);
int                 __imlib_font_query_char_coords(ImlibFont * fn,
                                                   const char *text, int pos,
                                                   int *cx, int *cy,
                                                   int *cw, int *ch);
int                 __imlib_font_query_text_at_pos(ImlibFont * fn,
                                                   const char *text,
                                                   int x, int y,
                                                   int *cx, int *cy,
                                                   int *cw, int *ch);

Imlib_Font_Glyph   *__imlib_font_get_next_glyph(ImlibFont * fn,
                                                const char *utf8,
                                                int *cindx,
                                                FT_UInt * pindex, int *pkern);
Imlib_Font_Glyph   *__imlib_font_cache_glyph_get(ImlibFont * fn, FT_UInt index);
void                __imlib_render_str(ImlibImage * im, ImlibFont * f,
                                       int drx, int dry, const char *text,
                                       uint32_t pixel, int dir, double angle,
                                       int *retw, int *reth, int blur,
                                       int *nextx, int *nexty, ImlibOp op,
                                       int clx, int cly, int clw, int clh);
void                __imlib_font_draw(ImlibImage * dst, uint32_t col,
                                      ImlibFont * fn, int x, int y,
                                      const char *text, int *nextx, int *nexty,
                                      int clx, int cly, int clw, int clh);

#endif /* FONT_H */
