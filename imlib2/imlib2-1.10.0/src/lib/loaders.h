#ifndef __LOADERS
#define __LOADERS 1

#include "types.h"

#define IMLIB2_LOADER_VERSION 1

typedef struct {
   unsigned char       ldr_version;     /* Module ABI version */
   unsigned char       rsvd;
   unsigned short      num_formats;     /* Length og known extension list */
   const char         *const *formats;  /* Known extension list */
   int                 (*load)(ImlibImage * im, int load_data);
   int                 (*save)(ImlibImage * im);
} ImlibLoaderModule;

struct _ImlibLoader {
   char               *file;
   void               *handle;
   ImlibLoaderModule  *module;
   ImlibLoader        *next;

   const char         *name;
};

void                __imlib_RemoveAllLoaders(void);
ImlibLoader       **__imlib_GetLoaderList(void);

#endif /* __LOADERS */
