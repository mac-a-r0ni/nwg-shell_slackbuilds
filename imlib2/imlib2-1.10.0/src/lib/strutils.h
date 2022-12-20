#ifndef STRUTILS_H
#define STRUTILS_H

char              **__imlib_StrSplit(const char *str, int delim);
void                __imlib_StrSplitFree(char **plist);

#endif /* STRUTILS_H */
