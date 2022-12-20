#ifndef EXIF_PARSE_H
#define EXIF_PARSE_H

#define ORIENT_TOPLEFT   1
#define ORIENT_TOPRIGHT  2
#define ORIENT_BOTRIGHT  3
#define ORIENT_BOTLEFT   4
#define ORIENT_LEFTTOP   5
#define ORIENT_RIGHTTOP  6
#define ORIENT_RIGHTBOT  7
#define ORIENT_LEFTBOT   8

typedef struct {
   unsigned char       orientation;
   char                swap_wh;
} ExifInfo;

int                 exif_parse(const void *data, unsigned int len,
                               ExifInfo * ei);

#endif /* EXIF_PARSE_H */
