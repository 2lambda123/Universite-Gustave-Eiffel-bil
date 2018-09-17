#ifndef RESOLUTIONMETHOD_H
#define RESOLUTIONMETHOD_H


enum ResolutionMethod_e {     /* Type of resolution method */
  ResolutionMethod_CROUT,     /* Crout method */
  ResolutionMethod_SLU,       /* SuperLU method*/
  ResolutionMethod_MA38       /* MA38 method*/
} ;


/* class-like structures "ResolutionMethod_t" and attributes */

/* vacuous declarations and typedef names */
typedef enum ResolutionMethod_e  ResolutionMethod_t ;



#endif