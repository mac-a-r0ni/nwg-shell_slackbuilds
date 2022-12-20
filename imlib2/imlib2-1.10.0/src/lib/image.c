#include "config.h"
#include <Imlib2.h>
#include "common.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "debug.h"
#include "file.h"
#include "image.h"
#include "loaders.h"
#ifdef BUILD_X11
#include "x11_pixmap.h"
#endif

struct _ImlibImageFileInfo {
   struct _ImlibImageFileInfo *next;
   char               *name;
   FILE               *fp;
   const void         *fdata;
   off_t               fsize;
   /* vvv Private vvv */
   bool                keep_fp;
   bool                keep_mem;
   /* ^^^ Private ^^^ */
};

#define DBG_PFX "IMG"
#define DP(fmt...) DC(DBG_LOAD, fmt)

/* Imlib loader context */
struct _ImlibLoaderCtx {
   ImlibProgressFunction progress;
   char                granularity;
   int                 pct, area, row;
   int                 pass, n_pass;
};

static ImlibImage  *images = NULL;

static int          cache_size = 4096 * 1024;

__EXPORT__ uint32_t *
__imlib_AllocateData(ImlibImage * im)
{
   int                 w = im->w;
   int                 h = im->h;

   if (w <= 0 || h <= 0)
      return NULL;

   if (im->data_memory_func)
      im->data = im->data_memory_func(NULL, w * h * sizeof(uint32_t));
   else
      im->data = malloc(w * h * sizeof(uint32_t));

   return im->data;
}

__EXPORT__ void
__imlib_FreeData(ImlibImage * im)
{
   if (!im->data)
      return;

   if (im->data_memory_func)
      im->data_memory_func(im->data, im->w * im->h * sizeof(uint32_t));
   else
      free(im->data);

   im->data = NULL;
}

__EXPORT__ void
__imlib_ReplaceData(ImlibImage * im, unsigned int *new_data)
{
   __imlib_FreeData(im);
   im->data = new_data;
   im->data_memory_func = NULL;
}

static int
__imlib_FileContextOpen(ImlibImageFileInfo * fi, FILE * fp,
                        const void *fdata, off_t fsize)
{
   struct stat         st;

   if (fp)
     {
        fi->keep_fp = true;
        fi->fp = fp;
     }
   else if (fdata)
     {
        fi->keep_mem = true;
     }
   else
     {
        fi->fp = fopen(fi->name, "rb");
        if (!fi->fp)
           return -1;
     }

   if (fi->fsize == 0)
     {
        if (fsize == 0)
          {
             __imlib_FileStat(fi->name, &st);
             fsize = st.st_size;
          }
        fi->fsize = fsize;
     }

   if (!fdata)
     {
        fdata = mmap(NULL, fi->fsize, PROT_READ, MAP_SHARED, fileno(fi->fp), 0);
        if (fdata == MAP_FAILED)
           return -1;
     }

   fi->fdata = fdata;

   return 0;
}

static void
__imlib_FileContextClose(ImlibImageFileInfo * fi)
{
   if (fi->fdata && !fi->keep_mem)
     {
        munmap((void *)fi->fdata, fi->fsize);
        fi->fdata = NULL;
     }

   if (fi->fp && !fi->keep_fp)
     {
        fclose(fi->fp);
        fi->fp = NULL;
     }
}

static int
__imlib_ImageFileContextPush(ImlibImage * im, char *file)
{
   ImlibImageFileInfo *fi;

   fi = calloc(1, sizeof(ImlibImageFileInfo));
   if (!fi)
      return -1;

   fi->name = file;             /* file must have been alloced, will be freed */

   fi->next = im->fi;
   im->fi = fi;

   return 0;
}

static void
__imlib_ImageFileContextPop(ImlibImage * im)
{
   ImlibImageFileInfo *fi = im->fi;

   im->fi = fi->next;

   free(fi->name);
   free(fi);
}

/* create an image data struct and fill it in */
static ImlibImage  *
__imlib_ProduceImage(void)
{
   ImlibImage         *im;

   im = calloc(1, sizeof(ImlibImage));
   im->flags = F_FORMAT_IRRELEVANT;

   return im;
}

/* free an image struct */
static void
__imlib_ConsumeImage(ImlibImage * im)
{
#ifdef BUILD_X11
   __imlib_PixmapUnrefImage(im);
#endif

   __imlib_FreeAllTags(im);

   if (im->file && (!im->fi || im->fi->name != im->file))
      free(im->file);
   free(im->key);
   if (im->data && !IM_FLAG_ISSET(im, F_DONT_FREE_DATA))
      __imlib_FreeData(im);
   free(im->format);

   if (im->fi)
      __imlib_ImageFileContextPop(im);

   free(im->pframe);

   free(im);
}

static ImlibImage  *
__imlib_FindCachedImage(const char *file, int frame)
{
   ImlibImage         *im, *im_prev;

   DP("%s: '%s' frame %d\n", __func__, file, frame);

   for (im = images, im_prev = NULL; im; im_prev = im, im = im->next)
     {
        /* if the filenames match and it's valid */
        if (strcmp(file, im->file) || IM_FLAG_ISSET(im, F_INVALID))
           continue;
        if (frame != im->frame)
           continue;

        /* move the image to the head of the image list */
        if (im_prev)
          {
             im_prev->next = im->next;
             im->next = images;
             images = im;
          }
        DP(" got %p: '%s' frame %d\n", im, im->fi->name, im->frame);
        return im;
     }
   DP(" got none\n");
   return NULL;
}

/* add an image to the cache of images (at the start) */
static void
__imlib_AddImageToCache(ImlibImage * im)
{
   DP("%s: %p: '%s' frame %d\n", __func__, im, im->fi->name, im->frame);
   im->next = images;
   images = im;
}

/* remove (unlink) an image from the cache of images */
static void
__imlib_RemoveImageFromCache(ImlibImage * im_del)
{
   ImlibImage         *im, *im_prev;

   im = im_del;
   DP("%s: %p: '%s' frame %d\n", __func__, im, im->fi->name, im->frame);

   for (im = images, im_prev = NULL; im; im_prev = im, im = im->next)
     {
        if (im == im_del)
          {
             if (im_prev)
                im_prev->next = im->next;
             else
                images = im->next;
             return;
          }
     }
}

/* work out how much we have floaitng aroudn in our speculative cache */
/* (images and pixmaps that have 0 reference counts) */
int
__imlib_CurrentCacheSize(void)
{
   ImlibImage         *im, *im_next;
   int                 current_cache = 0;

   for (im = images; im; im = im_next)
     {
        im_next = im->next;

        /* mayaswell clean out stuff thats invalid that we dont need anymore */
        if (im->references == 0)
          {
             if (IM_FLAG_ISSET(im, F_INVALID))
               {
                  __imlib_RemoveImageFromCache(im);
                  __imlib_ConsumeImage(im);
               }
             /* it's valid but has 0 ref's - append to cache size count */
             else
                current_cache += im->w * im->h * sizeof(uint32_t);
          }
     }

#ifdef BUILD_X11
   current_cache += __imlib_PixmapCacheSize();
#endif

   return current_cache;
}

/* clean out images from the cache if the cache is overgrown */
static void
__imlib_CleanupImageCache(void)
{
   ImlibImage         *im, *im_next, *im_del;
   int                 current_cache;

   current_cache = __imlib_CurrentCacheSize();

   /* remove 0 ref count invalid (dirty) images */
   for (im = images; im; im = im_next)
     {
        im_next = im->next;
        if (im->references <= 0 && IM_FLAG_ISSET(im, F_INVALID))
          {
             __imlib_RemoveImageFromCache(im);
             __imlib_ConsumeImage(im);
          }
     }

   /* while the cache size of 0 ref coutn data is bigger than the set value */
   /* clean out the oldest members of the imaeg cache */
   while (current_cache > cache_size)
     {
        for (im = images, im_del = NULL; im; im = im->next)
          {
             if (im->references <= 0)
                im_del = im;
          }
        if (!im_del)
           break;

        __imlib_RemoveImageFromCache(im_del);
        __imlib_ConsumeImage(im_del);

        current_cache = __imlib_CurrentCacheSize();
     }
}

/* set the cache size */
void
__imlib_SetCacheSize(int size)
{
   cache_size = size;
   __imlib_CleanupImageCache();
#ifdef BUILD_X11
   __imlib_CleanupImagePixmapCache();
#endif
}

/* return the cache size */
int
__imlib_GetCacheSize(void)
{
   return cache_size;
}

/* create a new image struct from data passed that is wize w x h then return */
/* a pointer to that image sturct */
ImlibImage         *
__imlib_CreateImage(int w, int h, uint32_t * data)
{
   ImlibImage         *im;

   im = __imlib_ProduceImage();
   im->w = w;
   im->h = h;
   im->data = data;
   im->references = 1;
   IM_FLAG_SET(im, F_UNCACHEABLE);
   return im;
}

static int
__imlib_LoadImageWrapper(const ImlibLoader * l, ImlibImage * im, int load_data)
{
   int                 rc;

   DP("%s: fmt='%s' file='%s'(%s) frame=%d, imm=%d\n", __func__,
      l->name, im->file, im->fi->name, im->frame, load_data);

#if IMLIB2_DEBUG
   unsigned int        t0 = __imlib_time_us();
#endif

   if (l->module->load)
     {
        if (!im->format)
           im->format = strdup(l->name);

        rc = l->module->load(im, load_data);
     }
   else
     {
        return LOAD_FAIL;
     }

   DP("%s: %-4s: %s: Elapsed time: %.3f ms\n", __func__,
      l->name, im->fi->name, 1e-3 * (__imlib_time_us() - t0));

   if (rc <= LOAD_FAIL)
     {
        /* Failed - clean up */
        DP("%s: Failed (rc=%d)\n", __func__, rc);

        im->w = im->h = 0;
        __imlib_FreeData(im);
        free(im->format);
        im->format = NULL;
     }

   return rc;
}

static void
__imlib_LoadCtxInit(ImlibImage * im, ImlibLoaderCtx * lc,
                    ImlibProgressFunction prog, int gran)
{
   im->lc = lc;
   lc->progress = prog;
   lc->granularity = gran;
   lc->pct = lc->row = 0;
   lc->area = 0;
   lc->pass = 0;
   lc->n_pass = 1;
}

static int
__imlib_LoadErrorToErrno(int loader_ret, int save)
{
   switch (loader_ret)
     {
     default:                  /* We should not go here */
        return IMLIB_ERR_INTERNAL;
     case LOAD_SUCCESS:
        return 0;
     case LOAD_FAIL:
        return (save) ? IMLIB_ERR_NO_SAVER : IMLIB_ERR_NO_LOADER;
     case LOAD_OOM:
        return ENOMEM;
     case LOAD_BADFILE:
        return errno;
     case LOAD_BADIMAGE:
        return IMLIB_ERR_BAD_IMAGE;
     case LOAD_BADFRAME:
        return IMLIB_ERR_BAD_FRAME;
     }
}

ImlibImage         *
__imlib_LoadImage(const char *file, ImlibLoadArgs * ila)
{
   ImlibImage         *im;
   ImlibLoader       **loaders, *best_loader, *l, *previous_l;
   int                 err, loader_ret;
   ImlibLoaderCtx      ilc;
   struct stat         st;
   char               *im_file, *im_key;

   if (!file || file[0] == '\0')
      return NULL;

   if (!ila->nocache)
     {
        /* see if we already have the image cached */
        im = __imlib_FindCachedImage(file, ila->frame);

        /* if we found a cached image and we should always check that it is */
        /* accurate to the disk conents if they changed since we last loaded */
        /* and that it is still a valid image */
        if (im && !IM_FLAG_ISSET(im, F_INVALID))
          {
             if (IM_FLAG_ISSET(im, F_ALWAYS_CHECK_DISK))
               {
                  time_t              current_modified_time;

                  current_modified_time = ila->fp ?
                     __imlib_FileModDateFd(fileno(ila->fp)) :
                     __imlib_FileModDate(im->fi->name);
                  /* if the file on disk is newer than the cached one */
                  if (current_modified_time != im->moddate)
                    {
                       /* invalidate image */
                       IM_FLAG_SET(im, F_INVALID);
                    }
                  else
                    {
                       /* image is ok to re-use - program is just being stupid loading */
                       /* the same data twice */
                       im->references++;
                       return im;
                    }
               }
             else
               {
                  im->references++;
                  return im;
               }
          }
     }

   im_file = im_key = NULL;
   if (ila->fdata)
     {
        err = 0;
        st.st_size = ila->fsize;
        st.st_mtime = st.st_ctime = 0;
     }
   else if (ila->fp)
     {
        err = fstat(fileno(ila->fp), &st);
     }
   else
     {
        err = __imlib_FileStat(file, &st);
        if (err)
          {
             im_key = __imlib_FileKey(file);
             if (im_key)
               {
                  im_file = __imlib_FileRealFile(file);
                  err = __imlib_FileStat(im_file, &st);
               }
          }
     }

   if (err)
      err = errno;
   else if (st.st_size == 0)
      err = IMLIB_ERR_BAD_IMAGE;
   else if (!ila->fdata && __imlib_StatIsDir(&st))
      err = EISDIR;

   if (err)
     {
        ila->err = err;
        free(im_file);
        free(im_key);
        return NULL;
     }

   /* either image in cache is invalid or we dont even have it in cache */
   /* so produce a new one and load an image into that */
   im = __imlib_ProduceImage();
   im->file = strdup(file);
   im->key = im_key;
   im->frame = ila->frame;

   if (__imlib_ImageFileContextPush(im, im_file ? im_file : im->file) ||
       __imlib_FileContextOpen(im->fi, ila->fp, ila->fdata, st.st_size))
     {
        ila->err = errno;
        __imlib_ConsumeImage(im);
        return NULL;
     }

   im->moddate = __imlib_StatModDate(&st);

   im->data_memory_func = imlib_context_get_image_data_memory_function();

   if (ila->pfunc)
     {
        __imlib_LoadCtxInit(im, &ilc, ila->pfunc, ila->pgran);
        ila->immed = 1;
     }

   /* take a guess by extension on the best loader to use */
   best_loader = __imlib_FindBestLoader(im->fi->name, NULL, 0);

   loader_ret = LOAD_FAIL;
   loaders = NULL;

   for (l = previous_l = NULL;;)
     {
        if (l == NULL && best_loader)
          {
             l = best_loader;   /* First try best_loader, if any */
          }
        else if (!loaders)
          {
             loaders = __imlib_GetLoaderList();
             l = *loaders;
             if (best_loader && l == best_loader)
                continue;       /* Skip best_loader that already failed */
          }
        else
          {
             previous_l = l;
             l = l->next;
             if (best_loader && l == best_loader)
                continue;       /* Skip best_loader that already failed */
          }
        if (!l)
           break;

        errno = 0;
        loader_ret = __imlib_LoadImageWrapper(l, im, ila->immed);

        switch (loader_ret)
          {
          case LOAD_BREAK:     /* Break signaled by progress callback */
          case LOAD_SUCCESS:   /* Image loaded successfully           */
             /* Loader accepted image - done */
             im->loader = l;

             /* move the successful loader to the head of the list */
             if (previous_l)
               {
                  previous_l->next = l->next;
                  l->next = *loaders;
                  *loaders = l;
               }
             break;

          case LOAD_FAIL:
             /* Image was not recognized by loader - try next */
             if (im->fi->fp)
               {
                  fflush(im->fi->fp);
                  rewind(im->fi->fp);
               }
             continue;

          default:             /* We should not go here */
          case LOAD_OOM:       /* Could not allocate memory           */
          case LOAD_BADFILE:   /* File could not be accessed          */
             /* Unlikely that another loader will succeed */
          case LOAD_BADIMAGE:  /* Image is invalid                    */
          case LOAD_BADFRAME:  /* Requested frame not found           */
             /* Signature was good but file was somehow not */
             break;
          }

        /* Done or don't look for other */
        break;
     }

   im->lc = NULL;

   if (!ila->fp)
      __imlib_FileContextClose(im->fi);

   if (loader_ret <= LOAD_FAIL)
     {
        /* Image loading failed.
         * Free the skeleton image struct we had and return NULL */
        ila->err = __imlib_LoadErrorToErrno(loader_ret, 0);
        __imlib_ConsumeImage(im);
        return NULL;
     }

   /* the load succeeded - make sure the image is referenced then add */
   /* it to our cache unless nocache is set */
   im->references = 1;
   if (loader_ret == LOAD_BREAK)
      ila->nocache = 1;
   if (!ila->nocache)
      __imlib_AddImageToCache(im);
   else
      IM_FLAG_SET(im, F_UNCACHEABLE);

   return im;
}

int
__imlib_LoadImageData(ImlibImage * im)
{
   int                 err;

   if (im->data)
      return 0;                 /* Ok */

   /* Just checking - it should be impossible that loader is not set */
   if (!im->loader)
      return IMLIB_ERR_INTERNAL;

   err = __imlib_FileContextOpen(im->fi, NULL, NULL, 0);
   if (err)
      return err;
   err = __imlib_LoadImageWrapper(im->loader, im, 1);

   __imlib_FileContextClose(im->fi);

   return __imlib_LoadErrorToErrno(err, 0);
}

__EXPORT__ int
__imlib_LoadEmbedded(ImlibLoader * l, ImlibImage * im, int load_data,
                     const char *file)
{
   int                 rc;

   if (!l || !im)
      return LOAD_FAIL;

   __imlib_ImageFileContextPush(im, strdup(file));
   rc = __imlib_FileContextOpen(im->fi, NULL, NULL, 0);
   if (rc)
      return LOAD_FAIL;

   rc = __imlib_LoadImageWrapper(l, im, load_data);

   __imlib_FileContextClose(im->fi);
   __imlib_ImageFileContextPop(im);

   return rc;
}

__EXPORT__ int
__imlib_LoadEmbeddedMem(ImlibLoader * l, ImlibImage * im, int load_data,
                        const void *fdata, unsigned int fsize)
{
   int                 rc;

   if (!l || !im)
      return LOAD_FAIL;

   __imlib_ImageFileContextPush(im, NULL);
   rc = __imlib_FileContextOpen(im->fi, NULL, fdata, fsize);
   if (rc)
      return LOAD_FAIL;

   rc = __imlib_LoadImageWrapper(l, im, load_data);

   __imlib_FileContextClose(im->fi);
   __imlib_ImageFileContextPop(im);

   return rc;
}

__EXPORT__ void
__imlib_LoadProgressSetPass(ImlibImage * im, int pass, int n_pass)
{
   im->lc->pass = pass;
   im->lc->n_pass = n_pass;

   im->lc->row = 0;
}

__EXPORT__ int
__imlib_LoadProgress(ImlibImage * im, int x, int y, int w, int h)
{
   ImlibLoaderCtx     *lc = im->lc;
   int                 rc;

   lc->area += w * h;
   lc->pct = (100. * lc->area + .1) / (im->w * im->h);

   rc = !lc->progress(im, lc->pct, x, y, w, h);

   return rc;
}

__EXPORT__ int
__imlib_LoadProgressRows(ImlibImage * im, int row, int nrows)
{
   ImlibLoaderCtx     *lc = im->lc;
   int                 col = 0;
   int                 rc = 0;
   int                 pct, nrtot;

   if (nrows > 0)
     {
        /* Row index counting up */
        nrtot = row + nrows;
        row = lc->row;
        nrows = nrtot - lc->row;
     }
   else
     {
        /* Row index counting down */
        nrtot = im->h - row;
        nrows = nrtot - lc->row;
     }

   pct = (100 * nrtot * (lc->pass + 1)) / (im->h * lc->n_pass);
   if (pct == 100 || pct >= lc->pct + lc->granularity)
     {
        rc = !lc->progress(im, pct, col, row, im->w, nrows);
        lc->row = nrtot;
        lc->pct += lc->granularity;
     }

   return rc;
}

__EXPORT__ ImlibImageFrame *
__imlib_GetFrame(ImlibImage * im)
{
   if (!im->pframe)
      im->pframe = calloc(1, sizeof(ImlibImageFrame));
   return im->pframe;
}

/* free and image - if its uncachable and refcoutn is 0 - free it in reality */
void
__imlib_FreeImage(ImlibImage * im)
{
   /* if the refcount is positive */
   if (im->references >= 0)
     {
        /* reduce a reference from the count */
        im->references--;
        /* if its uncachchable ... */
        if (IM_FLAG_ISSET(im, F_UNCACHEABLE))
          {
             /* and we're down to no references for the image then free it */
             if (im->references == 0)
                __imlib_ConsumeImage(im);
          }
        /* otherwise clean up our cache if the image becoem 0 ref count */
        else if (im->references == 0)
           __imlib_CleanupImageCache();
     }
}

/* dirty and image by settings its invalid flag */
void
__imlib_DirtyImage(ImlibImage * im)
{
   IM_FLAG_SET(im, F_INVALID);
#ifdef BUILD_X11
   /* and dirty all pixmaps generated from it */
   __imlib_DirtyPixmapsForImage(im);
#endif
}

__EXPORT__ const char *
__imlib_GetKey(const ImlibImage * im)
{
   return im->key;
}

void
__imlib_SaveImage(ImlibImage * im, const char *file, ImlibLoadArgs * ila)
{
   ImlibLoader        *l;
   ImlibLoaderCtx      ilc;
   int                 loader_ret;

   if (!file)
     {
        ila->err = ENOENT;
        return;
     }

   /* find the laoder for the format - if its null use the extension */
   l = __imlib_FindBestLoader(file, im->format, 1);
   /* no loader - abort */
   if (!l)
     {
        ila->err = IMLIB_ERR_NO_SAVER;
        return;
     }

   if (ila->pfunc)
      __imlib_LoadCtxInit(im, &ilc, ila->pfunc, ila->pgran);

   __imlib_ImageFileContextPush(im, strdup(file));

   /* call the saver */
   loader_ret = l->module->save(im);

   __imlib_ImageFileContextPop(im);

   im->lc = NULL;

   ila->err = __imlib_LoadErrorToErrno(loader_ret, 1);
}
