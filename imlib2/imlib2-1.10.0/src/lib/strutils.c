#include <stdlib.h>
#include <string.h>

#include "strutils.h"

/*
 * Create NULL terminated argv-like list of tokens in
 * str separated by delim
 */
char              **
__imlib_StrSplit(const char *str, int delim)
{
   const char         *s, *p;
   char              **lst;
   int                 n, len;

   if (delim == '\0')
      return NULL;

   lst = NULL;
   n = 0;
   for (s = str; s; s = p)
     {
        p = strchr(s, delim);
        if (p && delim != '\0')
          {
             len = p - s;
             p++;
          }
        else
          {
             len = strlen(s);
          }
        if (len <= 0)
           continue;

        lst = realloc(lst, (n + 2) * sizeof(char *));

        lst[n++] = strndup(s, len);
     }

   if (lst)
      lst[n] = NULL;

   return lst;
}

void
__imlib_StrSplitFree(char **lst)
{
   char              **l = lst;

   if (!l)
      return;

   for (; *l; l++)
      free(*l);
   free(lst);
}
