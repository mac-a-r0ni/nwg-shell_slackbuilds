#ifndef _DYNAMIC_FILTERS_H_
#define _DYNAMIC_FILTERS_H_

#include "script.h"

typedef struct {
   char               *name;
   char               *author;
   char               *description;
   char              **filters;
   int                 num_filters;
} ImlibFilterInfo;

typedef struct _ImlibExternalFilter {
   char               *name;
   char               *author;
   char               *description;
   int                 num_filters;
   char               *filename;
   void               *handle;
   char              **filters;
   void                (*init_filter)(ImlibFilterInfo * info);
   void                (*deinit_filter)(void);
   void               *(*exec_filter)(char *filter, void *im,
                                      IFunctionParam * params);
   struct _ImlibExternalFilter *next;
} ImlibExternalFilter;

void                __imlib_dynamic_filters_init(void);
void                __imlib_dynamic_filters_deinit(void);
ImlibExternalFilter *__imlib_get_dynamic_filter(char *name);

#endif
