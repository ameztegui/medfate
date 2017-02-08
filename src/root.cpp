#include <numeric>
#include <Rcpp.h>
using namespace Rcpp;
using namespace std;

/**
 *  Root distribution
 */
NumericVector ldrRS_one(double Z50, double Z95, NumericVector d){
  int nlayer = d.size();
  NumericVector dCum = clone(d);
  NumericVector Vd(nlayer);
  double c = 2.94/log(Z50/Z95);
  double Vtot = 0.0;
  Vd[0] = 1.0/(1.0+pow(d[0]/Z50,c));
  Vtot = Vd[0];
  for(int i=1;i<nlayer;i++) dCum[i] = dCum[i]+dCum[i-1];
  for(int i=1;i<nlayer;i++){
    Vd[i] = 1.0/(1.0+pow(dCum[i]/Z50,c)) -1.0/(1.0+pow(dCum[i-1]/Z50,c));
    Vtot +=Vd[i];
  }
  //Rescale proportions so that they sum 1
  for(int i=0;i<nlayer; i++) {
    Vd[i] = Vd[i]/Vtot;
  }
  return(Vd);
}
NumericVector conicRS_one(double Z, NumericVector d){
  int nlayers  = d.size();
  NumericVector Zd(nlayers,0.0);
  NumericVector Vd(nlayers,0.0);
  
  //Proportion of root length on each layer
  double sumZd=0.0;
  for(int l=0;l<nlayers;l++) {
     if(l==0) Zd[l] = std::min(d[l],Z)/Z;
     else if(l==(nlayers-1)) Zd[l] = 1.0-sumZd;
     else {
       Zd[l] = std::max(std::min(d[l]/Z,1.0-sumZd),0.0);  
     }
     sumZd = sumZd+Zd[l];
  }

  //Proportion of fine roots in each layer (assuming soil depth = maximum root depth)
  for(int l1=0;l1<nlayers;l1++) {
    if(l1==0) {// Vd[0] = 1.0-pow(Zd[1]+Zd[2],3.0);
      sumZd = 0.0;
      for(int l2=1;l2<nlayers;l2++) sumZd += Zd[l2];
      // Rcout<<"\n "<<l1<<" S "<<sumZd<<"\n";
      Vd[l1] = 1.0 - pow(sumZd,3.0);
    } else if(l1==(nlayers-1)) {
      // Vd[2] = pow(1.0-Zd[0]-Zd[1],3.0);
      sumZd = 0.0;
      for(int l2=0;l2<l1;l2++) {
        sumZd += Zd[l2]; 
        // Rcout<<"\n "<<l2<<" S "<<sumZd<<"\n";
      }
      Vd[l1] = pow(1.0-sumZd, 3.0);
    } else {
      // Vd[1] = pow(1.0-Zd[0],3.0)-pow(Zd[2],3.0);
      double sumZd1 = 0.0, sumZd2 = 0.0;
      for(int l2=0;l2<l1;l2++) sumZd1 += Zd[l2];
      for(int l2=l1+1;l2<nlayers;l2++) sumZd2 += Zd[l2];
      Vd[l1] = pow(1.0-sumZd1,3.0)-pow(sumZd2,3.0);
    }
    // Rcout<<"("<<Vd[l1]<<", "<<Zd[l1]<<")\n";
  }
  // Rcout<<"\n";
  //Remove non-existing part
  double Zsoil = 0.0;
  for(int l=0;l<nlayers;l++) Zsoil +=d[l];
  double Zreal = std::max((Z-Zsoil)/Z,0.0);  
  double Vrem = pow(Zreal,3.0);
  double Vtot = 0.0;
  for(int i=(nlayers-1);i>=0; i--) {
    Vd[i] = Vd[i]-Vrem;
    Vrem = (-1)*std::min(Vd[i],0.0);
    Vd[i] = std::max(Vd[i],0.0);
    Vtot +=Vd[i];
  }
  
  //Rescale proportions so that they sum 1
  for(int l=0;l<nlayers; l++) {
    Vd[l] = Vd[l]/Vtot;
  }
  return(Vd);
}

// [[Rcpp::export("root.conicDistribution")]]
NumericMatrix conicDistribution(NumericVector Z, NumericVector d) {
  int numCohorts = Z.size();
  NumericMatrix P(numCohorts,d.size());
  for(int c=0;c<numCohorts;c++){
    NumericVector PC = conicRS_one(Z[c],d);
    for(int i=0;i<PC.size();i++) P(c,i) = PC[i];
  }
  return(P);
}

/**
 * Root distribution according to the LDR model
 * 
 * Schenk, H., Jackson, R., 2002. The global biogeography of roots. Ecol. Monogr. 72, 311–328.
 */
// [[Rcpp::export("root.ldrDistribution")]]
NumericMatrix ldrDistribution(NumericVector Z50, NumericVector Z95, NumericVector d) {
  int numCohorts = Z50.size();
  NumericMatrix P(numCohorts,d.size());
  NumericVector PC(d.size());
  for(int c=0;c<numCohorts;c++){
    PC = ldrRS_one(Z50[c], Z95[c],d);
    for(int i=0;i<d.size();i++) P(c,i) = PC[i];
  }
  return(P);
}


/**
 * Proportions of root xylem conductance
 * 
 * Calculates the proportion of total xylem conductance that corresponds to each layer in a network of 
 * parallel xylem resistances.
 * 
 * Sperry, J. S., Y. Wang, B. T. Wolfe, D. S. Mackay, W. R. L. Anderegg, N. G. Mcdowell, and W. T. Pockman. 2016. 
 * Pragmatic hydraulic theory predicts stomatal responses to climatic water deficits. 
 * New Phytologist 212:577–589.
 * 
 */
// [[Rcpp::export("root.xylemConductanceProportions")]]
NumericVector xylemConductanceProportions(NumericVector v, NumericVector d, double depthWidthRatio = 1.0) {
  int nlayers = v.size();
  double maxRootDepth = 0.0;
  //Vertical lengths
  NumericVector vl(nlayers), zini(nlayers);
  for(int i=0;i<nlayers;i++) {
    if(i==0) {
      zini[i] = 0.0;
    } else {
      zini[i] = zini[i-1]+ d[i-1];
    }
    if(v[i]>0.0) {
      vl[i] = zini[i]+ d[i]/2.0;
      maxRootDepth +=d[i];
    } else {
      vl[i] = NA_REAL;
    }
    // Rcout<<vl[i]<<" ";
  }
  // Rcout<<"\n";
  int nlayerseff = nlayers;
  for(int i=(nlayers-1);i>=0;i--) if(NumericVector::is_na(vl[i])) nlayerseff = i;

  //Radial lengths
  NumericVector r(nlayerseff), rl(nlayerseff);
  double maxr = 0.0;
  for(int i=0;i<nlayerseff;i++) {
    r[i] = sqrt(v[i]/(d[i]*PI));
    maxr = std::max(r[i],maxr);
  }
  // Rcout<<maxr<<"\n";
  for(int i=0;i<nlayerseff;i++) {
    rl[i] = maxRootDepth*depthWidthRatio*(r[i]/maxr);
    // Rcout<<rl[i]<<" ";
  }
  // Rcout<<"\n";
  //Weights
  NumericVector w(nlayers, 0.0);
  double wsum=0.0;
  for(int i=0;i<nlayerseff;i++) {
    w[i]= 1.0/(rl[i]+vl[i]);
    wsum +=w[i];
  }
  for(int i=0;i<nlayerseff;i++) w[i] = w[i]/wsum;
  return(w);
}
