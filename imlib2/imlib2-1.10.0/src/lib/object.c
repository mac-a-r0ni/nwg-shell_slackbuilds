#include "common.h"

#include <stdlib.h>
#include <string.h>

#include "object.h"

typedef struct {
   Imlib_Object_List   _list_data;
   char               *key;
   void               *data;
} Imlib_Hash_El;

static int          _imlib_list_alloc_error = 0;
static int          _imlib_hash_alloc_error = 0;

static int
__imlib_list_alloc_error(void)
{
   return _imlib_list_alloc_error;
}

void               *
__imlib_object_list_prepend(void *in_list, void *in_item)
{
   Imlib_Object_List  *new_l;
   Imlib_Object_List  *list, *item;

   list = in_list;
   item = in_item;
   new_l = item;
   new_l->prev = NULL;
   if (!list)
     {
        new_l->next = NULL;
        return new_l;
     }
   new_l->next = list;
   list->prev = new_l;

   return new_l;
}

void               *
__imlib_object_list_remove(void *in_list, void *in_item)
{
   Imlib_Object_List  *return_l;
   Imlib_Object_List  *list = in_list;
   Imlib_Object_List  *item = in_item;

   /* checkme */
   if (!list)
      return list;
   if (!item)
      return list;

   if (item->next)
      item->next->prev = item->prev;

   if (item->prev)
     {
        item->prev->next = item->next;
        return_l = list;
     }
   else
     {
        return_l = item->next;
     }

   item->next = NULL;
   item->prev = NULL;

   return return_l;
}

static int
__imlib_hash_gen(const char *key)
{
   unsigned int        hash_num = 0;
   const unsigned char *ptr;

   if (!key)
      return 0;

   for (ptr = (unsigned char *)key; *ptr; ptr++)
      hash_num ^= (int)(*ptr);

   hash_num &= 0xff;
   return (int)hash_num;
}

Imlib_Hash         *
__imlib_hash_add(Imlib_Hash * hash, const char *key, const void *data)
{
   int                 hash_num;
   Imlib_Hash_El      *el;

   _imlib_hash_alloc_error = 0;

   if (!hash)
     {
        hash = calloc(1, sizeof(Imlib_Hash));
        if (!hash)
          {
             _imlib_hash_alloc_error = 1;
             return NULL;
          }
     }

   if (!(el = malloc(sizeof(Imlib_Hash_El))))
     {
        if (hash->population <= 0)
          {
             free(hash);
             hash = NULL;
          }
        _imlib_hash_alloc_error = 1;
        return hash;
     }

   if (key)
     {
        el->key = strdup(key);
        if (!el->key)
          {
             free(el);
             _imlib_hash_alloc_error = 1;
             return hash;
          }
        hash_num = __imlib_hash_gen(key);
     }
   else
     {
        el->key = NULL;
        hash_num = 0;
     }

   el->data = (void *)data;

   hash->buckets[hash_num] =
      __imlib_object_list_prepend(hash->buckets[hash_num], el);

   if (__imlib_list_alloc_error())
     {
        _imlib_hash_alloc_error = 1;
        free(el->key);
        free(el);
        return hash;
     }

   hash->population++;

   return hash;
}

void               *
__imlib_hash_find(Imlib_Hash * hash, const char *key)
{
   int                 hash_num;
   Imlib_Hash_El      *el;
   Imlib_Object_List  *l;

   _imlib_hash_alloc_error = 0;

   if (!hash)
      return NULL;

   hash_num = __imlib_hash_gen(key);
   for (l = hash->buckets[hash_num]; l; l = l->next)
     {
        el = (Imlib_Hash_El *) l;
        if (((el->key) && (key) && (!strcmp(el->key, key)))
            || ((!el->key) && (!key)))
          {
             if (l != hash->buckets[hash_num])
               {
                  /* FIXME: move to front of list without alloc */
                  hash->buckets[hash_num] =
                     __imlib_object_list_remove(hash->buckets[hash_num], el);
                  hash->buckets[hash_num] =
                     __imlib_object_list_prepend(hash->buckets[hash_num], el);
                  if (__imlib_list_alloc_error())
                    {
                       _imlib_hash_alloc_error = 1;
                       return el->data;
                    }
               }
             return el->data;
          }
     }

   return NULL;
}

static int
__imlib_hash_size(Imlib_Hash * hash)
{
   if (!hash)
      return 0;
   return 256;
}

void
__imlib_hash_free(Imlib_Hash * hash)
{
   int                 i, size;

   if (!hash)
      return;

   size = __imlib_hash_size(hash);
   for (i = 0; i < size; i++)
     {
        Imlib_Hash_El      *el, *el_next;

        for (el = (Imlib_Hash_El *) hash->buckets[i]; el; el = el_next)
          {
             el_next = (Imlib_Hash_El *) el->_list_data.next;
             free(el->key);
             free(el);
          }
     }
   free(hash);
}

void
__imlib_hash_foreach(Imlib_Hash * hash,
                     Imlib_Hash_Func * func, const void *fdata)
{
   int                 i, size;

   if (!hash)
      return;

   size = __imlib_hash_size(hash);
   for (i = 0; i < size; i++)
     {
        Imlib_Object_List  *l, *next_l;

        for (l = hash->buckets[i]; l;)
          {
             Imlib_Hash_El      *el;

             next_l = l->next;
             el = (Imlib_Hash_El *) l;
             if (!func(hash, el->key, el->data, (void *)fdata))
                return;
             l = next_l;
          }
     }
}
