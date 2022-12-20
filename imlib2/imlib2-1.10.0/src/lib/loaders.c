#include "common.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "debug.h"
#include "file.h"
#include "image.h"
#include "loaders.h"

#define DBG_PFX "LOAD"
#define DP(fmt...) DC(DBG_LOAD, fmt)

static ImlibLoader *loaders = NULL;
static char         loaders_loaded = 0;

typedef struct {
   const char         *dso;
   const char         *const *ext;
} KnownLoader;

static const char  *const ext_ani[] = { "ani", NULL };
static const char  *const ext_argb[] = { "argb", NULL };
static const char  *const ext_bmp[] = { "bmp", NULL };
static const char  *const ext_ff[] = { "ff", NULL };
#ifdef BUILD_GIF_LOADER
static const char  *const ext_gif[] = { "gif", NULL };
#endif
#ifdef BUILD_HEIF_LOADER
static const char  *const ext_heif[] =
   { "heif", "heifs", "heic", "heics", "avci", "avcs", "avif", "avifs", NULL };
#endif
static const char  *const ext_ico[] = { "ico", NULL };
#ifdef BUILD_JPEG_LOADER
static const char  *const ext_jpeg[] = { "jpg", "jpeg", "jfif", "jfi", NULL };
#endif
#ifdef BUILD_J2K_LOADER
static const char  *const ext_j2k[] = { "jp2", "j2k", NULL };
#endif
#ifdef BUILD_JXL_LOADER
static const char  *const ext_jxl[] = { "jxl", NULL };
#endif
static const char  *const ext_lbm[] = { "iff", "ilbm", "lbm", NULL };
#ifdef BUILD_PNG_LOADER
static const char  *const ext_png[] = { "png", NULL };
#endif
static const char  *const ext_pnm[] =
   { "pnm", "ppm", "pgm", "pbm", "pam", NULL };
#ifdef BUILD_PS_LOADER
static const char  *const ext_ps[] = { "ps", "eps", NULL };
#endif
#ifdef BUILD_SVG_LOADER
static const char  *const ext_svg[] = { "svg", NULL };
#endif
static const char  *const ext_tga[] = { "tga", NULL };
#ifdef BUILD_TIFF_LOADER
static const char  *const ext_tiff[] = { "tiff", "tif", NULL };
#endif
#ifdef BUILD_WEBP_LOADER
static const char  *const ext_webp[] = { "webp", NULL };
#endif
static const char  *const ext_xbm[] = { "xbm", NULL };
static const char  *const ext_xpm[] = { "xpm", NULL };

#ifdef BUILD_BZ2_LOADER
static const char  *const ext_bz2[] = { "bz2", NULL };
#endif
#ifdef BUILD_LZMA_LOADER
static const char  *const ext_lzma[] = { "xz", "lzma", NULL };
#endif
#ifdef BUILD_ZLIB_LOADER
static const char  *const ext_zlib[] = { "gz", NULL };
#endif

#ifdef BUILD_ID3_LOADER
static const char  *const ext_id3[] = { "mp3", NULL };
#endif

static const KnownLoader loaders_known[] = {
   {"ani", ext_ani},
   {"argb", ext_argb},
   {"bmp", ext_bmp},
   {"ff", ext_ff},
#ifdef BUILD_GIF_LOADER
   {"gif", ext_gif},
#endif
#ifdef BUILD_HEIF_LOADER
   {"heif", ext_heif},
#endif
   {"ico", ext_ico},
#ifdef BUILD_JPEG_LOADER
   {"jpeg", ext_jpeg},
#endif
#ifdef BUILD_J2K_LOADER
   {"j2k", ext_j2k},
#endif
#ifdef BUILD_JXL_LOADER
   {"jxl", ext_jxl},
#endif
   {"lbm", ext_lbm},
#ifdef BUILD_PNG_LOADER
   {"png", ext_png},
#endif
#ifdef BUILD_PS_LOADER
   {"ps", ext_ps},
#endif
   {"pnm", ext_pnm},
#ifdef BUILD_SVG_LOADER
   {"svg", ext_svg},
#endif
   {"tga", ext_tga},
#ifdef BUILD_TIFF_LOADER
   {"tiff", ext_tiff},
#endif
#ifdef BUILD_WEBP_LOADER
   {"webp", ext_webp},
#endif
   {"xbm", ext_xbm},
   {"xpm", ext_xpm},
#ifdef BUILD_BZ2_LOADER
   {"bz2", ext_bz2},
#endif
#ifdef BUILD_LZMA_LOADER
   {"lzma", ext_lzma},
#endif
#ifdef BUILD_ZLIB_LOADER
   {"zlib", ext_zlib},
#endif
#ifdef BUILD_ID3_LOADER
   {"id3", ext_id3},
#endif
};

static int
__imlib_IsLoaderLoaded(const char *file)
{
   ImlibLoader        *l;

   for (l = loaders; l; l = l->next)
     {
        if (strcmp(file, l->file) == 0)
           return 1;
     }

   return 0;
}

/* try dlopen()ing the file if we succeed finish filling out the malloced */
/* loader struct and return it */
static ImlibLoader *
__imlib_ProduceLoader(const char *file)
{
   ImlibLoader        *l;
   ImlibLoaderModule  *m;

   DP("%s: %s\n", __func__, file);

   l = malloc(sizeof(ImlibLoader));

   l->handle = dlopen(file, RTLD_NOW | RTLD_LOCAL);
   if (!l->handle)
      goto bail;

   l->module = m = dlsym(l->handle, "loader");
   if (!l->module)
      goto bail;

   /* Check version and that we have at least load() or save() */
   if (m->ldr_version != IMLIB2_LOADER_VERSION ||
       !m->formats || m->num_formats <= 0 || !(m->load || m->save))
     {
        dlclose(l->handle);
        goto bail;
     }

   l->file = strdup(file);
   l->name = m->formats[0];

   l->next = loaders;
   loaders = l;

   return l;

 bail:
   free(l);
   return NULL;
}

/* fre the struct for a loader and close its dlopen'd handle */
static void
__imlib_ConsumeLoader(ImlibLoader * l)
{
   free(l->file);
   if (l->handle)
      dlclose(l->handle);
   free(l);
}

/* remove all loaders int eh list we have cached so we can re-load them */
void
__imlib_RemoveAllLoaders(void)
{
   ImlibLoader        *l, *l_next;

   for (l = loaders; l; l = l_next)
     {
        l_next = l->next;
        __imlib_ConsumeLoader(l);
     }
   loaders = NULL;
   loaders_loaded = 0;
}

/* find all the loaders we can find and load them up to see what they can */
/* load / save */
static void
__imlib_LoadAllLoaders(void)
{
   int                 i, num;
   char              **list;

   DP("%s\n", __func__);

   /* list all the loaders imlib can find */
   list = __imlib_ModulesList(__imlib_PathToLoaders(), &num);
   /* no loaders? well don't load anything */
   if (!list)
      return;

   /* go through the list of filenames for loader .so's and load them */
   /* (or try) and if it succeeds, append to our loader list */
   for (i = num - 1; i >= 0; i--)
     {
        if (!__imlib_IsLoaderLoaded(list[i]))
           __imlib_ProduceLoader(list[i]);
        free(list[i]);
     }
   free(list);

   loaders_loaded = 1;
}

ImlibLoader       **
__imlib_GetLoaderList(void)
{
   if (!loaders_loaded)
      __imlib_LoadAllLoaders();
   return &loaders;
}

static ImlibLoader *
__imlib_LookupKnownLoader(const char *format)
{
   const KnownLoader  *kl;
   ImlibLoader        *l;
   unsigned int        i;
   const char         *const *exts;
   char               *dso;

   kl = NULL;
   for (i = 0; i < ARRAY_SIZE(loaders_known); i++)
     {
        for (exts = loaders_known[i].ext; *exts; exts++)
          {
             if (strcasecmp(format, *exts) != 0)
                continue;
             kl = &loaders_known[i];
             goto done;
          }
     }

 done:
   l = NULL;
   if (kl)
     {
        dso = __imlib_ModuleFind(__imlib_PathToLoaders(), kl->dso);
        l = __imlib_ProduceLoader(dso);
        free(dso);
     }
   DP("%s: '%s' -> '%s': %p\n", __func__, format, kl ? kl->dso : "-", l);
   return l;
}

static int
_loader_ok_for(const ImlibLoader * l, int for_save)
{
   return (for_save && l->module->save) || (!for_save && l->module->load);
}

static ImlibLoader *
__imlib_LookupLoadedLoader(const char *format, int for_save)
{
   ImlibLoader        *l;
   ImlibLoaderModule  *m;

   DP("%s: fmt='%s'\n", __func__, format);

   /* go through the loaders - first loader that claims to handle that */
   /* image type (extension wise) wins as a first guess to use - NOTE */
   /* this is an OPTIMISATION - it is possible the file has no extension */
   /* or has an unrecognised one but still is loadable by a loader. */
   /* if thkis initial loader failes to load the load mechanism will */
   /* systematically go from most recently used to least recently used */
   /* loader until one succeeds - or none are left and all have failed */
   /* and only if all fail does the laod fail. the lao9der that does */
   /* succeed gets it way to the head of the list so it's going */
   /* to be used first next time in this search mechanims - this */
   /* assumes you tend to laod a few image types and ones generally */
   /* of the same format */
   for (l = loaders; l; l = l->next)
     {
        int                 i;

        m = l->module;

        /* go through all the formats that loader supports */
        for (i = 0; i < m->num_formats; i++)
          {
             /* does it match ? */
             if (strcasecmp(m->formats[i], format) == 0)
               {
                  /* does it provide the function we need? */
                  if (_loader_ok_for(l, for_save))
                     goto done;
               }
          }
     }

 done:
   DP("%s: fmt='%s': %s\n", __func__, format, l ? l->file : "-");
   return l;
}

__EXPORT__ ImlibLoader *
__imlib_FindBestLoader(const char *file, const char *format, int for_save)
{
   ImlibLoader        *l;

   DP("%s: file='%s' fmt='%s'\n", __func__, file, format);

   if (!format)
      format = __imlib_FileExtension(file);

   if (!format || format[0] == '\0')
      return NULL;

   if (loaders)
     {
        /* At least one loader loaded */
        l = __imlib_LookupLoadedLoader(format, for_save);
        if (l || loaders_loaded)
           goto done;
     }

   l = __imlib_LookupKnownLoader(format);
   if (l && _loader_ok_for(l, for_save))
      goto done;

   __imlib_LoadAllLoaders();

   l = __imlib_LookupLoadedLoader(format, for_save);

 done:
   DP("%s: fmt='%s': %s\n", __func__, format, l ? l->file : "-");
   return l;
}
