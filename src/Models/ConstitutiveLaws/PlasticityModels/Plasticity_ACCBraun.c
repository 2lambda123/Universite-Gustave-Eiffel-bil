static Plasticity_ComputeTangentStiffnessTensor_t    Plasticity_CTACCBraun ;
static Plasticity_ReturnMapping_t                    Plasticity_RMACCBraun ;
static Plasticity_YieldFunction_t                    Plasticity_YFACCBraun ;
static Plasticity_FlowRules_t                        Plasticity_FRACCBraun ;
static Plasticity_SetParameters_t                    Plasticity_SPACCBraun ;

static double f_fun(Plasticity_t* plasty,const double* sig,const double pc);//ADDED
static double g_fun(Plasticity_t* plasty,const double* sig,const double pc);//ADDED
static double pc_fun(Plasticity_t* plasty,const double* sig, const double* sig_t, const double* C_inv, const double pc_t); //ADDED

static void readMatrix(int mi, double matrix[mi][mi], double input_matrix[mi*mi]) ;
static void PrintInverse(double ar[][20], int ni, int mi) ;
static void InverseOfMatrix(double matrix[][20], int order) ;



#define DBL_DIG 20
#define LDBL_DIG 20


#define Elasticity_GetInverseStiffnessTensor \
        Elasticity_GetComplianceTensor


#define Plasticity_GetACC_k(PL) \
        Plasticity_GetParameter(PL)[0]

#define Plasticity_GetACC_M(PL) \
        Plasticity_GetParameter(PL)[1]
        
#define Plasticity_GetACC_N(PL) \
        Plasticity_GetParameter(PL)[2]
        
#define Plasticity_GetInitialIsotropicTensileLimit(PL) \
        Plasticity_GetParameter(PL)[3]
        
#define Plasticity_GetACCInitialPreconsolidationPressure(PL) \
        Plasticity_GetParameter(PL)[4]

#define Plasticity_GetVolumetricStrainHardeningParameter(PL) \
        Plasticity_GetParameter(PL)[5]

#define Plasticity_GetThermalHardeningParameter(PL) \
        Plasticity_GetParameter(PL)[6]

#define Plasticity_GetACC_f_par(PL) \
        Plasticity_GetParameter(PL)[7]

#define Plasticity_GetACC_f_perp(PL) \
        Plasticity_GetParameter(PL)[8]
        

void Plasticity_SPACCBraun(Plasticity_t* plasty,...)
{
  va_list args ;
  
  va_start(args,plasty) ;
  
  {
    Plasticity_GetACC_k(plasty)                           = va_arg(args,double) ;
    Plasticity_GetACC_M(plasty)                           = va_arg(args,double) ;
    Plasticity_GetACC_N(plasty)                           = va_arg(args,double) ;
    Plasticity_GetInitialIsotropicTensileLimit(plasty)    = va_arg(args,double) ;
    Plasticity_GetACCInitialPreconsolidationPressure(plasty) = va_arg(args,double) ;
    Plasticity_GetVolumetricStrainHardeningParameter(plasty) = va_arg(args,double) ;
    Plasticity_GetThermalHardeningParameter(plasty)          = va_arg(args,double) ;
    Plasticity_GetACC_f_par(plasty)   = va_arg(args,double) ;
    Plasticity_GetACC_f_perp(plasty)   = va_arg(args,double) ;
    
    {
      double pc = Plasticity_GetACCInitialPreconsolidationPressure(plasty) ;
      
      Plasticity_GetHardeningVariable(plasty)[0] = pc ;
      //Plasticity_GetHardeningVariable(plasty)[0] = log(pc) ;
      
      Plasticity_GetTypicalSmallIncrementOfHardeningVariable(plasty)[0] = 1.e-6*pc ;
      Plasticity_GetTypicalSmallIncrementOfStress(plasty) = 1.e-6*pc ;
    }
    
  }

  va_end(args) ;
}



double Plasticity_CTACCBraun(Plasticity_t* plasty,const double* sig,const double* hardv,const double dlambda) 
/** Assymmetric Cam-Clay criterion 
 * first hardening parameter is the preconsolidation pressure
 * second hardening parameter is the isotropic tensile elastic limit
*/
{
  double m       = Plasticity_GetACC_M(plasty)  ;
  double n       = Plasticity_GetACC_N(plasty)  ;
  double k       = Plasticity_GetACC_k(plasty)   ;
  //double e0      = Plasticity_GetInitialVoidRatio(plasty) ;
  double pc0     = Plasticity_GetACCInitialPreconsolidationPressure(plasty) ;
  double* dfsds  = Plasticity_GetYieldFunctionGradient(plasty) ;
  double* dgsds  = Plasticity_GetPotentialFunctionGradient(plasty) ;
  double* hm     = Plasticity_GetHardeningModulus(plasty) ;
  double pc      = hardv[0] ;
  double ps      = Plasticity_GetInitialIsotropicTensileLimit(plasty) ;
  double beta_eps=  Plasticity_GetVolumetricStrainHardeningParameter(plasty) ;
  

  double eps_v_pl = 1/beta_eps * log(pc/pc0) ; // we need here the current absolute plastic vol strain

  double id[9] = {1,0,0,0,1,0,0,0,1} ;
  double p,q,crit ;
  
  /* 
     The yield criterion
  */
  p    = (sig[0] + sig[4] + sig[8])/3. ;
  q    = sqrt(3*Math_ComputeSecondDeviatoricStressInvariant(sig)) ;
  crit = f_fun(plasty,sig,pc) ;
  //crit = q*q*exp(k*(2*p+ps-pc)/(ps-pc))+m*m*(p+ps)*(p-pc);  //Insert here the yield function
  


  /*
    Gradients
    ---------
    dp/dsig_ij = 1/3 delta_ij = id[i]/3.
    dq/dsig_ij = 3/2 dev_ij/q 

    further equations are taken from the jupyter notebook

    OLD:
    //df/dsig_ij = 1/3 (df/dp) delta_ij + 3/2 (df/dq) dev_ij/q 
    //df/dp      = 2*p + pc - ps
    //df/dq      = 2*q/m2
    
    //df/dsig_ij = 1/3 (2*p + pc - ps) delta_ij + (3/m2) dev_ij 
  */
  {
    int    i ;
    
    for(i = 0 ; i < 9 ; i++) {
      double dev = sig[i] - p*id[i] ;
      double dpsdsig = id[i]/3. ;
      //double dqsdsig = 3/2*dev/q ; //taken from existing camclay
      double dqsdsig = 3/2*dev ; //multiply already with q and remove q in the later equation

      //paste function calculated in jupyter
      dfsds[i] = (-2*dpsdsig*k*pow(q, 2) + dpsdsig*pow(m, 2)*(pc - ps)*(2*p - pc + ps)*exp(k*(2*p - pc + ps)/(pc - ps)) + 2*dqsdsig*(pc - ps))*exp(-k*(2*p - pc + ps)/(pc - ps))/(pc - ps);

      //the potential gradient should be the same, only replace m by n
      dgsds[i] = (-2*dpsdsig*k*pow(q, 2) + dpsdsig*pow(n, 2)*(pc - ps)*(2*p - pc + ps)*exp(k*(2*p - pc + ps)/(pc - ps)) + 2*dqsdsig*(pc - ps))*exp(-k*(2*p - pc + ps)/(pc - ps))/(pc - ps);
      
      //OLD:
      ////dfsds[i] = (2*p + pc - ps)*id[i]/3. + 3./m2*dev ;
      ////dgsds[i] = dfsds[i] ;
    }
  }
  
  /* The hardening modulus */
  /* H is defined by: df = (df/dsig_ij) dsig_ij - dl H 
  for further caluclations see jupyter
   */
  {
    ////double v = 1./(lambda - kappa) ;
    double dfsdpc = -pow(m, 2)*(p + ps) + pow(q, 2)*(-k/(-pc + ps) + k*(2*p - pc + ps)/pow(-pc + ps, 2))*exp(k*(2*p - pc + ps)/(-pc + ps));
    double dpcsdeps = pc0 * beta_eps * exp(beta_eps*eps_v_pl); //! check this !!!
    double dgsdp = (-2*k*pow(q, 2) + pow(n, 2)*(pc - ps)*(2*p - pc + ps)*exp(k*(2*p - pc + ps)/(pc - ps)))*exp(-k*(2*p - pc + ps)/(pc - ps))/(pc - ps);
    
    double pc_t = pc ;
    double p_star = (1.0/2.0)*((k - 1)*(pc_t - ps) - sqrt(pow(k, 2)*pow(pc_t, 2) + 2*pow(k, 2)*pc_t*ps + pow(k, 2)*pow(ps, 2) + pow(pc_t, 2) - 2*pc_t*ps + pow(ps, 2)))/k;
    //double softening_off = (p_t-p_star)*softening_off_switch; 

    *hm = - dfsdpc * dpcsdeps * dgsdp ; // this is negative
    //*hm = MIN(- dfsdpc * dpcsdeps * dgsdp, 0.) ; //if hm is zero, no stress increments are generated, we stay at one point

    if(p-p_star>0) {    // condition for plastic dilation/softening    //! remove this loop to activate softening
      *hm = 0. ; // constant yield surface
    }

    //*hm = beta_eps*pc0*(-2*k*pow(q, 2) + pow(n, 2)*(pc - ps)*(2*p - pc + ps)*exp(k*(2*p - pc + ps)/(pc - ps)))*(-2*k*p*pow(q, 2) + pow(m, 2)*(p + ps)*pow(pc - ps, 2)*exp(k*(2*p - pc + ps)/(pc - ps)))*exp((beta_eps*eps_v_pl*(pc - ps) - 2*k*(2*p - pc + ps))/(pc - ps))/pow(pc - ps, 3);
  }
  return(crit) ;
}







/*******
Function that reads the elements of a matrix row-wise
Parameters: rows(mi),columns(ni),matrix[mi][ni] 
*******/
void readMatrix(int mi, double matrix[mi][mi], double input_matrix[mi*mi]){
    int i,j;
    for(i=0;i<mi;i++){
        for(j=0;j<mi;j++){
            matrix[i][j] = input_matrix[i*mi+j];
        }
    } 
}
/*******
Function that prints the elements of a matrix row-wise
Parameters: rows(mi),columns(ni),matrix[mi][ni] 
*******/


  
// Function to Print inverse matrix
void PrintInverse(double ar[][20], int ni, int mi)
{
    for (int i = 0; i < ni; i++) {
        for (int j = ni; j < mi; j++) {
            printf("%.3f  ", ar[i][j]);
        }
        printf("\n");
    }
    return;
}
  
// Function to perform the inverse operation on the matrix.
void InverseOfMatrix(double matrix[][20], int order)
{
    // Matrix Declaration.
  
    float temp;
  
    // PrintMatrix function to print the element
    // of the matrix.
    printf("=== Matrix ===\n");
    //PrintMatrix(matrix, order, order);
  
    // Create the augmented matrix
    // Add the identity matrix
    // of order at the end of orignal matrix.
    for (int i = 0; i < order; i++) {
  
        for (int j = 0; j < 2 * order; j++) {
  
            // Add '1' at the diagonal places of
            // the matrix to create a identity matirx
            if (j == (i + order))
                matrix[i][j] = 1;
        }
    }
// Interchange the row of matrix,
    // interchanging of row will start from the last row
    for (int i = order - 1; i > 0; i--) {
  
        if (matrix[i - 1][0] < matrix[i][0])
            for (int j = 0; j < 2 * order; j++) {
  
                // Swapping of the row, if above
                // condition satisfied.
                temp = matrix[i][j];
                matrix[i][j] = matrix[i - 1][j];
                matrix[i - 1][j] = temp;
            }
    }
  
    // Print matrix after interchange operations.
    //printf("\n=== Augmented Matrix ===\n");
    //PrintMatrix(matrix, order, order * 2);
  
    // Replace a row by sum of itself and a
    // constant multiple of another row of the matrix
    for (int i = 0; i < order; i++) {
  
        for (int j = 0; j < 2 * order; j++) {
  
            if (j != i) {
  
                temp = matrix[j][i] / matrix[i][i];
                for (int ki = 0; ki < 2 * order; ki++) {
  
                    matrix[j][ki] -= matrix[i][ki] * temp;
                }
            }
        }
    }
  
    // Multiply each row by a nonzero integer.
    // Divide row element by the diagonal element
    for (int i = 0; i < order; i++) {
  
        temp = matrix[i][i];
        for (int j = 0; j < 2 * order; j++) {
  
            matrix[i][j] = matrix[i][j] / temp;
        }
    }
  
    // print the resultant Inverse matrix.
    printf("\n=== Inverse Matrix ===\n");
    PrintInverse(matrix, order, 2 * order);
  
    return;
}






double Plasticity_RMACCBraun(Plasticity_t* plasty,double* sig,double* eps_p,double* hardv)
/** Assymmetric Cam-Clay return mapping. Inputs are: 
 *  the elastic properties K and G
 *  the plasticity parmeters,
 *  the trial preconsolidation pressure pc=hardv[0]  
 *  -> this is new ! for thermal hardening, temperature effects are evaluated outside of _plasticity
 *  On outputs, the following values are modified:
 *  the stresses (sig), 
 *  the plastic strains (eps_p), 
 *  the pre-consolidation pressure (pc=hardv[0]).
 * 
 * Braun: For the beginning we only implement the mapping with isotropic elasticity.
 * Note that this does not affect the previous function (ComputeFunctionGradients), which remains general.
 * */
{
  Elasticity_t* elasty = Plasticity_GetElasticity(plasty) ;
  double young   = Elasticity_GetYoungModulus(elasty) ;
  double poisson = Elasticity_GetPoissonRatio(elasty) ;
  double K     = young / 3 / (1 - 2 * poisson) ;       //ADDED drained bulk modulus
  double G     = young / (2*(1+poisson)) ;             //ADDED shear modulus                   
  double* cijkl     = Elasticity_GetStiffnessTensor(elasty) ;
  double* C_inv  = Elasticity_GetInverseStiffnessTensor(elasty) ;//ADDED, see elasticity: writes inverse stiffness matrix in analytical form on C_inv
  //Elasticity_ComputeInverseStiffnessTensor(elasty,C_inv) ; 
  

  double m       = Plasticity_GetACC_M(plasty)  ;
  double n       = Plasticity_GetACC_N(plasty)  ;
  double k       = Plasticity_GetACC_k(plasty)   ;
  //double* dfsds  = Plasticity_GetYieldFunctionGradient(plasty) ; //!
  //double* dgsds  = Plasticity_GetPotentialFunctionGradient(plasty) ; //!
  double* hm     = Plasticity_GetHardeningModulus(plasty) ;
  double pc      = hardv[0] ;
  double pc0     = Plasticity_GetACCInitialPreconsolidationPressure(plasty) ;  
  double ps      = Plasticity_GetInitialIsotropicTensileLimit(plasty) ;
  double beta_eps= Plasticity_GetVolumetricStrainHardeningParameter(plasty) ;
  double beta_T0 =  Plasticity_GetThermalHardeningParameter(plasty) ;
  

  double id[9] = {1,0,0,0,1,0,0,0,1} ;
  double p,q,crit ;

  // initialize plastic strain changes
  double depsp[9] = { 0 } ;
  double depspv = 0.  ;

  //calculate the vol plastic strain from the previous step
  double eps_p_v_n = 0. ;
  int i,j,l; 
  for(i = 0 ; i < 9 ; i++) eps_p_v_n += eps_p[i] * id[i] ;  
  
  /* 
     The yield criterion
  */
  p    = (sig[0] + sig[4] + sig[8])/3. ;  
  q    = sqrt(3*Math_ComputeSecondDeviatoricStressInvariant(sig)) ;
  
  //this part has been moved out of _plasticity to the model:
  //double pc_t = pc0*(beta_T0*theta + 1)*exp(beta_eps*eps_p_v_n);   //the check for free thermal hardening is done outside of _plast
  double pc_t = hardv[0] ;

  pc = pc_t ; 
  //check if the trial state (p,q) is within the elastic domain (crit<=0) or if we plastify (crit>0)
  //the criterion is evaluated using pc from the previous step
  //crit = q*q*exp(k*(2*p+ps-pc)/(ps-pc))+m*m*(p+ps)*(p-pc);  //Insert here the yield function 
  crit = f_fun(plasty,sig,pc) ;
  
  //Message_Direct("probe = %4.2e\n",p/1.e6) ;

  

  //! until here just copy from above
  double sig_old[9] = {0} ;
  double sig_new[9] = {0} ;
  int dich_counter = 0 ;



  //! deactivate softening by setting = 1. to activate, set = 0
  int softening_off_switch = 1 ; 




  double softening_off1 = 0 ;
  double softening_off2 = 0;
  

  double sig_t[9] = { 0 };
  double p_t,q_t ;
  double dl ;



  /*
    Newton-raphson return mapping
   */
  // initialize trial values: remain constant during return mapping
  dl    = 0. ;
  for(i = 0 ; i < 9 ; i++) sig_t[i] = sig[i] ;
  p_t   = p ;
  q_t   = q ;
  
  if(crit > 0.) {
    //double pc_n  = pc ;

    //printf("start return mapping \n") ;
    //printf("crit= %4.3e",crit) ;

    double fcrit = crit ;
    int nf    = 0 ;
    double tol = 1.e6; //! set tolerance
    //dimension of Newton raphson system: 7
    double R_rm[7] = { 0 } ;   
    double x_rm[7] = { 0 } ;  
    int i;

    
    //give initial values: trial stresses and dlam = zero 


    double J_rm[100] = { 0 } ;
    double J_red[49] = { 0 } ;
    double dx_rm[7] = { 0 } ; 

    while(fcrit > tol) {    
    //while(fcrit > tol*pc_t*pc_t) {     //tolareance criterion taken from CamClayOffset model with tol = 1e-8
      //TODO check what derivatives / calculation can be done before the loop
      
      //Elasticity_PrintStiffnessTensor(elasty);
      //Elasticity_PrintInverseStiffnessTensor(elasty);
      for(i = 0 ; i < 3 ; i++) x_rm[i] = sig[i] ;
      x_rm[3] = sig[4] ;
      x_rm[4] = sig[5] ;
      x_rm[5] = sig[8] ;
      x_rm[6] = dl ; 


      /*
      here we need the residual vector Res, with 
      R[0] = f = 0
      R[1] = -p_t + p + K*dlam*dgsdp = 0
      R[2] = -q_t + q + 3*G*dlam*dgsdq = 0

      the unknown vector x contains
      x[0] = p
      x[1] = q
      x[2] = dlam

      Then we require the Jacobian J of R with respect to x

      the increment of unknowns dx is calculated by
      dx = J^-1 * R

      In the following, the Jacobian is gven in an analytical form, evaluated numerically and then inverted

      the subscript _rm is added to variable for identification
      */

      //initialize variables
      #define T2_3(a,i,j)      ((a)[(i) * 3 + (j)])
      #define T2_9(a,i,j)      ((a)[(i) * 9 + (j)])  //C(i,j)  (cijkl[(i)*9+(j)])
      #define T2_10(a,i,j)      ((a)[(i) * 10 + (j)])
      #define T2_7(a,i,j)      ((a)[(i) * 7 + (j)])
      #define T2_4(a,i,j)      ((a)[(i) * 4 + (j)])
      #define T4(a,i,j,k,l)  ((a)[(((i) * 3+(j)) * 3 + (k)) * 3 + (l)])


      //assign x
      ////p = x_rm[0] ; //TODO 1x10 
      ////q = x_rm[1] ;
      //for(i = 0 ; i < 9 ; i++) sig[i] = x_rm[i] ;  //! no need to update here, since it is initialized before and updated at the end of the loop??!
      //dl = x_rm[6] ; //! idem

      p    = (sig[0] + sig[4] + sig[8])/3. ;  
      q    = sqrt(3*Math_ComputeSecondDeviatoricStressInvariant(sig)) ;

      // calculate depsp[i] = C^-1_ijkl * (sig_t_kl - sig_kl)  corresponds after reorganization to C^-1_ij * (sig_t_j - sig_j)
      // calculate also volumeric part depspv
      {
        depspv = 0. ;
        int i,j ;
        for (i = 0 ; i < 9; i++) {
          depsp[i] = 0. ;
          
          for (j = 0 ; j < 9; j++) {
            depsp[i] += T2_9(C_inv,i,j) * (sig_t[j] - sig[j]);
          }
          //depspv +=  depsp[i] * id[i] ;
          //depsp[i] = dl * dgsds[i] ; //!--------------------------------!
        } 
      }
      depspv =  depsp[0] + depsp[4] + depsp[8] ;
      //depspv = dl * (dgsds[0]+dgsds[4]+dgsds[8]) ;

      // calculate new pc
      ////pc = pc_t*exp(beta_eps*(-p + p_t)/K); 
      pc = pc_t*exp(beta_eps*depspv);   
      
      int stop_crit = 0 ;
      


      


      //**********************************************************************************


      
      
      //scale the trial stress to evaluate scaled criterion

      //grab parameters f_par and f_perp
      double f_par       = Plasticity_GetACC_f_par(plasty)   ;
      double f_perp      = Plasticity_GetACC_f_perp(plasty)   ;
      int axis_3  = Elasticity_GetAxis3(Plasticity_GetElasticity(plasty)) ; 

      //scale sig_t
      double sig_t_tilde[9] = {0};
      for (i = 0 ; i < 9; i++) sig_t_tilde[i] = sig_t[i] ; 
      sig_t_tilde[0] = sig_t[0]*f_par;
      sig_t_tilde[4] = sig_t[4]*f_par;
      sig_t_tilde[8] = sig_t[8]*f_par;
      sig_t_tilde[axis_3*3+axis_3] = sig_t[axis_3*3+axis_3]*f_perp;

      //scale sig
      double sig_tilde[9] = {0};
      for (i = 0 ; i < 9; i++) sig_tilde[i] = sig[i] ; 
      sig_tilde[0] = sig[0]*f_par;
      sig_tilde[4] = sig[4]*f_par;
      sig_tilde[8] = sig[8]*f_par;
      sig_tilde[axis_3*3+axis_3] = sig[axis_3*3+axis_3]*f_perp;

      //evaluate p for scaled sig
      double p_t_tilde    = (sig_t_tilde[0] + sig_t_tilde[4] + sig_t_tilde[8])/3. ;  
      double p_tilde    = (sig_tilde[0] + sig_tilde[4] + sig_tilde[8])/3. ;  

      //softening threshold: p_star divides in softening and hardening side of the yield surface
      double p_star = (1.0/2.0)*((k - 1)*(pc_t - ps) - sqrt(pow(k, 2)*pow(pc_t, 2) + 2*pow(k, 2)*pc_t*ps + pow(k, 2)*pow(ps, 2) + pow(pc_t, 2) - 2*pc_t*ps + pow(ps, 2)))/k;
      if (k==0) p_star = (pc_t-ps)/2 ;

      // printf("p= %.*e ",DBL_DIG, p) ;
      // printf("\n") ;
      // printf("p_tilde= %.*e ",DBL_DIG, p_t_tilde) ;
      // printf("\n") ;
      // printf("q= %.*e ",DBL_DIG, q) ;
      // printf("\n") ;
      // printf("q_tilde= %.*e ",DBL_DIG, q_t_tilde) ;
      // printf("\n") ;
      // printf("p_star= %.*e ",DBL_DIG, p_star) ;
      // printf("\n") ;
      
      softening_off1 = (p_t_tilde-p_star)*softening_off_switch; //! positive if trial stress p_t is on the softening side
      //double softening_off2 = (p_tilde-p_star)*softening_off_switch; // positive if final stress p is on the softening side
      //double softening_off3 = 0. ;
      //if(pc > pc_t) softening_off3 = 1.0; 

      // if( (softening_off_switch*softening_off3) >0 ) {    //! new forced condition to avoid yield surface shrinking, see also below
      //    pc = pc_t ; // no yield surface shrinking
      // }


      if(softening_off1>0  ||  softening_off2>0) {    // condition for plastic dilation/softening and forced softening_off
      //if(softening_off1>0  ) {    // condition for plastic dilation/softening and forced softening_off
         pc = pc_t ; // no yield surface shrinking
      }


      //**********************************************************************************
    

      double dx = 1.0e2;    //!stress increment
      double dpc = 1.0e2;     //!pc increment
      
      //we have to calculate 
      // f1, f2, f3, f22, f33

      double f2[9] = { 0 };
      double f3[9] = { 0 };
      double g2[9] = { 0 };
      double g3[9] = { 0 };
      double g22[81] = { 0 };
      double g33[81] = { 0 };
      // long double g23[81] = { 0 };
      // long double g32[81] = { 0 };
      double g_sig2_pc2[9] = { 0 };
      double g_sig3_pc3[9] = { 0 };
      double g_sig_pc2 = 0 ;
      double g_sig_pc3 = 0 ;

      double f_pc2 = 0 ;
      double f_pc3 = 0 ;
    

      double pc2[9] = { 0 };
      double pc3[9] = { 0 };

      double sig_dx[9] = { 0 };
      double pc_dpc ;
      int in ;

      // notation: 
      // f1 = f(x) 
      // f2 = f(x+dx) 
      // f3 = f(x-dx) 
      // f12 = f(x,y+dy) 
      // f23 = f(x+dx,y-dy) 


      double f1 = 0 ;
      double g1 = 0 ;

      f1 = f_fun(plasty,sig,pc) ; 
      g1 = g_fun(plasty,sig,pc) ; 


      if (10>50){
        {
          int i ;
          int j ; 
          printf("\n") ;
          printf("sig_before:\n") ;
          for (j = 0 ; j < 9 ; j++) {
            printf(" %.*e",DBL_DIG, sig[j]) ;
                        
          }
          printf("\n") ;
        }
      
        printf("crit_before(g1)= %.*e ",DBL_DIG, g1) ;
        printf("crit_before(crit)= %.*e ",DBL_DIG, crit) ;
        printf("p_before= %.*e ",DBL_DIG, p) ;
        printf("q_before= %.*e ",DBL_DIG, q) ;
        printf("\n") ;
      }


      //printf("crit_before(crit)= %.*e \n",DBL_DIG, crit) ;


      //calculate forward and backward function values:
      {
        int in ;
        for (in = 0 ; in < 9; in++) sig_dx[in] = sig[in] ; //reset sig_dx=sig
        pc_dpc = pc ;
        pc_dpc += dpc ;
        g_sig_pc2 = g_fun(plasty,sig_dx,pc_dpc) -g1;
        f_pc2 = f_fun(plasty,sig_dx,pc_dpc) -f1;
      }

      {
        int in ;
        for (in = 0 ; in < 9; in++) sig_dx[in] = sig[in] ; //reset sig_dx=sig
        pc_dpc = pc ;
        pc_dpc -= dpc ;
        g_sig_pc3 = g_fun(plasty,sig_dx,pc_dpc) -g1;
        f_pc3 = f_fun(plasty,sig_dx,pc_dpc) -f1;
      }

      {
        int i,j,in ;  
        // i and j are used to iterate over sig[i] for first order derivative and over both sig[i] and sig[j] for the second order one     
        for (i = 0 ; i < 9; i++) {
              
          for (in = 0 ; in < 9; in++) sig_dx[in] = sig[in] ; //reset sig_dx=sig
          sig_dx[i] += dx ; 
          if(i==1) sig_dx[3] = sig_dx[i] ;                                    //symmetries. the inverse symmetries are not required, since this part of the matrix is not evaluated
          if(i==2) sig_dx[6] = sig_dx[i] ;  
          if(i==5) sig_dx[7] = sig_dx[i] ;  
          f2[i] = f_fun(plasty,sig_dx,pc) -f1;
          g2[i] = g_fun(plasty,sig_dx,pc) -g1;
          pc2[i] = pc_fun(plasty,sig_dx,sig_t,C_inv,pc_t) ;//!

          for (in = 0 ; in < 9; in++) sig_dx[in] = sig[in] ; //reset sig_dx=sig
          sig_dx[i] -= dx ;
          if(i==1) sig_dx[3] = sig_dx[i] ;                                    //symmetries. the inverse symmetries are not required, since this part of the matrix is not evaluated
          if(i==2) sig_dx[6] = sig_dx[i] ;  
          if(i==5) sig_dx[7] = sig_dx[i] ;  
          f3[i] = f_fun(plasty,sig_dx,pc) -f1;
          g3[i] = g_fun(plasty,sig_dx,pc) -g1;
          pc3[i] = pc_fun(plasty,sig_dx,sig_t,C_inv,pc_t) ;//!

          // second order deriv wrt to sig[i] and pc: f_sig2_pc2, f_sig3_pc3
          for (in = 0 ; in < 9; in++) sig_dx[in] = sig[in] ; //reset sig_dx=sig
          sig_dx[i] += dx ;
          if(i==1) sig_dx[3] = sig_dx[i] ;                                    //symmetries. the inverse symmetries are not required, since this part of the matrix is not evaluated
          if(i==2) sig_dx[6] = sig_dx[i] ;  
          if(i==5) sig_dx[7] = sig_dx[i] ;  
          pc_dpc = pc ;
          pc_dpc += dpc ;
          g_sig2_pc2[i] = g_fun(plasty,sig_dx,pc_dpc) -g1;//!

          for (in = 0 ; in < 9; in++) sig_dx[in] = sig[in] ; //reset sig_dx=sig
          sig_dx[i] -= dx ;
          if(i==1) sig_dx[3] = sig_dx[i] ;                                    //symmetries. the inverse symmetries are not required, since this part of the matrix is not evaluated
          if(i==2) sig_dx[6] = sig_dx[i] ;  
          if(i==5) sig_dx[7] = sig_dx[i] ;  
          pc_dpc = pc ;
          pc_dpc -= dpc ;
          g_sig3_pc3[i] = g_fun(plasty,sig_dx,pc_dpc) -g1;//!


          // loop over j for second order derviv
          for (j = 0 ; j < 9; j++) {

            for (in = 0 ; in < 9; in++) sig_dx[in] = sig[in] ; //reset sig_dx=sig
            sig_dx[i] += dx ;
            sig_dx[j] += dx ;
            if(i==1) sig_dx[3] = sig_dx[i] ;                                    //symmetries. the inverse symmetries are not required, since this part of the matrix is not evaluated
            if(i==2) sig_dx[6] = sig_dx[i] ;  
            if(i==5) sig_dx[7] = sig_dx[i] ;  
            if(j==1) sig_dx[3] = sig_dx[j] ;                                    //symmetries. the inverse symmetries are not required, since this part of the matrix is not evaluated
            if(j==2) sig_dx[6] = sig_dx[j] ;  
            if(j==5) sig_dx[7] = sig_dx[j] ;  
            T2_9(g22,i,j) = g_fun(plasty,sig_dx,pc) -g1 ;
            
            // for (in = 0 ; in < 9; in++) sig_dx[in] = sig[in] ; //reset sig_dx=sig
            // sig_dx[i] += dx ;
            // sig_dx[j] -= dx ;
            // T2_9(g23,i,j) = g_fun(plasty,sig_dx,pc) ;
            
            // for (in = 0 ; in < 9; in++) sig_dx[in] = sig[in] ; //reset sig_dx=sig
            // sig_dx[i] -= dx ;
            // sig_dx[j] += dx ;
            // T2_9(g32,i,j) = g_fun(plasty,sig_dx,pc) ;

            for (in = 0 ; in < 9; in++) sig_dx[in] = sig[in] ; //reset sig_dx=sig
            sig_dx[i] -= dx ;
            sig_dx[j] -= dx ;
            if(i==1) sig_dx[3] = sig_dx[i] ;                                    //symmetries. the inverse symmetries are not required, since this part of the matrix is not evaluated
            if(i==2) sig_dx[6] = sig_dx[i] ;  
            if(i==5) sig_dx[7] = sig_dx[i] ;  
            if(j==1) sig_dx[3] = sig_dx[j] ;                                    //symmetries. the inverse symmetries are not required, since this part of the matrix is not evaluated
            if(j==2) sig_dx[6] = sig_dx[j] ;  
            if(j==5) sig_dx[7] = sig_dx[j] ;  
            T2_9(g33,i,j) = g_fun(plasty,sig_dx,pc) -g1;
          }
        }
      }
      //calculate derivatives
      // we need:
      // df / dpc
      double dfsdpc ; 
      // df / ds
      double dfsds[9] = { 0 } ; 
      // dg / ds
      double dgsds[9] = { 0 } ; 
      // dg²/ ds ds
      double ddgsdsds[81] = { 0 } ; 
      // dg²/ ds dpc
      double ddgsdsdpc[9] = { 0 } ; 
      // dpc/ ds
      double dpcsds[9] = { 0 } ;


      double id[9] = {1,0,0,0,1,0,0,0,1} ;
      double dpsdsig[9] = {0} ;
      double dqsdsig_q[9] = {0} ;
      double ddqsdsigdsig_q[9] = {0} ;
      {
        int i ; 
        for (i = 0 ; i < 9; i++) {
        dpsdsig[i] = id[i]/3. ;
        dqsdsig_q[i] = 3./2. * (sig[i] - p*id[i]); // /q ;
        ddqsdsigdsig_q[i] = 0. ;//3./2. * ( 1 - 1./3. * id[i] - 3./2. * pow((sig[i] - p*id[i]),2)  ) ;
        ddqsdsigdsig_q[i] = 1.5*  (dqsdsig_q[i]*(id[i]*p - sig[i])/q/q + (-dpsdsig[i]*id[i] + 1))   ;
        }
      }

      {
        int i,j ;  
        for (i = 0 ; i < 9; i++) {

          // calculate first order derivatives 
          dfsds[i]  = (f2[i]-f3[i])/(2*dx) ;
          dgsds[i]  = (g2[i]-g3[i])/(2*dx) ;
          dpcsds[i] = (pc2[i]-pc3[i])/(2*dx) ; //!  new
          
          // calculate second order derivatives 
          ddgsdsdpc[i] = ( g_sig2_pc2[i] - g2[i] - g_sig_pc2 - g3[i] - g_sig_pc3 + g_sig3_pc3[i] ) / (2*dx*dpc)  ;  //!  new
          //ddgsdsdpc[1] = 0.; //!---------------------------------------------------------------------------------------------------------------------------------------------------------
          //ddgsdsdpc[2] = 0.; 
          //ddgsdsdpc[5] = 0.; 
          
          if(softening_off1>0  ||  softening_off2>0)  {    // condition for plastic dilation/softening and forced softening_off //! added
          //if(softening_off1>0)  {    // condition for plastic dilation/softening and forced softening_off //! added
            dpcsds[i] = 0. ; 
            //ddgsdsdpc[i] = 0. ; // this is already later multiplied by dpcsds[i] = 0. if softening off
          }

          // if(softening_off_switch*softening_off3 >0)  {    // condition for plastic dilation/softening and forced softening_off //! added
          //   dpcsds[i] = 0. ; 
          // }


          for (j = 0 ; j < 9; j++) {
            T2_9(ddgsdsds,i,j) = ( T2_9(g22,i,j) - g2[i] - g2[j] - g3[i] - g3[j] + T2_9(g33,i,j) ) / (2*dx*dx) ; //!
            //T2_9(ddgsdsds,i,j) = ( T2_9(g22,i,j) - T2_9(g32,i,j)  - T2_9(g23,i,j) + T2_9(g33,i,j)  )     / (4*dx*dx) ; 

            // double kronecker = 0.;
            // if(i==j) kronecker = 1.;
            
            // T2_9(ddgsdsds,i,j) = -2*dpsdsig[i]*k*kronecker*(-2*dpsdsig[i]*k*pow(q, 2) + dpsdsig[i]*pow(n, 2)*
            // (pc - ps)*(2*p - pc + ps)*exp(k*(2*p - pc + ps)/(pc - ps)) + 2*dqsdsig_q[i]*(pc - ps))*exp(-k*(2*p - pc + ps)/
            // (pc - ps))/pow(pc - ps, 2) + (ddqsdsigdsig_q[i]*kronecker*(2*pc - 2*ps) + 2*pow(dpsdsig[i], 2)*k*kronecker*pow(n, 2)*
            // (2*p - pc + ps)*exp(k*(2*p - pc + ps)/(pc - ps)) + 2*pow(dpsdsig[i], 2)*kronecker*pow(n, 2)*(pc - ps)*exp(k*(2*p - pc + ps)/
            // (pc - ps)) - 4*dpsdsig[i]*dqsdsig_q[i]*k*kronecker + pow((dqsdsig_q[i]/q), 2)*kronecker*(2*pc - 2*ps))*exp(-k*(2*p - pc + ps)/(pc - ps))/(pc - ps);



          }
        }
      }
      // this one we can obtain analytically:
      dfsdpc = -m*m*(p + ps) + q*q*(-k/(-pc + ps) + k*(2*p - pc + ps)/(-pc + ps)/(-pc + ps))*exp(k*(2*p - pc + ps)/(-pc + ps)) ; // this is automatically multiplied by zero if softening off
      dfsdpc = (f_pc2-f_pc3)/(2*dpc) ;
      
            
      // {
      //   int i ;
      //   int j ; 
      //   printf("\n") ;
      //   printf("sig_t:\n") ;
      //   for (j = 0 ; j < 9 ; j++) {
      //       printf(" % e",sig_t[j]) ;
      //   }
      //   printf("\n") ;
      //   printf("g_forward:\n") ;
      //   for (j = 0 ; j < 9 ; j++) {
      //       printf(" %.10Le",g2[j]) ;
      //   }
      //   printf("\n") ;
      //   printf("g_back:\n") ;
      //   for (j = 0 ; j < 9 ; j++) {
      //       printf(" %.10Le",g3[j]) ;
      //   }
      //   printf("\n") ;
      //   printf("g_2forw:\n") ;
      //   for(i = 0 ; i < 9 ; i++) {          
      //     for (j = 0 ; j < 9 ; j++) {
      //       printf(" %.10Le",g22[i*9 + j]) ;
      //     }
      //     printf("\n") ;
      //   }
      //   printf("\n") ;
      //   printf("g_2back:\n") ;
      //   for(i = 0 ; i < 9 ; i++) {          
      //     for (j = 0 ; j < 9 ; j++) {
      //       printf(" %.10Le",g33[i*9 + j]) ;
      //     }
      //     printf("\n") ;
      //   }
      //   printf("\n") ;
      //   printf("ddgsdsds:\n") ;
      //   for(i = 0 ; i < 9 ; i++) {          
      //     for (j = 0 ; j < 9 ; j++) {
      //       printf(" %.10Le",ddgsdsds[i*9 + j]) ;
      //     }
      //     printf("\n") ;
      //   }
      // }



      // {
      //   int    i ;
        
      //   for(i = 0 ; i < 9 ; i++) {
      //     double dev = sig[i] - p*id[i] ;
      //     double dpsdsig = id[i]/3. ;
      //     //double dqsdsig = 3/2*dev/q ; //taken from existing camclay
      //     double dqsdsig = 3/2*dev ; //multiply already with q and remove q in the later equation

      //     //paste function calculated in jupyter
      //     dfsds[i] = (-2*dpsdsig*k*pow(q, 2) + dpsdsig*pow(m, 2)*(pc - ps)*(2*p - pc + ps)*exp(k*(2*p - pc + ps)/(pc - ps)) + 2*dqsdsig*(pc - ps))*exp(-k*(2*p - pc + ps)/(pc - ps))/(pc - ps);

      //     //the potential gradient should be the same, only replace m by n
      //     dgsds[i] = (-2*dpsdsig*k*pow(q, 2) + dpsdsig*pow(n, 2)*(pc - ps)*(2*p - pc + ps)*exp(k*(2*p - pc + ps)/(pc - ps)) + 2*dqsdsig*(pc - ps))*exp(-k*(2*p - pc + ps)/(pc - ps))/(pc - ps);
          
      //     //OLD:
      //     ////dfsds[i] = (2*p + pc - ps)*id[i]/3. + 3./m2*dev ;
      //     ////dgsds[i] = dfsds[i] ;
      //   }
      // }


      //-------------------------calculate Residual 

      // for (i = 0 ; i < 9; i++) {
      //   R_rm[i] = dl * dgsds[i] - depsp[i];
      // } 
      // //erase the entries index 3,6,7
      // // R_rm[0] = dl * dgsds[0] - depsp[0];
      // // R_rm[1] = dl * dgsds[4] - depsp[4];
      // // R_rm[2] = dl * dgsds[8] - depsp[8];

      // R_rm[9] = f_fun(plasty,sig,pc);  //copy here the yield function from above


      for(i = 0 ; i < 3 ; i++) R_rm[i] = dl * dgsds[i] - depsp[i]; 
      R_rm[3] = dl * dgsds[4] - depsp[4]; 
      R_rm[4] = dl * dgsds[5] - depsp[5]; 
      R_rm[5] = dl * dgsds[8] - depsp[8]; 
      R_rm[6] = f_fun(plasty,sig,pc); 


      //----------------------------Jacobian:

      // | x1 0  | 
      // | x3 x4 | 


      // changed to
      // | x1 x2  | 
      // | x3 0 |
      

      {
        int i,j;
        //the x1 (dimensions 9x9) 
        for (i = 0 ; i < 9; i++) {
          for (j = 0 ; j < 9; j++) {
            T2_10(J_rm,i,j) = T2_9(C_inv,i,j) + dl * ( T2_9(ddgsdsds,i,j) + ddgsdsdpc[i] * dpcsds[j] )  ; 
          }
        }
      }

      { 
        int i;
        // x2 (dimensions 9x1)
        for (i = 0 ; i < 9; i++) {
          T2_10(J_rm,i,9) =  dgsds[i] ;
        }
      }

      // x3 (dimensions 1x9)

      { 
        int i;
        //loop over 9 i.s
        for (i = 0 ; i < 9; i++) {
          T2_10(J_rm,9,i) = dfsdpc * dpcsds[i] + dfsds[i] ;
        }
      }


      // REMOVE lines/rows from the 10x10 Jacobian J_rm
      // output reduced Jacobian J_red with dimensions 4x4
      //! multiply double entries
      
      int i_inc = 0 ;
      int j_inc = 0 ;
      { 
        int i,j; 
        for (i = 0 ; i < 7; i++) {
          if (i == 3) i_inc = 1 ;
          if (i == 5) i_inc = 3 ; 
          j_inc = 0 ;
          for (j = 0 ; j < 7; j++) {
            if (j == 3) j_inc = 1 ;
            if (j == 5) j_inc = 3 ; 
            T2_7(J_red,i,j) = T2_10(J_rm,i+i_inc,j+j_inc) ;
          }
        }
        T2_7(J_red,1,1) += T2_9(C_inv,1,3) ;
        T2_7(J_red,2,2) += T2_9(C_inv,2,6) ;
        T2_7(J_red,4,4) += T2_9(C_inv,5,7) ;
      }
      //printf("J_rm[0] = %e ",J_rm[0]) ;
 

      //--------------------------solve equation
      // we can solve an equation type A*x=b where A is J, b is -R and x is dx, hence J*dx=-R
      // this can be done using the function Math_SolveByGaussElimination wich takes J and R as arguments
      // the solution dx is written on b, therefore we have to declare first dx = R
      // assign R to dx, to be overwritten after the the solution dx is found by the following function
      int i ;
      
      double J_red_temp[100] = {0} ;
      for(i = 0 ; i < 100 ; i++) J_red_temp[i] = J_rm[i] ;  

      //printf("g2[0] = %e",g2[0] ) ; //!
      //
      
      //Declare a matrix to store the user given matrix
      //double a[7][7];
      //Declare another matrix to store the resultant matrix obtained after Gauss Elimination
      //double U[7][7];
      //readMatrix(7,U,J_red);
      //Perform Gauss Elimination 
      //gaussElimination(7,7,U);
      // printf("\nThe Upper Triangular matrix after Gauss Eliminiation is:\n\n");
      // printMatrix(7,7,U);
      //gaussElimPhil(7,J_red);
      //InverseOfMatrix(U, 7); 

      double gje_sol[7] = {0} ;
      double x_before[7] = {0} ;
      double ge_sol[7] = {0} ;
      double gsl_sol[7] = {0} ;
      double J_ge_sol[49] = {0} ;
      double J_gje_sol[49] = {0} ;
      for(i = 0 ; i < 100 ; i++) J_red_temp[i] = J_rm[i] ;

      for(i = 0 ; i < 49 ; i++) J_ge_sol[i] = J_red[i] ;  
      for(i = 0 ; i < 49; i++) J_gje_sol[i] = J_red[i] ;    

      for(i = 0 ; i < 7 ; i++) gje_sol[i] = - R_rm[i] ;  
      for(i = 0 ; i < 7 ; i++) ge_sol[i] = - R_rm[i] ;  
      for(i = 0 ; i < 7 ; i++) gsl_sol[i] = - R_rm[i] ;  

      for(i = 0 ; i < 7 ; i++) x_before[i] = - R_rm[i] ; 
                
      // printf("\n") ;
      // printf("Reduced Jacobian:\n") ;
      // for(i = 0 ; i < 7 ; i++) {          
      //   for (j = 0 ; j < 7 ; j++) {
      //     printf(" % e",J_red[i*7 + j]) ;
      //   }
      //   printf("\n") ;
      // }
      //-------------------------------------------------------------------GSL
      gsl_matrix_view J_red_gsl = gsl_matrix_view_array(J_red, 7, 7);
      gsl_vector_view b_gsl = gsl_vector_view_array(gsl_sol, 7);
      gsl_vector *x_gsl = gsl_vector_alloc(7);
      gsl_permutation *p_gsl = gsl_permutation_alloc(7);
      int s;
      gsl_linalg_LU_decomp(&J_red_gsl.matrix, p_gsl, &s);
      gsl_linalg_LU_solve(&J_red_gsl.matrix, p_gsl, &b_gsl.vector, x_gsl);

      //printf("x = \n");
      //gsl_vector_fprintf(stdout, x_gsl, "%g");

      gsl_permutation_free(p_gsl);

      //-------------------------------------------------------------------GSL

      //Math_SolveByGaussJordanElimination(J_gje_sol, gje_sol, 7, 1);
      //Math_SolveByGaussElimination(J_ge_sol, ge_sol, 7); //! use this 
      //Math_SolveByGaussJordanElimination(J_red,dx_rm,4,1) ;
      //Math_SolveByGaussJordanElimination(J_red,dx_rm,7,1) ;

      //for(i = 0 ; i < 7 ; i++) dx_rm[i] = gje_sol[i] ;  // choose gauss jordan elimination
      for(i = 0 ; i < 7 ; i++) dx_rm[i] = gsl_vector_get(x_gsl,i); // choose GSL solution with LU decomposition //!

      double x_after_ge[7] = {0};
      for (i = 0 ; i < 7; i++) {
        for (j = 0 ; j < 7; j++) {
          x_after_ge[i] += T2_7(J_red,i,j) * ge_sol[j];
        }
      } 

      double x_after_gje[7] = {0};
      for (i = 0 ; i < 7; i++) {
        for (j = 0 ; j < 7; j++) {
          x_after_gje[i] += T2_7(J_red,i,j) * gje_sol[j];
        }
      } 

      double x_after_gsl[7] = {0};
      for (i = 0 ; i < 7; i++) {
        for (j = 0 ; j < 7; j++) {
          x_after_gsl[i] += T2_7(J_red,i,j) * gsl_vector_get(x_gsl,j);
        }
      } 

      gsl_vector_free(x_gsl) ;

       // The input matrix is a[n*n]. b[n*m] is input containing the m right-hand 
       //  side vectors. On output, a is replaced by its matrix inverse, and b is 
       //  replaced by the corresponding set of solution vectors.


      //update x=dx+x

      // printf("x_before_increment = ") ;      
      // for (j = 0 ; j < 7 ; j++) {
      //   printf(" % e",x_rm[j]) ;
      // }
      // printf("\n") ;

      for(i = 0 ; i < 7 ; i++) x_rm[i] += dx_rm[i] ;  


      // printf("x_after_increment = ") ;      
      // for (j = 0 ; j < 7 ; j++) {
      //   printf(" % e",x_rm[j]) ;
      // }
      // printf("\n") ;
      
      //update stress state
      // repeat/copy assignment from above

      // for(i = 0 ; i < 9 ; i++) sig[i] = x_rm[i];
      // dl   = x_rm[9] ; 

      for(i = 0 ; i < 3 ; i++) sig[i] = x_rm[i];
      sig[3] += dx_rm[1] ; //!
      sig[4] = x_rm[3] ;
      sig[5] = x_rm[4] ;
      sig[6] += dx_rm[2] ; //!
      sig[7] += dx_rm[4] ; //!
      sig[8] = x_rm[5] ;
      dl   = x_rm[6] ; 

      // sig[0] = x_rm[0] ;
      // sig[4] = x_rm[1] ;
      // sig[8] = x_rm[2] ;
      // dl   = x_rm[3] ; 

      
      p    = (sig[0] + sig[4] + sig[8])/3. ;  
      q    = sqrt(3*Math_ComputeSecondDeviatoricStressInvariant(sig)) ;

      // calculate depsp[i]   COPY FROM ABOVE
      // depsp[i] = C^-1_ijkl * (sig_t_kl - sig_kl)  corresponds after reorganization to C^-1_ij * (sig_t_j - sig_j)
      // calculate also volumeric part depspv
      depspv = 0. ;
      for (i = 0 ; i < 9; i++) {
        depsp[i] = 0. ;
        
        for (j = 0 ; j < 9; j++) {
          depsp[i] += T2_9(C_inv,i,j) * (sig_t[j] - sig[j]);
          
        }
        //depspv +=  depsp[i] * id[i] ;
        //depsp[i] = dl * dgsds[i] ; //!--------------------------------!
      } 
      depspv =  depsp[0] + depsp[4] + depsp[8] ;
      

      // update pc   COPY FROM ABOVE
      ////pc = pc_t*exp(beta_eps*(-p + p_t)/K); 
      pc = pc_t*exp(beta_eps*depspv);     

      //if(pc > pc_t) softening_off3 = 1.0; //check for softening      


      // if((softening_off_switch*softening_off3) > 0) {    //! new forced condition to avoid yield surface shrinking, see also below
      //   pc = pc_t ; // no yield surface shrinking
      // }


      if(softening_off1>0  ||  softening_off2>0) {    // condition for plastic dilation/softening and forced softening_off
      //if(softening_off1>0 ) {    // condition for plastic dilation/softening and forced softening_off
        pc = pc_t ; // no yield surface shrinking
      }      

      //-----------------------STOP CRITERION
      //calculate R_rm as above (copy)
      //here we have the following dimensions:

      //we need to update dgsds   COPY FROM ABOVE

      // for (i = 0 ; i < 9; i++) {
      //   for (in = 0 ; in < 9; in++) sig_dx[in] = sig[in] ; //reset sig_dx=sig
      //   sig_dx[i] += dx ;                                   //add increment
      //   g2[i] = g_fun(plasty,sig_dx,pc) ;
        
      //   for (in = 0 ; in < 9; in++) sig_dx[in] = sig[in] ; //reset sig_dx=sig
      //   sig_dx[i] -= dx ;
      //   g3[i] = g_fun(plasty,sig_dx,pc) ;

      //   dgsds[i] = (g2[i]-g3[i])/(2*dx) ;
      // }

      //then R can be updated   COPY FROM ABOVE

      // for (i = 0 ; i < 9; i++) {
      //   R_rm[i] = dl * dgsds[i] - depsp[i];
      // } 
      // R_rm[9] = f_fun(plasty,sig,pc);  //copy here the yield function from above


      // R_rm[9] dimensionless
      // R_rm[0-8] stress
      // R_rm[0] = dl * dgsds[0] - depsp[0];
      // R_rm[1] = dl * dgsds[4] - depsp[4];
      // R_rm[2] = dl * dgsds[8] - depsp[8];

      // R_rm[3] = f_fun(plasty,sig,pc);  //copy here the yield function from above

      // for(i = 0 ; i < 3 ; i++) R_rm[i] = dl * dgsds[i] - depsp[i]; 
      // R_rm[3] = dl * dgsds[4] - depsp[4]; 
      // R_rm[4] = dl * dgsds[5] - depsp[5]; 
      // R_rm[5] = dl * dgsds[8] - depsp[8]; 
      //R_rm[6] = f_fun(plasty,sig,pc); 


      // // save crit for output in case the loop stops
      crit = f_fun(plasty,sig,pc); 

      //we calculate relative (dimensionless) residuals and use the maximum one as criterion     
      //hence we divide the stress residuals by the trial stress , to obtain relative dimensionless values
      // R_rm[0] /= sig_t[0];
      // R_rm[1] /= sig_t[1];
      // R_rm[2] /= sig_t[2];
      // R_rm[3] /= sig_t[4];
      // R_rm[4] /= sig_t[5];
      // R_rm[5] /= sig_t[8];
      // R_rm[6] = fabs(R_rm[6]) ;
      
      //relative errors, check maximum
      fcrit = fabs(crit) ;
      //fcrit =  fabs(crit) / 1.0e7 *  MIN(   MIN(fabs(sig[0]),fabs(sig[4])) ,  fabs(sig[8]))  ; //!store smallest sig value

      // if(fcrit <= tol) {
      //   printf("Number of iterations: %i ",nf) ;
      //   printf("f = %e ",R_rm[0]) ;
      //   printf("\n") ;
      // }

      if(10 > 100) {  //! always plot
        printf("\n") ;
        printf("I %i ",nf+1) ;
        printf("crit_after= %.*e",DBL_DIG, crit) ;
        printf("\n") ;
        printf("p_t= %4.3e MPa ",(p_t)/1.e6) ;
        printf("q_t= %4.3e MPa ",(q_t)/1.e6) ;
        printf("\n") ;
        printf("p= %4.3e MPa ",(p)/1.e6) ;
        printf("q= %4.3e MPa ",(q)/1.e6) ;
        printf("\n") ;
        printf("pc= %4.3e MPa ",pc/1.e6) ;
        printf("pc_t= %4.3e MPa ",pc_t/1.e6) ;
        printf(" %.*e",DBL_DIG, dl) ;
        //-----------------------------------Print singular matrix //! remove afterwards  
        printf("\n") ;
        
        {
          int i ;
          int j ; 

          printf("sig_t:\n") ;
          for (j = 0 ; j < 9 ; j++) {
              printf(" %.*e",DBL_DIG, sig_t[j]) ;
          }
          printf("\n") ;
          printf("sig:\n") ;
          for (j = 0 ; j < 9 ; j++) {
              printf(" %.*e",DBL_DIG, sig[j]) ;
          }
          // printf("\n") ;
          // printf("Jacobian:\n") ;
          // for(i = 0 ; i < 10 ; i++) {          
          //   for (j = 0 ; j < 10 ; j++) {
          //     printf(" % e",J_red_temp[i*10 + j]) ;
          //   }
          //   printf("\n") ;
          // }
          printf("\n") ;
          printf("Gauss-Jordan solution for dx:\n") ;
          for (j = 0 ; j < 7 ; j++) {
              printf(" %.*e",DBL_DIG, gje_sol[j]) ;
          }

          printf("\n") ;

          printf("Gsl solution:\n") ;
          for (j = 0 ; j < 7 ; j++) {
              printf(" %.*e",DBL_DIG, gsl_vector_get(x_gsl,j)) ;
          }
          printf("\n") ;

          // printf("Gauss solution:\n") ;
          // for (j = 0 ; j < 7 ; j++) {
          //     printf(" %.*e",DBL_DIG, ge_sol[j]) ;
          // }
          // printf("\n") ;

          printf("residual before iteration:\n") ;
          for (j = 0 ; j < 7 ; j++) {
              printf(" %.*e",DBL_DIG, x_before[j]) ;
          }
          printf("\n") ;



          printf("recalculate residual after gauss jordan elim for verification:\n") ;
          for (j = 0 ; j < 7 ; j++) {
              printf(" %.*e",DBL_DIG, x_after_gje[j]) ;
          }
          printf("\n") ;


          printf("recalculate residual after gsl for verification:\n") ;
          for (j = 0 ; j < 7 ; j++) {
              printf(" %.*e",DBL_DIG, x_after_gsl[j]) ;
          }
          printf("\n") ;

          printf("Reduced Jacobian:\n") ;
          for(i = 0 ; i < 7 ; i++) {          
            for (j = 0 ; j < 7 ; j++) {
              printf(" % e",J_red[i*7 + j]) ;
            }
            printf("\n") ;
          }
          
          printf("\n") ;
          printf("Inverse reduced Jacobian:\n") ;
          for(i = 0 ; i < 7 ; i++) {          
            for (j = 0 ; j < 7 ; j++) {
              printf(" % e",J_gje_sol[i*7 + j]) ;
            }
            printf("\n") ;
          }


          // printf("\n") ;
          // printf("Inverse reduced Jacobian with second funct:\n") ;
          // for(i = 0 ; i < 7 ; i++) {          
          //   for (j = 0 ; j < 7 ; j++) {
          //     printf(" % e",U[i][j]) ;
          //   }
          //   printf("\n") ;
          // }

          //J_gje_sol

          printf("\n") ;
          printf("ddgsdsdpc:\n") ;      
          for (j = 0 ; j < 9 ; j++) {
            printf(" %.10e",ddgsdsdpc[j]) ;
          }
          printf("\n") ;
          
          printf("dpcsds:\n") ;
          for (j = 0 ; j < 9 ; j++) {
              printf(" %.*e",DBL_DIG, dpcsds[j]) ;
          }
          printf("\n") ;

          printf("g_sig2_pc2:\n") ;
          for (j = 0 ; j < 9 ; j++) {
              printf(" %.*e",DBL_DIG, g_sig2_pc2[j]) ;
          }
          printf("\n") ;

          printf("g_sig_pc2:\n") ;
          
          printf(" %.*e",DBL_DIG, g_sig_pc2) ;
          
          printf("\n") ;

          printf("g_sig3_pc3:\n") ;
          for (j = 0 ; j < 9 ; j++) {
              printf(" %.*e",DBL_DIG, g_sig3_pc3[j]) ;
          }
          printf("\n") ;

          printf("g_sig_pc3:\n") ;
          
          printf(" %.*e",DBL_DIG, g_sig_pc3) ;
          
          printf("\n") ;

          
          printf("ddgsdsds:\n") ;
          for(i = 0 ; i < 9 ; i++) {          
            for (j = 0 ; j < 9 ; j++) {
              printf(" %.*e",8, ddgsdsds[i*9 + j]) ;
            }
            printf("\n") ;
          }

          //g_sig2_pc2[i] - g2[i] - g_sig_pc2 - g3[i] - g_sig_pc3 + g_sig3_pc3[i]
          // printf("g_back:\n") ;
          // for (j = 0 ; j < 9 ; j++) {
          //     printf(" %.*e",DBL_DIG, g3[j]) ;
          // }
          // printf("\n") ;
        }
        // printf("C_inv:\n") ;
        // {
        //   int i ;
          
        //   for(i = 0 ; i < 9 ; i++) {
        //     int j = i - (i/3)*3 ;
              
        //     printf("C%d%d--:",i/3 + 1,j + 1) ;
              
        //     for (j = 0 ; j < 9 ; j++) {
        //       printf(" % e",C_inv[i*9 + j]) ;
        //     }
              
        //     printf("\n") ;
        //   }
        // }
        
      }

      // if(nf> 0) {  
      //   printf("\n") ;
      // }
      // if(fcrit < tol) {
      //   printf("\n") ;
      //   Message_Direct("converged after %i iters\n",nf+1) ;
      // }
      // if(nf==0 && dich_counter==0) printf("--------new RM-sequence \n") ;

      // printf("I %i \n",nf) ;
      // printf("softening_off1= %4.3e \n",softening_off1) ;
      // printf("softening_off2= %4.3e \n",softening_off2) ;
      // printf("crit = % e \n",crit) ;
      // printf("p_star= % e \n",p_star) ;
      // printf("p_t_scaled = % e \n",p_t_tilde) ;
      // printf("pc_t = % e \n",pc_t) ;
      // printf("pc = % e \n",pc) ;
      // printf("deps_p_v = % e \n",depspv) ;
      // printf("sig_t: ") ;
      // for (j = 0 ; j < 9 ; j++) {
      //     printf(" % e",sig_t[j]) ;
      // }
      // printf("\n") ;
      // printf("sig: ") ;
      // for (j = 0 ; j < 9 ; j++) {
      //     printf(" % e",sig[j]) ;
      // }
      // printf("\n") ;

      // printf("Reduced Jacobian:\n") ;
      // for(i = 0 ; i < 7 ; i++) {          
      //   for (j = 0 ; j < 7 ; j++) {
      //     printf(" % e",J_red[i*7 + j]) ;
      //   }
      //   printf("\n") ;
      // }
      
      // printf("Inverse reduced Jacobian:\n") ;
      // for(i = 0 ; i < 7 ; i++) {          
      //   for (j = 0 ; j < 7 ; j++) {
      //     printf(" % e",J_gje_sol[i*7 + j]) ;
      //   }
      //   printf("\n") ;
      // }

      // printf("dx = ") ;      
      // for (j = 0 ; j < 7 ; j++) {
      //   printf(" % e",dx_rm[j]) ;
      // }
      // printf("\n") ;

      // printf("residual = ") ;      
      // for (j = 0 ; j < 7 ; j++) {
      //   printf(" % e",R_rm[j]) ;
      // }
      // printf("\n") ;


      // printf("ddgsdsdpc: ") ;      
      // for (j = 0 ; j < 9 ; j++) {
      //   printf(" % e",ddgsdsdpc[j]) ;
      // }
      // printf("\n") ;
      
      // printf("dpcsds: ") ;
      // for (j = 0 ; j < 9 ; j++) {
      //     printf(" % e", dpcsds[j]) ;
      // }
      // printf("\n") ;

      // printf("dfsdpc: ") ;
      //   printf(" % e", dfsdpc) ;
      // printf("\n") ;



      // if (nf > 10) {//!deactivated

      //   for (j = 0 ; j < 9; j++) {
      //     sig_new[j] = sig[j] ;

      //   }

      //   //mean of sigma n and n-1, //!store old sigma!
      //   for (j = 0 ; j < 9; j++) {
      //     sig[j] = ( sig_old[j] + sig_new[j] ) /2 ;
      //   }

      //   //calculate dl
      //   double depsp1 ;
      //   for (j = 0 ; j < 9; j++) {
      //     depsp1 += T2_9(C_inv,i,j) * (sig_t[j] - sig[j]);
      //   }

      //   // here we don't update crit, since we would like to continue to run the NR-RM once more
      //   printf("\n") ;
      //   printf("Dichotomie %i activated \n",dich_counter) ;
      //   printf("crit= %4.3e \n",crit) ;
      //   printf("fcrit= %4.3e \n",fcrit) ;
      //   printf("sig_(n-1): ") ;
      //   for (j = 0 ; j < 9 ; j++) {
      //     printf(" % e",sig_old[j]) ;
      //   }
      //   printf("\n") ;
      //   printf("sig_(n): ") ;
      //   for (j = 0 ; j < 9 ; j++) {
      //     printf(" % e",sig_new[j]) ;
      //   }
      //   printf("\n") ;
      //   printf("sig_mean: ") ;
      //   for (j = 0 ; j < 9 ; j++) {
      //     printf(" % e",sig[j]) ;
      //   }
      //   printf("\n") ;


        
      //   if (dich_counter++ > 5) {
      //     printf("reached %i dichotomie iterations\n",dich_counter-1) ;
      //     stop_crit=1; 
      //   }
      //   nf=0; //reset global NR counter
      // }

      // for (j = 0 ; j < 9; j++) {
      //   sig_old[j] = sig[j] ;
      // }

      // if (abs(sig[0]-sig_t[0]) > 1.e6) {
      //   stop_crit=1;  //! stop if too much stress change, unscaled values!
      //   printf("too much stress change \n") ;
      // }
      //if (dl < 0) stop_crit=1;  
      // if(pc-pc_t>1e5 ) {
      //   //stop_crit = 1 ;
      //   printf("large softening detected \n") ;
      // }

      // if max iterations reached even with softening off, stop 


      if (nf++ >= 50 && softening_off2 == 1.) stop_crit=1; 

      if (nf >= 20 && pc > pc_t) {// if max iterations reached, forced restart with softening off
        softening_off2 = 1. ;
        dl    = 0. ;
        pc = pc_t;
        for(i = 0 ; i < 9 ; i++) sig[i] = sig_t[i] ;

        crit = f_fun(plasty,sig,pc) ;
        fcrit = crit ;
        nf = 0;
      }
      
      if (fcrit <= tol && pc > pc_t) {     //if converged but softening occured, forced restart with softening off
        //printf("restart NR with deactivated softening\n") ;
        softening_off2 = 1. ;
        dl    = 0. ;
        pc = pc_t;
        for(i = 0 ; i < 9 ; i++) sig[i] = sig_t[i] ;

        crit = f_fun(plasty,sig,pc) ;
        fcrit = crit ;
        nf = 0;
      }

      if (nf == 30 && softening_off2 == 1.) tol *= 10; 
      if (nf == 40 && softening_off2 == 1.) tol *= 10; 

      if (crit != crit) stop_crit=1;  //stop if crit is NaN
      //if (nf++ >= 30) stop_crit=1; 

      //stop after too many iterations
      if(stop_crit==1) {  
        printf("\n") ;
        printf("I %i ",nf-1) ;
        printf("crit= %4.3e \n",crit) ;
        printf("fcrit= %4.3e \n",fcrit) ;
        printf("softening_off1= %4.3e \n",softening_off1) ;
        printf("softening_off2= %4.3e \n",softening_off2) ;
        printf("p_t= %4.3e MPa ",(p_t)/1.e6) ;
        printf("q_t= %4.3e MPa ",(q_t)/1.e6) ;
        printf("\n") ;
        printf("p= %4.3e MPa ",(p)/1.e6) ;
        printf("q= %4.3e MPa ",(q)/1.e6) ;
        printf("\n") ;
        printf("pc_t= %4.3e MPa ",pc_t/1.e6) ;
        printf("\n") ;
        printf("pc= %4.3e MPa ",pc/1.e6) ;
        printf("\n") ;
        printf("dl= %4.3e ",dl) ;
        printf("\n") ;
        
        {
          int i ;
          int j ; 

          printf("sig_t:\n") ;
          for (j = 0 ; j < 9 ; j++) {
              printf(" % e",sig_t[j]) ;
          }
          printf("\n") ;
          printf("sig:\n") ;
          for (j = 0 ; j < 9 ; j++) {
              printf(" % e",sig[j]) ;
          }
          printf("Reduced Jacobian:\n") ;
          for(i = 0 ; i < 7 ; i++) {          
            for (j = 0 ; j < 7 ; j++) {
              printf(" % e",J_red[i*7 + j]) ;
            }
            printf("\n") ;
          }
          
          printf("Inverse reduced Jacobian:\n") ;
          for(i = 0 ; i < 7 ; i++) {          
            for (j = 0 ; j < 7 ; j++) {
              printf(" % e",J_gje_sol[i*7 + j]) ;
            }
            printf("\n") ;
          }
        }
        //-----------------------------------------------
        Message_FatalError("Plasticity_ReturnMappingACC: no convergence") ;
        printf("Plasticity_ReturnMappingACC: no convergence, continue anyway\n") ;
      }




    }
  }
  // if(dl < 0) {
  //   printf("dl = %e",dl) ;
  //   printf("\n") ;
  // }  

  


  /*
    Stresses and plastic strains and yield criterion update
  */
  
  {
    // - stress update is not necessary as we already work with the updated complete stress tensor sig[i]
    // - crit has been calculated before the loop and also been updated during the Newton Raphson iterations
    // - total plastic strains are updated with the increment depsp[i] obtained during NR  
    for (i = 0 ; i < 9; i++) eps_p[i] += depsp[i] ;    //changed to non associative
  }
  
  /* Consolidation pressure */
  // if(pc>pc_t ) {
  //   //printf("softening hard correct \n"); 
  //   //printf("pc = % e \n",pc) ;
  //   //printf("pc = pc_t = % e \n",pc_t) ;
  //   pc=pc_t ;
  // }
  hardv[0] = pc ;
  return(crit) ;
}


double f_fun(Plasticity_t* plasty,const double* sig,const double pc)
{
  double m       = Plasticity_GetACC_M(plasty)  ;
  double n       = Plasticity_GetACC_N(plasty)  ;
  double k       = Plasticity_GetACC_k(plasty)   ;
  double ps      = Plasticity_GetInitialIsotropicTensileLimit(plasty) ;
  double beta_eps=  Plasticity_GetVolumetricStrainHardeningParameter(plasty) ;

  //grab parameters f_par and f_perp
  double f_par       = Plasticity_GetACC_f_par(plasty)   ;
  double f_perp      = Plasticity_GetACC_f_perp(plasty)   ;
  
  //scale sig
  int axis_3  = Elasticity_GetAxis3(Plasticity_GetElasticity(plasty)) ; // axis 0,1,2
  double sig_tilde[9] = {0};
  int i ;
  for (i = 0 ; i < 9; i++) sig_tilde[i] = sig[i] ; 
  sig_tilde[0] = sig[0]*f_par;
  sig_tilde[4] = sig[4]*f_par;
  sig_tilde[8] = sig[8]*f_par;
  sig_tilde[axis_3*3+axis_3] = sig[axis_3*3+axis_3]*f_perp;

  //double p    = (sig[0] + sig[4] + sig[8])/3. ;  
  //double q    = sqrt(3*Math_ComputeSecondDeviatoricStressInvariant(sig)) ;
  //evaluate p and q for scaled sig_tilde
  double p    = (sig_tilde[0] + sig_tilde[4] + sig_tilde[8])/3. ;  
  double q    = sqrt(3*Math_ComputeSecondDeviatoricStressInvariant(sig_tilde)) ;
  double f    = q*q*exp(k*(2*p+ps-pc)/(ps-pc))+m*m*(p+ps)*(p-pc);
  return(f) ;
}


double g_fun(Plasticity_t* plasty,const double* sig,const double pc)
{
  double m       = Plasticity_GetACC_M(plasty)  ;
  double n       = Plasticity_GetACC_N(plasty)  ;
  double k       = Plasticity_GetACC_k(plasty)   ;
  double ps      = Plasticity_GetInitialIsotropicTensileLimit(plasty) ;
  double beta_eps=  Plasticity_GetVolumetricStrainHardeningParameter(plasty) ;

  //grab parameters f_par and f_perp
  double f_par       = Plasticity_GetACC_f_par(plasty)   ;
  double f_perp      = Plasticity_GetACC_f_perp(plasty)   ;
  
  //scale sig
  int axis_3  = Elasticity_GetAxis3(Plasticity_GetElasticity(plasty)) ; // axis 0,1,2
  double sig_tilde[9] = {0};
  int i ;
  for (i = 0 ; i < 9; i++) sig_tilde[i] = sig[i] ; 
  sig_tilde[0] = sig[0]*f_par;
  sig_tilde[4] = sig[4]*f_par;
  sig_tilde[8] = sig[8]*f_par;
  sig_tilde[axis_3*3+axis_3] = sig[axis_3*3+axis_3]*f_perp;

  //double p    = (sig[0] + sig[4] + sig[8])/3. ;  
  //double q    = sqrt(3*Math_ComputeSecondDeviatoricStressInvariant(sig)) ;
  //evaluate p and q for scaled sig_tilde
  double p    = (sig_tilde[0] + sig_tilde[4] + sig_tilde[8])/3. ;  
  double q    = sqrt(3*Math_ComputeSecondDeviatoricStressInvariant(sig_tilde)) ;
  double g    = q*q*exp(k*(2*p+ps-pc)/(ps-pc))+n*n*(p+ps)*(p-pc);

  // if(g != g) { 
  //   int i ;
  //   int j ; 
  //   printf("\n") ;
  //   printf("sig:\n") ;
  //   for (j = 0 ; j < 9 ; j++) {
  //       printf(" %.*e",DBL_DIG, sig[j]) ;
  //   }
  //   printf("\n") ;
  //   printf("sig_tilde:\n") ;
  //   for (j = 0 ; j < 9 ; j++) {
  //       printf(" %.*e",DBL_DIG, sig_tilde[j]) ;
  //   }
  //   printf("\n") ;
  //   printf("p = %.*e",DBL_DIG, p) ;
  //   printf("\n") ;
  //   printf("q = %.*e",DBL_DIG, q) ;
  //   printf("\n") ;
  //   printf("pc = %.*e",DBL_DIG, pc) ;
  //   printf("\n") ;
  //   printf("g = %.*e",DBL_DIG, g) ;
  //   // printf("\n") ;
  //   // printf("Jacobian:\n") ;
  //   // for(i = 0 ; i < 10 ; i++) {          
  //   //   for (j = 0 ; j < 10 ; j++) {
  //   //     printf(" % e",J_red_temp[i*10 + j]) ;
  //   //   }
  //   //   printf("\n") ;
  //   // }
    
  // }




  return(g) ;
}


double pc_fun(Plasticity_t* plasty,const double* sig, const double* sig_t, const double* C_inv, const double pc_t)
{
  double m       = Plasticity_GetACC_M(plasty)  ;
  double n       = Plasticity_GetACC_N(plasty)  ;
  double k       = Plasticity_GetACC_k(plasty)   ;
  double ps      = Plasticity_GetInitialIsotropicTensileLimit(plasty) ;
  double beta_eps=  Plasticity_GetVolumetricStrainHardeningParameter(plasty) ;

  double p    = (sig[0] + sig[4] + sig[8])/3. ;  
  double q    = sqrt(3*Math_ComputeSecondDeviatoricStressInvariant(sig)) ;

  int id[9] = {1,0,0,0,1,0,0,0,1} ;
  #define T2_9(a,i,j)      ((a)[(i) * 9 + (j)])  //C(i,j)  (cijkl[(i)*9+(j)])

  double depsp[9] = { 0 } ;
  double depspv = 0. ;
  int i;
  int j; 
 
  for (i = 0 ; i < 9; i++) {
    depsp[i] = 0. ;
    for (j = 0 ; j < 9; j++) {
      depsp[i] += T2_9(C_inv,i,j) * (sig_t[j] - sig[j]);
    }
    //depspv +=  depsp[i] * id[i] ;
  }   
  depspv =  depsp[0] + depsp[4] + depsp[8] ;

  double pc = pc_t*exp(beta_eps*depspv) ; 
  return(pc) ;
}
