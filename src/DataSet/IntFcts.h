#ifndef INTFCTS_H
#define INTFCTS_H


/* vacuous declarations and typedef names */

/* class-like structure "DataSet_t and attributes */
struct IntFcts_s      ; typedef struct IntFcts_s      IntFcts_t ;

/*   1. IntFcts_t attributes */
struct IntFct_s       ; typedef struct IntFct_s       IntFct_t ;


/* Declaration of Macros, Methods and Structures */


/* 1. IntFcts_t 
 * ------------*/
extern IntFcts_t*  (IntFcts_Create)(void) ;
extern int         (IntFcts_FindIntFct)(IntFcts_t*,int,int,const char*) ;
extern int         (IntFcts_AddIntFct)(IntFcts_t*,int,int,const char*) ;


#define IntFcts_MaxNbOfIntFcts             (4)

#define IntFcts_GetNbOfIntFcts(IFCTS)    ((IFCTS)->n_fi)
#define IntFcts_GetIntFct(IFCTS)         ((IFCTS)->fi)



struct IntFcts_s {            /* interpolations */
  unsigned int n_fi ;         /* nb of interpolation function */
  IntFct_t *fi ;              /* interpolation function */
} ;



/* 2. IntFct_t 
 * -----------*/
extern void   (IntFct_ComputeIsoShapeFctInActualSpace)(int,int,double**,double*,int,double*,double*) ;
extern int    (IntFct_ComputeFunctionIndexAtPointOfReferenceFrame)(IntFct_t*,double*) ;
extern double (IntFct_InterpolateAtPoint)(IntFct_t*,double*,int,int) ;


#define IntFct_MaxLengthOfKeyWord          (30)

#define IntFct_MaxNbOfIntPoints            (8)

#define IntFct_GetType(IFCT)             ((IFCT)->type)
#define IntFct_GetNbOfNodes(IFCT)        ((IFCT)->nn)
#define IntFct_GetNbOfFunctions(IFCT)    ((IFCT)->nn)
#define IntFct_GetNbOfPoints(IFCT)       ((IFCT)->np)
#define IntFct_GetWeight(IFCT)           ((IFCT)->w)
#define IntFct_GetFunction(IFCT)         ((IFCT)->h)
#define IntFct_GetFunctionGradient(IFCT) ((IFCT)->dh)
#define IntFct_GetDimension(IFCT)        ((IFCT)->dim)
#define IntFct_GetPointCoordinates(IFCT) ((IFCT)->a)
//#define IntFct_GetComputeValuesAtPoint(IFCT)        ((IFCT)->computevaluesatpoint)



#define IntFct_GetFunctionAtPoint(IFCT,p) \
        (IntFct_GetFunction(IFCT) + (p)*IntFct_GetNbOfNodes(IFCT))

#define IntFct_GetFunctionGradientAtPoint(IFCT,p) \
        (IntFct_GetFunctionGradient(IFCT) + \
        (p)*IntFct_GetDimension(IFCT)*IntFct_GetNbOfNodes(IFCT))




/*  Typedef names of Methods */
//typedef double* IntFct_ComputeValuesAtPoint_t(IntFct_t*,double*) ;


struct IntFct_s {             /* Interpolation function */
  char   *type ;              /* Type of the function */
  unsigned short int nn ;     /* Number of functions/nodes */
  unsigned short int np ;     /* Number of integration points */
  double* a ;                 /* Reference coordinates of integration points */
  double* w ;                 /* Weights */
  double* h ;                 /* Values of interpolation functions */
  double* dh ;                /* Values of function gradients */
  unsigned short int dim ;    /* sous-dimension (0,1,2,3) */
  //IntFct_ComputeValuesAtPoint_t* computevaluesatpoint ;
} ;


/* Old notations which should be eliminated */
#define inte_t    IntFct_t
#define MAX_PGAUSS                    IntFct_MaxNbOfIntPoints
#define fint_abs                      IntFct_ComputeIsoShapeFctInActualSpace

#endif