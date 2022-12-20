#include "config.h"
#include "Imlib2_Loader.h"

#include <errno.h>
#include <limits.h>
#include <id3tag.h>

#define USE_TAGS 0

static const char  *const _formats[] = { "mp3" };

typedef struct context {
   int                 id;
   char               *filename;
   struct id3_tag     *tag;
   int                 refcount;
   struct context     *next;
} context;

static context     *id3_ctxs = NULL;

static struct id3_frame *
id3_tag_get_frame(struct id3_tag *tag, size_t index)
{
   return tag->frames[index];
}

static unsigned int
id3_tag_get_numframes(struct id3_tag *tag)
{
   return tag->nframes;
}

static char const  *
id3_frame_id(struct id3_frame *frame)
{
   return frame->id;
}

static context     *
context_create(const char *filename, FILE * f)
{
   context            *node = (context *) malloc(sizeof(context));
   context            *ptr, *last;
   int                 last_id = INT_MAX;

   node->refcount = 1;
   {
      int                 fd;
      struct id3_file    *file;
      struct id3_tag     *tag;
      unsigned int        i;

      fd = dup(fileno(f));
      file = id3_file_fdopen(fd, ID3_FILE_MODE_READONLY);
      if (!file)
        {
           close(fd);
           fprintf(stderr, "Unable to open tagged file %s: %s\n",
                   filename, strerror(errno));
           goto fail_free;
        }
      tag = id3_file_tag(file);
      if (!tag)
        {
           fprintf(stderr, "Unable to find ID3v2 tags in file %s\n", filename);
           id3_file_close(file);
           goto fail_free;
        }
      node->tag = id3_tag_new();
      for (i = 0; i < id3_tag_get_numframes(tag); i++)
         if (!strcmp(id3_frame_id(id3_tag_get_frame(tag, i)), "APIC"))
            id3_tag_attachframe(node->tag, id3_tag_get_frame(tag, i));
      id3_file_close(file);
   }

   node->filename = strdup(filename);

   if (!id3_ctxs)
     {
        node->id = 1;
        node->next = NULL;
        id3_ctxs = node;
        return node;
     }
   ptr = id3_ctxs;

   last = NULL;
   while (ptr && (ptr->id + 1) >= last_id)
     {
        last_id = ptr->id;
        last = ptr;
        ptr = ptr->next;
     }

   /* Paranoid! this can occur only if there are INT_MAX contexts :) */
   if (!ptr)
     {
        fprintf(stderr, "Too many open ID3 contexts\n");
        goto fail_close;
     }

   node->id = ptr->id + 1;

   if (last)
     {
        node->next = last->next;
        last->next = node;
     }
   else
     {
        node->next = id3_ctxs;
        id3_ctxs = node;
     }
   return node;

 fail_close:
   free(node->filename);
   id3_tag_delete(node->tag);
 fail_free:
   free(node);
   return NULL;
}

static void
context_destroy(context * ctx)
{
   id3_tag_delete(ctx->tag);
   free(ctx->filename);
   free(ctx);
}

static void
context_addref(context * ctx)
{
   ctx->refcount++;
}

static context     *
context_get(int id)
{
   context            *ptr = id3_ctxs;

   while (ptr)
     {
        if (ptr->id == id)
          {
             context_addref(ptr);
             return ptr;
          }
        ptr = ptr->next;
     }
   fprintf(stderr, "No context by handle %d found\n", id);
   return NULL;
}

static context     *
context_get_by_name(const char *name)
{
   context            *ptr = id3_ctxs;

   while (ptr)
     {
        if (!strcmp(name, ptr->filename))
          {
             context_addref(ptr);
             return ptr;
          }
        ptr = ptr->next;
     }
   return NULL;
}

static void
context_delref(context * ctx)
{
   ctx->refcount--;
   if (ctx->refcount <= 0)
     {
        context            *last = NULL, *ptr = id3_ctxs;

        while (ptr)
          {
             if (ptr == ctx)
               {
                  if (last)
                     last->next = ctx->next;
                  else
                     id3_ctxs = ctx->next;
                  context_destroy(ctx);
                  return;
               }
             last = ptr;
             ptr = ptr->next;
          }
     }
}

static int
str2int(const char *str, int old)
{
   long                index;

   errno = 0;
   index = strtol(str, NULL, 10);
   return ((errno || index > INT_MAX) ? old : (int)index);
}

static unsigned int
str2uint(const char *str, unsigned int old)
{
   unsigned long       index;

   errno = 0;
   index = strtoul(str, NULL, 10);
   return ((errno || index > UINT_MAX) ? old : index);
}

#if USE_TAGS
static void
destructor_data(ImlibImage * im, void *data)
{
   free(data);
}

static void
destructor_context(ImlibImage * im, void *data)
{
   context_delref((context *) data);
}
#endif

typedef struct lopt {
   context            *ctx;
   unsigned int        index;
   int                 traverse;
   char                cache_level;
} lopt;

static char
get_options(lopt * opt, const ImlibImage * im)
{
   unsigned int        handle = 0, index = 0, traverse = 0;
   context            *ctx;
   const char         *str;

   str = __imlib_GetKey(im);
   if (str)
     {
        char               *key = strdup(str);
        char               *tok = strtok(key, ",");

        traverse = 0;
        while (tok)
          {
             char               *value = strchr(tok, '=');

             if (!value)
               {
                  value = tok;
                  tok = (char *)"index";
               }
             else
               {
                  *value = '\0';
                  value++;
               }
             if (!strcasecmp(tok, "index"))
                index = str2uint(value, index);
             else if (!strcasecmp(tok, "context"))
                handle = str2uint(value, handle);
             else if (!strcasecmp(tok, "traverse"))
                traverse = str2int(value, traverse);
             tok = strtok(NULL, ",");
          }
        free(key);
     }
   else
      traverse = 1;

   if (!handle)
     {
        ImlibImageTag      *htag = __imlib_GetTag(im, "context");

        if (htag && htag->val)
           handle = htag->val;
     }
   if (handle)
      ctx = context_get(handle);
   else if (!(ctx = context_get_by_name(im->fi->name)) &&
            !(ctx = context_create(im->fi->name, im->fi->fp)))
      return 0;

   if (!index)
     {
        ImlibImageTag      *htag = __imlib_GetTag(im, "index");

        if (htag && htag->val)
           index = htag->val;
     }
   if (index > id3_tag_get_numframes(ctx->tag) ||
       (index == 0 && id3_tag_get_numframes(ctx->tag) < 1))
     {
        if (index)
           fprintf(stderr, "No picture frame # %d found\n", index);
        context_delref(ctx);
        return 0;
     }
   if (!index)
      index = 1;

   opt->ctx = ctx;
   opt->index = index;
   opt->traverse = traverse;
   opt->cache_level = (id3_tag_get_numframes(ctx->tag) > 1 ? 1 : 0);
   return 1;
}

static int
extract_pic(struct id3_frame *frame, int dest)
{
   union id3_field    *field;
   unsigned char const *data;
   id3_length_t        length;
   int                 done = 0;

   field = id3_frame_field(frame, 4);
   data = id3_field_getbinarydata(field, &length);
   if (!data)
     {
        fprintf(stderr, "No image data found for frame\n");
        return 0;
     }
   while (length > 0)
     {
        ssize_t             res;

        if ((res = write(dest, data + done, length)) < 0)
          {
             if (errno == EINTR)
                continue;
             perror("Unable to write to file");
             return 0;
          }
        length -= res;
        done += res;
     }
   return 1;
}

#define EXT_LEN 14

static char
get_loader(lopt * opt, ImlibLoader ** loader)
{
   union id3_field    *field;
   char const         *data;
   char                ext[EXT_LEN + 2];

   ext[EXT_LEN + 1] = '\0';
   ext[0] = '.';

   field = id3_frame_field(id3_tag_get_frame(opt->ctx->tag, opt->index - 1), 1);
   data = (char const *)id3_field_getlatin1(field);
   if (!data)
     {
        fprintf(stderr, "No mime type data found for image frame\n");
        return 0;
     }
   if (strncasecmp(data, "image/", 6))
     {
        if (!strcmp(data, "-->"))
          {
             *loader = NULL;
             return 1;
          }
        fprintf(stderr,
                "Picture frame with unknown mime-type \'%s\' found\n", data);
        return 0;
     }
   strncpy(ext + 1, data + 6, EXT_LEN);
   if (!(*loader = __imlib_FindBestLoader(ext, NULL, 0)))
     {
        fprintf(stderr, "No loader found for extension %s\n", ext);
        return 0;
     }
   return 1;
}

#if USE_TAGS
static const char  *const id3_pic_types[] = {
   /* $00 */ "Other",
   /* $01 */ "32x32 pixels file icon",
   /* $02 */ "Other file icon",
   /* $03 */ "Cover (front)",
   /* $04 */ "Cover (back)",
   /* $05 */ "Leaflet page",
   /* $06 */ "Media",
   /* $07 */ "Lead artist/lead performer/soloist",
   /* $08 */ "Artist/performer",
   /* $09 */ "Conductor",
   /* $0A */ "Band/Orchestra",
   /* $0B */ "Composer",
   /* $0C */ "Lyricist/text writer",
   /* $0D */ "Recording Location",
   /* $0E */ "During recording",
   /* $0F */ "During performance",
   /* $10 */ "Movie/video screen capture",
   /* $11 */ "A bright coloured fish",
   /* $12 */ "Illustration",
   /* $13 */ "Band/artist logotype",
   /* $14 */ "Publisher/Studio logotype"
};

#define NUM_OF_ID3_PIC_TYPES \
    (ARRAY_SIZE(id3_pic_types))

static const char  *const id3_text_encodings[] = {
   /* $00 */ "ISO-8859-1",
   /* $01 */ "UTF-16 encoded Unicode with BOM",
   /* $02 */ "UTF-16BE encoded Unicode without BOM",
   /* $03 */ "UTF-8 encoded Unicode"
};

#define NUM_OF_ID3_TEXT_ENCODINGS \
    (ARRAY_SIZE(id3_text_encodings))

static void
write_tags(ImlibImage * im, lopt * opt)
{
   struct id3_frame   *frame = id3_tag_get_frame(opt->ctx->tag, opt->index - 1);
   union id3_field    *field;
   unsigned int        num_data;
   char               *data;

   if ((field = id3_frame_field(frame, 1)) &&
       (data = (char *)id3_field_getlatin1(field)))
      __imlib_AttachTag(im, "mime-type", 0, strdup(data), destructor_data);
   if ((field = id3_frame_field(frame, 3)) &&
       (data = (char *)id3_field_getstring(field)))
     {
        size_t              length;
        char               *dup;
        id3_ucs4_t         *ptr = (id3_ucs4_t *) data;

        while (*ptr)
           ptr++;
        length = (ptr - (id3_ucs4_t *) data + 1) * sizeof(id3_ucs4_t);
        dup = (char *)malloc(length);
        memcpy(dup, data, length);
        __imlib_AttachTag(im, "id3-description", 0, dup, destructor_data);
     }
   if ((field = id3_frame_field(frame, 0)))
     {
        num_data = id3_field_gettextencoding(field);
        __imlib_AttachTag(im, "id3-description-text-encoding", num_data,
                          num_data < NUM_OF_ID3_TEXT_ENCODINGS ?
                          (char *)id3_text_encodings[num_data] : NULL, NULL);
     }
   if ((field = id3_frame_field(frame, 2)))
     {
        num_data = id3_field_getint(field);
        __imlib_AttachTag(im, "id3-picture-type", num_data,
                          num_data < NUM_OF_ID3_PIC_TYPES ?
                          (char *)id3_pic_types[num_data] : NULL, NULL);
     }
   __imlib_AttachTag(im, "count", id3_tag_get_numframes(opt->ctx->tag),
                     NULL, NULL);
   if (opt->cache_level)
     {
        context_addref(opt->ctx);
        __imlib_AttachTag(im, "context", opt->ctx->id,
                          opt->ctx, destructor_context);
     }
   __imlib_AttachTag(im, "index", opt->index, NULL, NULL);
   if (opt->traverse)
     {
        char               *buf = NULL;

        if ((opt->index + opt->traverse)
            <= id3_tag_get_numframes(opt->ctx->tag)
            && (opt->index + opt->traverse) > 0)
          {
             buf = (char *)malloc((strlen(im->fi->name) + 50) * sizeof(char));
             sprintf(buf, "%s:index=%d,traverse=%d", im->fi->name,
                     opt->index + opt->traverse, opt->traverse);
          }
        __imlib_AttachTag(im, "next", 0, buf, destructor_data);
     }
}
#endif

static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   ImlibLoader        *loader;
   lopt                opt;

   rc = LOAD_FAIL;
   opt.ctx = NULL;

   if (!im->fi->fp)
      return rc;

   if (!get_options(&opt, im))
      goto quit;

   rc = LOAD_BADIMAGE;          /* Format accepted */

   if (!get_loader(&opt, &loader))
      goto quit;

   if (loader)
     {
        char                tmp[] = "/tmp/imlib2_loader_id3-XXXXXX";
        int                 dest, res;

        if ((dest = mkstemp(tmp)) < 0)
          {
             fprintf(stderr, "Unable to create a temporary file\n");
             goto quit;
          }

        res = extract_pic(id3_tag_get_frame(opt.ctx->tag, opt.index - 1), dest);
        close(dest);

        if (!res)
          {
             unlink(tmp);
             goto quit;
          }

        rc = __imlib_LoadEmbedded(loader, im, load_data, tmp);

        unlink(tmp);
     }
   else
     {
        /* The tag actually provides a image url rather than image data.
         * Practically, dunno if such a tag exists on earth :)
         * Here's the code anyway...
         */
        union id3_field    *field;
        id3_length_t        length;
        char const         *data;
        char               *url, *file;

        field = id3_frame_field
           (id3_tag_get_frame(opt.ctx->tag, opt.index - 1), 4);
        data = (char const *)id3_field_getbinarydata(field, &length);
        if (!data || !length)
          {
             fprintf(stderr, "No link image URL present\n");
             goto quit;
          }
        url = (char *)malloc((length + 1) * sizeof(char));
        strncpy(url, data, length);
        url[length] = '\0';
        file = (strncmp(url, "file://", 7) ? url : url + 7);
        if (!(loader = __imlib_FindBestLoader(file, NULL, 0)))
          {
             fprintf(stderr, "No loader found for file %s\n", file);
             free(url);
             goto quit;
          }

        rc = __imlib_LoadEmbedded(loader, im, load_data, file);

#if USE_TAGS
        if (!im->loader)
           __imlib_AttachTag(im, "id3-link-url", 0, url, destructor_data);
        else
#endif
           free(url);
     }

#if USE_TAGS
   if (!im->loader)
      write_tags(im, &opt);
#endif

#ifdef DEBUG
   if (!im->loader)
     {
        ImlibImageTag      *cur = im->tags;

        fprintf(stderr, "Tags for file %s:\n", im->file);
        while (cur)
          {
             fprintf(stderr, "\t%s: (%d) %s\n", cur->key,
                     cur->val, (char *)cur->data);
             cur = cur->next;
          }
     }
#endif

 quit:
   if (opt.ctx)
      context_delref(opt.ctx);

   return rc;
}

IMLIB_LOADER(_formats, _load, NULL);
