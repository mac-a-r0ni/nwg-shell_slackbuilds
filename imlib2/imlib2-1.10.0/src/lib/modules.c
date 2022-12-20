#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "strutils.h"

static const char  *
__imlib_PathToModules(void)
{
#if 0
   static const char  *path = NULL;

   if (path)
      return path;

   path = getenv("IMLIB2_MODULE_PATH");
   if (path && __imlib_FileIsDir(path))
      return path;
#endif

   return PACKAGE_LIB_DIR "/imlib2";
}

static char       **
_module_paths(const char *env, const char *mdir)
{
   char              **ppaths, **pp;
   const char         *penv;
   char                buf[1024];

   penv = getenv(env);
   if (penv)
     {
        ppaths = __imlib_StrSplit(penv, ':');
        if (!ppaths)
           goto done;
        for (pp = ppaths; *pp; pp++)
          {
             if (strcmp(*pp, "*") == 0)
               {
                  /* Substitute default path */
                  free(*pp);
                  snprintf(buf, sizeof(buf), "%s/%s",
                           __imlib_PathToModules(), mdir);
                  *pp = strdup(buf);
               }
          }
     }
   else
     {
        /* Use default path */
        ppaths = malloc(2 * sizeof(char *));
        if (!ppaths)
           goto done;
        snprintf(buf, sizeof(buf), "%s/%s", __imlib_PathToModules(), mdir);
        ppaths[0] = strdup(buf);
        ppaths[1] = NULL;
     }

 done:
   return ppaths;
}

char              **
__imlib_PathToFilters(void)
{
   static char       **ppaths = NULL;

   if (ppaths)
      return ppaths;

   ppaths = _module_paths("IMLIB2_FILTER_PATH", "filters");

   return ppaths;
}

char              **
__imlib_PathToLoaders(void)
{
   static char       **ppaths = NULL;

   if (ppaths)
      return ppaths;

   ppaths = _module_paths("IMLIB2_LOADER_PATH", "loaders");

   return ppaths;
}

static bool
_file_is_module(const char *name)
{
   const char         *ext;

   ext = strrchr(name, '.');
   if (!ext)
      return false;

   if (
#ifdef __CYGWIN__
         strcasecmp(ext, ".dll") != 0 &&
#endif
         strcasecmp(ext, ".so") != 0)
      return false;

   return true;
}

char              **
__imlib_ModulesList(char **ppath, int *num_ret)
{
   char              **pp, **list, **l;
   char                file[1024], *p;
   int                 num, i, ntot;

   *num_ret = 0;
   list = NULL;

   if (!ppath)
      goto done;

   ntot = 0;

   for (pp = ppath; *pp; pp++)
     {
        l = __imlib_FileDir(*pp, &num);
        if (!l)
           continue;
        if (num <= 0)
           continue;

        list = realloc(list, (ntot + num) * sizeof(char *));
        if (list)
          {
             for (i = 0; i < num; i++)
               {
                  if (!_file_is_module(l[i]))
                     continue;
                  snprintf(file, sizeof(file), "%s/%s", *pp, l[i]);
                  p = strdup(file);
                  if (p)
                     list[ntot++] = p;
               }
          }
        __imlib_FileFreeDirList(l, num);
        if (!list)
           goto done;
     }

   *num_ret = ntot;

 done:
   return list;
}

char               *
__imlib_ModuleFind(char **ppath, const char *name)
{
   int                 n;
   char              **pp;
   char                nbuf[4096];

   if (!ppath)
      return NULL;

   for (pp = ppath; *pp; pp++)
     {
        n = snprintf(nbuf, sizeof(nbuf), "%s/%s.so", *pp, name);

        if (n < 0 || n >= (int)sizeof(nbuf) || !__imlib_FileIsFile(nbuf))
           continue;

        return strdup(nbuf);
     }

   return NULL;
}
