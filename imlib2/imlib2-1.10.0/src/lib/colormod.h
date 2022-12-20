#ifndef __COLORMOD
#define __COLORMOD 1

#include "types.h"

struct _ImlibColorModifier {
   uint8_t             red_mapping[256];
   uint8_t             green_mapping[256];
   uint8_t             blue_mapping[256];
   uint8_t             alpha_mapping[256];
   uint64_t            modification_count;
};

#define CMOD_APPLY_RGB(cm, r, g, b) \
(r) = (cm)->red_mapping[(int)(r)]; \
(g) = (cm)->green_mapping[(int)(g)]; \
(b) = (cm)->blue_mapping[(int)(b)];

#define CMOD_APPLY_RGBA(cm, r, g, b, a) \
(r) = (cm)->red_mapping[(int)(r)]; \
(g) = (cm)->green_mapping[(int)(g)]; \
(b) = (cm)->blue_mapping[(int)(b)]; \
(a) = (cm)->alpha_mapping[(int)(a)];

#define CMOD_APPLY_R(cm, r) \
(r) = (cm)->red_mapping[(int)(r)];
#define CMOD_APPLY_G(cm, g) \
(g) = (cm)->green_mapping[(int)(g)];
#define CMOD_APPLY_B(cm, b) \
(b) = (cm)->blue_mapping[(int)(b)];
#define CMOD_APPLY_A(cm, a) \
(a) = (cm)->alpha_mapping[(int)(a)];

#define R_CMOD(cm, r) \
(cm)->red_mapping[(int)(r)]
#define G_CMOD(cm, g) \
(cm)->green_mapping[(int)(g)]
#define B_CMOD(cm, b) \
(cm)->blue_mapping[(int)(b)]
#define A_CMOD(cm, a) \
(cm)->alpha_mapping[(int)(a)]

ImlibColorModifier *__imlib_CreateCmod(void);
void                __imlib_FreeCmod(ImlibColorModifier * cm);
void                __imlib_CmodChanged(ImlibColorModifier * cm);
void                __imlib_CmodSetTables(ImlibColorModifier * cm, uint8_t * r,
                                          uint8_t * g, uint8_t * b,
                                          uint8_t * a);
void                __imlib_CmodReset(ImlibColorModifier * cm);
void                __imlib_DataCmodApply(uint32_t * data, int w, int h,
                                          int jump, bool has_alpha,
                                          ImlibColorModifier * cm);

void                __imlib_CmodGetTables(ImlibColorModifier * cm, uint8_t * r,
                                          uint8_t * g, uint8_t * b,
                                          uint8_t * a);
void                __imlib_CmodModBrightness(ImlibColorModifier * cm,
                                              double v);
void                __imlib_CmodModContrast(ImlibColorModifier * cm, double v);
void                __imlib_CmodModGamma(ImlibColorModifier * cm, double v);
#endif
