#include <Rcpp.h>
#include <math.h> 
using namespace Rcpp;

/* K-Epsilon Models of Katul et al. (2004)
 * Code adapted from Matlab to Rcpp
 * www: https://nicholas.duke.edu/people/faculty/katul/k_epsilon_model.htm
*/

/* (A)
 *   
 * Second order turbulence model closure constants for canopy-layer simulation.
 * Revised by csz: Fri Mar 28 2003
 */

//Rate of MKE lost by drag converted into TKE (in [0, 1])
// (roughly-) Fitted value for canopy-layer velocity profile simulation
const double Bp=1.0;
// Mixing lenght constant (Seginer, 1974; Massman and Weil, 1999)
// alpha=0.09 (Katul and Chang, 1999)
double alphaCNT=0.05;

// ASL Values for sigma_u/u*, sigma_v/u*, sigma_w/u*
// (set upper boundary condition on TKE)
const double AAu=2.3;
const double AAv=2.1;
const double AAw=1.25;
const double Aq=0.5*(pow(AAu,2.0)+pow(AAv,2.0)+pow(AAw,2.0));
      
// Determine the Kolmogorov constant from Au, Av, Aw
// (constant for the turbulent viscocity)
const double Cu=pow(1.0/Aq,2.0);
      
// Von karman constant
const double kv=0.4;
      
// Constants for the dissipation budget (Launder and Spalding, 1974)
const double Ce1=1.44;
const double Ce2=1.92;
      
// Prandtl numbers
// Prandtl number (PrTKE = 1) for TKE hardwired in solver routines.
const double PrTKE=1.0;
// dissipation budget (Detering and Etling, 1974)
const double Pr=pow(kv,2.0)/(sqrt(Cu)*(Ce2-Ce1));
      
// Wake TKE budget coefficients (Sanz, 2003)
const double Cg=pow(2.0/alphaCNT,2.0/3.0);
      
// Shortcircuit in Dissipation (linear in Bp)
const double Bd=sqrt(Cu)*Cg*Bp + 3.0/PrTKE;
      
// Dissipation budget (constant with alpha)
const double Ce4=PrTKE*(2.0/Pr-sqrt(Cu)/6.0*Cg*(Ce2-Ce1));                
      
// Ce5[=1.5]=Ce4 (Green, 1992), not [=0.6]!=Ce4 (Liu et al., 1996).
// *** Proof: see ``A note on k-epsilon modelling...'' (Sanz, 2003)
// *** The '1.5' value above for engineering (e.g. wind tunnel) flows (Cu=0.09)
const double Ce5=Ce4;

/* (B)
 * 
 * tri-diagonal solver needed for the implicit schemes used in solving the non-linear ODEs.
 */
NumericVector thomas(NumericVector aa, NumericVector bb, NumericVector cc, NumericVector dd) {
  int n=bb.length();
  NumericVector bet(n);
  NumericVector gam(n);
  NumericVector q(n);
  bet[0]=bb[0];
  gam[0]=dd[0]/bb[0];
  for(int i=1;i<n;i++){
    bet[i]=bb[i]-(aa[i]*cc[i-1]/bet[i-1]);
    gam[i]=(dd[i]-aa[i]*gam[i-1])/bet[i];
  }
  q[n-1]=gam[n-1];
  for(int i=(n-2); i >=0;i--) {
    q[i]=gam[i]-(cc[i]*q[i+1]/bet[i]);  
  }
  return(q);
}

// Generates a vector with linear spacing
NumericVector linspace(double x1, double x2, int N) {
  NumericVector v(N);
  v[0] = x1;
  double s = (x2 - x1)/((double)(N - 1));
  for(int i=1;i<N;i++) {
    v[i] = v[i-1] + s;
  }
  return(v);
}
// Implementation of R which
IntegerVector which(LogicalVector l) {
  int c = 0;
  for(int i=0;i<l.size();i++) if(l[i]) c++;
  IntegerVector w(c);
  int cnt=0;
  for(int i=0;i<l.size();i++) if(l[i]) {w[cnt] = i;cnt++;}
  return(w);
}
/* (C)
 * 
 *  K-epsilon model (equations 1-7 with equation 4b)
 *  K-U model (equations 9 and 10)
 *  
 *  k_epsilon_CSL(z, Cx, h, do,zo)
 *  output [z1, U1, k1, uw1, Lmix1]
 *  
 *  zm - height vector in m
 *  Effective drag = Cd x leaf area density
 *  hm - canopy height (m)
 *  d0 - displacement height (m)
 *  z0 - Momentum roughness height (m)
 */
// [[Rcpp::export("wind_canopyTurbulenceModel")]]
DataFrame windCanopyTurbulenceModel(NumericVector zm, NumericVector Cx, double hm, double d0, double z0,
                                     String model = "k-epsilon") {
  int N=zm.size();
  double zmax=max(zm);
  double dz=zm[1]-zm[0];
  // ------- Define starting conditions for U/u*, k/(u*^2), epsilon/(u*3/h)
  double Ulow=0.0;
  double Uhigh=(1.0/kv)*log((zmax-d0)/z0);
  NumericVector U=linspace(Ulow,Uhigh,N);
  
  double khigh=0.5*(pow(AAu,2.0)+pow(AAv,2.0)+pow(AAw,2.0));
  double klow=0.001*khigh;
  NumericVector k=linspace(klow,khigh,N);
  
  double epsilonhigh=(1.0/(kv*(zmax-d0)));
  double epsilonlow=0.001*epsilonhigh;
  NumericVector epsilon=linspace(epsilonlow,epsilonhigh,N);

  // Mixing Length model
  double alpha = kv*(hm-d0)/hm;  //fraction of mixing length (Lmixing = alpha x h) inside the canopy up to canopy top
  NumericVector Lmix(N, alpha*hm);
  IntegerVector c1 = which(zm < hm);
  int nn=max(c1);
  for(int i=nn;i<N;i++) {
    Lmix[i] = kv*(zm[i] - d0);
  }
  Lmix[nn]=(Lmix[nn-1]+Lmix[nn+1])/2.0;
  
  double am3=-pow(Aq,3.0)*(pow(AAu,2.0)-pow(AAw,2.0))/(pow(AAw,2.0)-pow(Aq,2.0)/3.0);
  NumericVector lambda3=am3*Lmix;
  
  NumericVector y(N); 
  NumericVector vt(N); //viscosity
  NumericVector dvt(N); //derivative
  NumericVector dU(N); //derivative
  NumericVector upd, dia, lod;
  NumericVector aa(N), bb(N), cc(N), dd(N);
  NumericVector Su(N),Sk(N),Se1(N),Se2(N); 
  NumericVector a1, a2, a3;
  NumericVector Un, Kn, uw;
  double eps1=0.1;
  double maxerr=9999.9;
  
  int cnt=0;
  while(maxerr>0.1) {
    // Viscocity (and derivative) Model
    for(int i=0;i<N;i++) {
      vt[i]=pow(Cu,1.0/4.0)*Lmix[i]*sqrt(abs(k[i])); 
    }
    for(int i=1;i<N;i++) {
      y[i] = (vt[i] - vt[i-1])/dz;
    }
    y[0] = y[1];
    dvt = y;
    //  dU/dz
    for(int i=1;i<N;i++) {
      y[i] = (U[i] - U[i-1])/dz;
    }
    y[0] = y[1];
    dU = y;
    //  Compute the Reynolds stress
    uw= -vt*dU;
    uw[N-1]=uw[N-2];
    uw[0]=uw[1];
    if(model=="k-U") { // Estimate dissipation rate in k-U model
      for(int i=0;i<N;i++) {
        epsilon[i] = (3.0/3.0)*(pow(sqrt(2.0*k[i]),3.0)/lambda3[i]);
      }
    }
    //   Set up coefficients for Mean Momentum ODE-------------------------------------------
    a1=vt;
    a2=dvt;
    a3=-Cx * abs(U);
    double dx=dz;
    //  ------ Set the elements of the Tri-diagonal Matrix
    upd=(a1/(dx*dx)+a2/(2.0*dx));
    dia=(-a1*2.0/(dx*dx)+a3);
    lod=(a1/(dx*dx)-a2/(2.0*dx));
    for(int i=0;i<N;i++) {
      aa[i]=lod[i];
      bb[i]=dia[i];
      cc[i]=upd[i];
      dd[i] = 0.0;
    }
    aa[0]=0.0;
    bb[0]=1.0;
    cc[0]=-1.0;
    dd[0]= 0.0;
    aa[N-1]=0.0;
    bb[N-1]=1.0;
    cc[N-1]=0.0;
    dd[N-1]=Uhigh;
    //   Use the Thomas Algorithm to solve the tridiagonal matrix
    Un=thomas(aa,bb,cc,dd);
    //   Use successive relaxations in iterations
    U=abs(eps1*Un+(1.0-eps1)*U);
    
    //   Set up coefficients for TKE ODE------------------------------------------------------------
    a1=vt;
    a2=dvt;
    a3=-Bd*abs(U)*Cx;
    dx=dz;
    //  ------ Set the elements of the Tri-diagonal Matrix
    upd=(a1/(dx*dx)+a2/(2.0*dx));
    dia=(-a1*2.0/(dx*dx)+a3);
    lod=(a1/(dx*dx)-a2/(2.0*dx));
    for(int i=0;i<N;i++) {
      aa[i]=lod[i];
      bb[i]=dia[i];
      cc[i]=upd[i];
      dd[i] = epsilon[i]-Cx[i]*Bp*pow(abs(U[i]),3.0)-vt[i]*pow(dU[i],2.0);
    }
    aa[0]=0.0;
    bb[0]=1.0;
    cc[0]=-1.0;
    dd[0]= 0.0;
    aa[N-1]=0.0;
    bb[N-1]=1.0;
    cc[N-1]=0.0;
    dd[N-1]=khigh;

    //   ------Use the Thomas Algorithm to solve the tridiagonal matrix
    Kn=thomas(aa,bb,cc,dd);
    maxerr=max(abs(Kn-k));
    // -----Use successive relaxations in iterations
    k=abs(eps1*Kn+(1.0-eps1)*k);
    
    if(model=="k-epsilon") {
      //   Set up coefficients for dissipation ODE ---------------------------------------------------------------------------------
      for(int i=0;i<N;i++){
        Su[i]=Cx[i]*pow(U[i],2.0); 
        Sk[i]=Cx[i]*(Bp*pow(U[i],3.0)-Bd*U[i]*k[i]);
        Se1[i]=(Ce4*epsilon[i]/k[i])*Sk[i];
        Se2[i]=epsilon[i]*Cx[i]*((Ce4*Bp*pow(U[i],3.0)/k[i])-Bd*Ce5*U[i]);
      }
      a1=vt/Pr;
      a2=dvt/Pr;
      a3=(-Ce2*epsilon/k);
      dx=dz;
      //  ------ Set the elements of the Tri-diagonal Matrix
      upd=(a1/(dx*dx)+a2/(2.0*dx));
      dia=(-a1*2.0/(dx*dx)+a3);
      lod=(a1/(dx*dx)-a2/(2.0*dx));
      for(int i=0;i<N;i++) {
        aa[i]=lod[i];
        bb[i]=dia[i];
        cc[i]=upd[i];
        dd[i] = -Ce1*Cu*k[i]*pow(dU[i],2.0)-Se2[i];
      }
      aa[0]=0.0;
      bb[0]=1.0;
      cc[0]=-1.0;
      dd[0]= 0.0;
      aa[N-1]=0.0;
      bb[N-1]=1.0;
      cc[N-1]=0.0;
      dd[N-1]=epsilonhigh;
      //   Use the Thomas Algorithm to solve the tridiagonal matrix
      NumericVector epsilonn=thomas(aa,bb,cc,dd);
      //   Use successive relaxations in iterations
      epsilon=abs(eps1*epsilonn+(1.0-eps1)*epsilon);
    }
    cnt++;
    if(cnt==100) stop("too many iterations");
  }
  return(DataFrame::create(Named("z1") = zm,
                           Named("U1") = U,
                           Named("dU1") = dU,
                           Named("epsilon1") = epsilon,
                           Named("k1") = k,
                           Named("uw1") = uw/(abs(uw[N-1])),
                           Named("Lmix1") = Lmix));
}
/*
 *   zmid - Vector of mid heights for canopy layers (cm)
 *   LAD - Vector of leaf area density for canopy layers (m2/m3)
 *   canopyHeight - Canopy height (cm)
 *   u - Wind speed over the canopy (m/s)
 *   windMeasurementHeight - Height of wind measurement over canopy
 */
// [[Rcpp::export("wind_canopyTurbulence")]]
DataFrame windCanopyTurbulence(NumericVector zmid, NumericVector LAD, double canopyHeight,
                                double u, double windMeasurementHeight = 200, String model = "k-epsilon") {
  //z - height vector in m
  NumericVector zm = zmid/100.0;
  //Effective drag = Cd x leaf area density
  NumericVector Cx = LAD*0.2; 
  //hm - canopy height (m)
  double hm = (canopyHeight/100.0);
  //d0 - displacement height (m)
  double d0 = 0.67*hm;
  //z0 - Momentum roughness height (m)
  double z0 = 0.08*hm;
  
  //u_f - Friction velocity
  double u_f = u*kv/log(((hm + windMeasurementHeight/100.0)-d0)/z0);
  DataFrame cmout = windCanopyTurbulenceModel(zm, Cx, hm, d0, z0);
  NumericVector U1 = cmout["U1"];
  NumericVector dU1 = cmout["dU1"];
  NumericVector epsilon1 = cmout["epsilon1"];
  NumericVector k1 = cmout["k1"];
  NumericVector uw1 = cmout["uw1"];
  NumericVector Lmix1 = cmout["Lmix1"];
  return(DataFrame::create(Named("zmid") = zmid,
                           Named("u") = U1*u_f,
                           Named("du") = dU1*u_f,
                           Named("epsilon") = epsilon1*(pow(u_f,3.0)/hm),
                           Named("k") = k1*pow(u_f,2.0),
                           Named("uw") = uw1*pow(u_f,2.0)));
}