#ifndef OBJECT_H
#define OBJECT_H

typedef struct _Imlib_Object_List {
   struct _Imlib_Object_List *next, *prev;
} Imlib_Object_List;

typedef struct {
   int                 population;
   Imlib_Object_List  *buckets[256];
} Imlib_Hash;

typedef int         (Imlib_Hash_Func) (Imlib_Hash * hash, const char *key,
                                       void *data, void *fdata);

void               *__imlib_object_list_prepend(void *in_list, void *in_item);
void               *__imlib_object_list_remove(void *in_list, void *in_item);

Imlib_Hash         *__imlib_hash_add(Imlib_Hash * hash, const char *key,
                                     const void *data);
void               *__imlib_hash_find(Imlib_Hash * hash, const char *key);
void                __imlib_hash_free(Imlib_Hash * hash);
void                __imlib_hash_foreach(Imlib_Hash * hash,
                                         Imlib_Hash_Func * func,
                                         const void *fdata);

#endif /* OBJECT_H */
