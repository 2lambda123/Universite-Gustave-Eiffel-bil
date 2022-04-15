#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Model.h"
#include "Models.h"

#include "Message.h"
#include "Geometry.h"
#include "DataFile.h"
#include "ObVals.h"
#include "Views.h"
#include "Mry.h"


extern  Model_SetModelProp_t  Models_ListOfSetModelProp ;



/*
static int Return0(void) ;
int Return0(void)
{
  return(0) ;
}
*/




Model_t* (Model_New)(void)
{
  Model_t* model = (Model_t*) Mry_New(Model_t) ;
  
  
  {
    /* Allocation of space for name of equations */
    {
      char** names = (char**) Mry_New(char*,Model_MaxNbOfEquations) ;
      
      Model_GetNameOfEquation(model) = names ;
    }
      
    {
      char* name = (char*) Mry_New(char,Model_MaxLengthOfKeyWord*Model_MaxNbOfEquations) ;
    
      {
        int j ;
      
        for(j = 0 ; j < Model_MaxNbOfEquations ; j++) {
          Model_GetNameOfEquation(model)[j] = name + j*Model_MaxLengthOfKeyWord ;
        }
      }
    }
    
    
    /* Allocation of space for name of unknowns */
    {
      char** names = (char**) Mry_New(char*,Model_MaxNbOfEquations) ;
    
      Model_GetNameOfUnknown(model) = names ;
    }
      
    {
      char* name = (char*) Mry_New(char,Model_MaxLengthOfKeyWord*Model_MaxNbOfEquations) ;
    
      {
        int j ;
      
        for(j = 0 ; j < Model_MaxNbOfEquations ; j++) {
          Model_GetNameOfUnknown(model)[j]  = name + j*Model_MaxLengthOfKeyWord ;
        }
      }
    }
    
    
    /* Allocation of space for the sequential indexes of unknowns/equations */
    {
      int* ind = (int*) Mry_New(int,Model_MaxNbOfEquations) ;
    
      Model_GetSequentialIndexOfUnknown(model) = ind ;
    }
    
    
    /* Allocation of space for code name of the model */
    {
      char* name = (char*) Mry_New(char,Model_MaxLengthOfKeyWord) ;
      
      Model_GetCodeNameOfModel(model) = name ;
    }
  
    
    /* Allocation of space for short title of the model */
    {
      char* name = (char*) Mry_New(char,Model_MaxLengthOfShortTitle) ;
      
      Model_GetShortTitle(model) = name ;
    
      Model_CopyShortTitle(model,"\0") ;
    }
  
    
    /* Allocation of space for name of authors */
    {
      char* name = (char*) Mry_New(char,Model_MaxLengthOfAuthorNames) ;
    
      Model_GetNameOfAuthors(model) = name ;
      
      Model_CopyNameOfAuthors(model,"\0") ;
    }
    
    
    /* Allocation of space for objective values */
    {
      ObVal_t* obval = (ObVal_t*) Mry_New(ObVal_t,Model_MaxNbOfEquations) ;
    
      Model_GetObjectiveValue(model) = obval ;
    }
    
    
    /* Allocation of space for views */
    {
      Views_t* views = Views_Create(Model_MaxNbOfViews) ;
      
      Model_GetViews(model) = views ;
    }
  
  
    /* Allocation of space for the local variables */
    {
      int nvar = Model_MaxNbOfVariables ;
      LocalVariableVectors_t* lvv = LocalVariableVectors_Create(nvar) ;
    
      Model_GetLocalVariableVectors(model) = lvv ;
    }
  
  
    /* Allocation of space for the local fluxes */
    {
      int nvar = Model_MaxNbOfVariableFluxes ;
      LocalVariableVectors_t* lvv = LocalVariableVectors_Create(nvar) ;
    
      Model_GetLocalFluxVectors(model) = lvv ;
    }
  }
  
  /* Default initialization of methods (to be checked) */
  #if 0
  {
    Model_GetSetModelProp(model)             = Return0 ;
    Model_GetReadMaterialProperties(model)   = Return0 ;
    Model_GetPrintModelProp(model)           = Return0 ;
    Model_GetDefineElementProperties(model)  = Return0 ;
    Model_GetComputeInitialState(model)      = Return0 ;
    Model_GetComputeExplicitTerms(model)     = Return0 ;
    Model_GetComputeImplicitTerms(model)     = Return0 ;
    Model_GetComputeMatrix(model)            = Return0 ;
    Model_GetComputeResidu(model)            = Return0 ;
    Model_GetComputeLoads(model)             = Return0 ;
    Model_GetComputeOutputs(model)           = Return0 ;
    Model_GetComputePropertyIndex(model)     = Return0 ;
  }
  #endif
  
  return(model) ;
}



void  (Model_Delete)(void* self)
{
  Model_t* model = (Model_t*) self ;
  
  {
    free(Model_GetNameOfEquation(model)[0]) ;
    free(Model_GetNameOfEquation(model)) ;
    free(Model_GetNameOfUnknown(model)[0]) ;
    free(Model_GetNameOfUnknown(model)) ;
    free(Model_GetSequentialIndexOfUnknown(model)) ;
    free(Model_GetCodeNameOfModel(model)) ;
    free(Model_GetShortTitle(model)) ;
    free(Model_GetNameOfAuthors(model)) ;
    free(Model_GetObjectiveValue(model)) ;
    {
      Views_t* views = Model_GetViews(model) ;
      
      Views_Delete(views) ;
      free(views) ;
    }
    {
      LocalVariableVectors_t* lvv = Model_GetLocalVariableVectors(model) ;
      
      LocalVariableVectors_Delete(lvv) ;
      free(lvv) ;
    }
    {
      LocalVariableVectors_t* lvv = Model_GetLocalFluxVectors(model) ;
      
      LocalVariableVectors_Delete(lvv) ;
      free(lvv) ;
    }
  }
}



Model_t* (Model_Initialize)(Model_t* model,const char* codename,Geometry_t* geom,DataFile_t* datafile)
{
  int n_models = Models_NbOfModels ;
  const char* modelnames[] = {Models_ListOfNames} ;
  Model_SetModelProp_t* xModel_SetModelProp[] = {Models_ListOfSetModelProp} ;
  int i = 0 ;
  
  Model_GetGeometry(model) = geom ; /* Important! */
  Model_GetDataFile(model) = datafile ;
  
  while(i < n_models && strcmp(modelnames[i],codename)) i++ ;
    
  if(i < n_models) {
    Model_CopyCodeNameOfModel(model,modelnames[i]) ;
    Model_GetSetModelProp(model) = xModel_SetModelProp[i] ;
    Model_SetModelProp(model) ; /* Call to SetModelProp */
    
    return(model) ;
  }
  
  //return(NULL) ;
  return(model) ;
}



void (Model_Scan)(Model_t* model,DataFile_t* datafile,Geometry_t* geom)
{
  char* line = DataFile_ReadLineFromCurrentFilePositionInString(datafile) ;
  
  /* Code name of the model */
  {
    char   codename[Model_MaxLengthOfKeyWord] ;
    int n = String_FindAndScanExp(line,"Name",","," = %s",codename) ;
    
    if(n) {
      Model_Initialize(model,codename,geom,datafile) ;
    } else {
      arret("Model_Scan") ;
    }
  }
      
  /* Name of equations */
  {
    int n = String_FindAndScanExp(line,"Equations",","," = ") ;
        
    if(n) {
      int neq = Model_GetNbOfEquations(model) ;
      char* pline = String_GetAdvancedPosition ;
      int i ;
          
      for(i = 0 ; i < neq ; i++) {
        char  name[Model_MaxLengthOfKeyWord] ;
        
        pline += String_Scan(pline,"%s",name) ;
        
        Model_CopyNameOfEquation(model,i,name) ;
      }
    }
  }
      
  /* Name of unknowns */
  {
    int n = String_FindAndScanExp(line,"Unknowns",","," = ") ;
        
    if(n) {
      int neq = Model_GetNbOfEquations(model) ;
      char* pline = String_GetAdvancedPosition ;
      int i ;
          
      for(i = 0 ; i < neq ; i++) {
        char  name[Model_MaxLengthOfKeyWord] ;
        
        pline += String_Scan(pline,"%s",name) ;
        
        Model_CopyNameOfUnknown(model,i,name) ;
      }
    }
  }
}




double* (Model_ComputeVariableDerivatives)(Element_t* el,double t,double dt,double dxi,int i,int n)
{
  Model_t* model = Element_GetModel(el) ;
  int NbOfVariables = Model_GetNbOfVariables(model) ;
  
  if(NbOfVariables > Model_MaxNbOfVariables) {
    arret("Model_ComputeVariableDerivatives") ;
  }
  
  {
    double* x  = Model_GetVariable(model,n) ;
    double* dx = Model_GetVariableDerivative(model,n) ;
    int j ;
  
    for(j = 0 ; j < NbOfVariables ; j++) {
      dx[j] = x[j] ;
    }
  
    dx[i] += dxi ;
  
    {
      Model_ComputeSecondaryVariables_t* computesecondaryvariables = Model_GetComputeSecondaryVariables(model) ;
      
      computesecondaryvariables(el,t,dt,dx) ;
    }
  
    for(j = 0 ; j < NbOfVariables ; j++) {
      dx[j] -= x[j] ;
      dx[j] /= dxi ;
    }

    return(dx) ;
  }
}
