#ifndef BASENAME_SETMODELPROP_H
#define BASENAME_SETMODELPROP_H


#include "Utils.h"

#define BASENAME_SETMODELPROP(base)   (Utils_CAT(base,_SetModelProp))

/* The macro BASENAME is sent from the compiler */
#ifdef  BASENAME
  #define BaseName_SetModelProp         BASENAME_SETMODELPROP(BASENAME)
#else
  #error "The macro BASENAME is not defined!"
#endif

#endif
