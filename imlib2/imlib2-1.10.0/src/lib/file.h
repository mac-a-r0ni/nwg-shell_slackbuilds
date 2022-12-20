#ifndef __FILE_H
#define __FILE_H 1

#include <time.h>
#include <sys/stat.h>

int                 __imlib_IsRealFile(const char *s);
char               *__imlib_FileKey(const char *file);
char               *__imlib_FileRealFile(const char *file);

const char         *__imlib_FileExtension(const char *file);

int                 __imlib_FileStat(const char *file, struct stat *st);

static inline       time_t
__imlib_StatModDate(const struct stat *st)
{
   return (st->st_mtime > st->st_ctime) ? st->st_mtime : st->st_ctime;
}

static inline int
__imlib_StatIsFile(const struct stat *st)
{
   return S_ISREG(st->st_mode);
}

static inline int
__imlib_StatIsDir(const struct stat *st)
{
   return S_ISDIR(st->st_mode);
}

int                 __imlib_FileExists(const char *s);
int                 __imlib_FileIsFile(const char *s);
int                 __imlib_FileIsDir(const char *s);
time_t              __imlib_FileModDate(const char *s);
time_t              __imlib_FileModDateFd(int fd);
int                 __imlib_FileCanRead(const char *s);

char              **__imlib_FileDir(const char *dir, int *num);
void                __imlib_FileFreeDirList(char **l, int num);

void                __imlib_FileDel(const char *s);
char               *__imlib_FileHomeDir(int uid);
int                 __imlib_ItemInList(char **list, int size, char *item);

char              **__imlib_PathToFilters(void);
char              **__imlib_PathToLoaders(void);
char              **__imlib_ModulesList(char **path, int *num_ret);
char               *__imlib_ModuleFind(char **path, const char *name);

#endif
