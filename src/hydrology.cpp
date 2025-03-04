// [[Rcpp::interfaces(r,cpp)]]

#include <Rcpp.h>
#include "soil.h"
#include <meteoland.h>
using namespace Rcpp;
//Old defaults
//ERconv=0.05, ERsyn = 0.2
//New defaults
//Rconv = 5.6, Rsyn = 1.5
// [[Rcpp::export("hydrology_erFactor")]]
double erFactor(int doy, double pet, double prec, double Rconv = 5.6, double Rsyn = 1.5){
  double Ri = 0.0; //mm/h
  if((doy<=120)|(doy>=335)) {
    Ri = std::max(prec/24.0,Rsyn);
  } else {
    Ri = std::max(prec/24.0,Rconv);
  }
  double Ei =pet/24.0;
  return(Ei/Ri);
}

// [[Rcpp::export("hydrology_soilEvaporationAmount")]]
double soilEvaporationAmount(double DEF,double PETs, double Gsoil){
  double t = pow(DEF/Gsoil, 2.0);
  double Esoil = 0.0;
  Esoil = std::min(Gsoil*(sqrt(t+1)-sqrt(t)), PETs);
  return(Esoil);
}

// [[Rcpp::export("hydrology_soilEvaporation")]]
NumericVector soilEvaporation(List soil, String soilFunctions, double pet, double LgroundSWR,
                              bool modifySoil = true) {
  NumericVector W = soil["W"]; //Access to soil state variable
  NumericVector dVec = soil["dVec"];
  NumericVector Water_FC = waterFC(soil, soilFunctions);
  int nlayers = W.size();
  NumericVector EsoilVec(nlayers,0.0);
  double swe = soil["SWE"]; //snow pack
  if(swe == 0.0) {
    double PETsoil = pet*(LgroundSWR/100.0);
    double Gsoil = soil["Gsoil"];
    double Ksoil = soil["Ksoil"];
    double Esoil = soilEvaporationAmount((Water_FC[0]*(1.0 - W[0])), PETsoil, Gsoil);
    for(int l=0;l<nlayers;l++) {
      double cumAnt = 0.0;
      double cumPost = 0.0;
      for(int l2=0;l2<l;l2++) cumAnt +=dVec[l2];
      cumPost = cumAnt+dVec[l];
      //Exponential decay to divide bare soil evaporation among layers
      if(l<(nlayers-1)) EsoilVec[l] = Esoil*(exp(-Ksoil*cumAnt)-exp(-Ksoil*cumPost));
      else EsoilVec[l] = Esoil*exp(-Ksoil*cumAnt);
      if(modifySoil) W[l] = W[l] - ((EsoilVec[l])/Water_FC[l]);
    }
  }
  return(EsoilVec);
}

// [[Rcpp::export(".hydrology_infiltrationAmount")]]
double infiltrationAmount(double input, double Ssoil) {
  double I = 0;
  if(input>0.2*Ssoil) {
    I = input-(pow(input-0.2*Ssoil,2.0)/(input+0.8*Ssoil));
  } else {
    I = input;
  }
  return(I);
}

/**
 * Calculates infiltrated water that goes to each layer
 */
// [[Rcpp::export("hydrology_infiltrationRepartition")]]
NumericVector infiltrationRepartition(double I, NumericVector dVec, NumericVector macro, 
                                      double a = -0.005, double b = 3.0) {
  int nlayers = dVec.length();
  NumericVector Pvec = NumericVector(nlayers,0.0);
  NumericVector Ivec = NumericVector(nlayers,0.0);
  double z1 = 0.0;
  double p1 = 1.0;
  for(int i=0;i<nlayers;i++) {
    double ai = a*pow(1.0-macro[i],b);
    if(i<(nlayers-1)) {
      Pvec[i] = p1*(1.0-exp(ai*dVec[i]));
    } else {
      Pvec[i] = p1;
    }
    p1 = p1*exp(ai*dVec[i]);
    z1 = z1 + dVec[i];
    Ivec[i] = I*Pvec[i];
  }
  return(Ivec);
}


// [[Rcpp::export(".hydrology_interceptionGashDay")]]
double interceptionGashDay(double Precipitation, double Cm, double p, double ER=0.05) {
  double I = 0.0;
  double PG = (-Cm/(ER*(1.0-p)))*log(1.0-ER); //Precipitation need to saturate the canopy
  if(Cm==0.0 || p==1.0) PG = 0.0; //Avoid NAs
  if(Precipitation>PG) {
    I = (1-p)*PG + (1-p)*ER*(Precipitation-PG);
  } else {
    I = (1-p)*Precipitation;
  }
  return(I);
}


// [[Rcpp::export("hydrology_snowMelt")]]
double snowMelt(double tday, double rad, double LgroundSWR, double elevation) {
  if(NumericVector::is_na(rad)) stop("Missing radiation data for snow melt!");
  if(NumericVector::is_na(elevation)) stop("Missing elevation data for snow melt!");
  double rho = meteoland::utils_airDensity(tday, meteoland::utils_atmosphericPressure(elevation));
  double ten = (86400.0*tday*rho*1013.86*1e-6/100.0); //ten can be negative if temperature is below zero
  double ren = (rad*(LgroundSWR/100.0))*(0.1); //90% albedo of snow
  double melt = std::max(0.0,(ren+ten)/0.33355); //Do not allow negative melting values
  return(melt);
}

// [[Rcpp::export("hydrology_soilWaterInputs")]]
NumericVector soilWaterInputs(List soil, String soilFunctions, double prec, double er, double tday, double rad, double elevation,
                             double Cm, double LgroundPAR, double LgroundSWR, 
                             double runon = 0.0,
                             bool snowpack = true, bool modifySoil = true) {
  //Soil input
  double swe = soil["SWE"]; //snow pack

  //Snow pack dynamics
  double snow = 0.0, rain=0.0;
  double melt = 0.0;
  if(snowpack) {
    //Turn rain into snow and add it into the snow pack
    if(tday < 0.0) { 
      snow = prec; 
      swe = swe + snow;
    } else {
      rain = prec;
    }
    //Apply snow melting
    if(swe > 0.0) {
      melt = std::min(swe, snowMelt(tday, rad, LgroundSWR, elevation));
      // Rcout<<" swe: "<< swe<<" temp: "<<ten<< " rad: "<< ren << " melt : "<< melt<<"\n";
      swe = swe-melt;
    }
  } else {
    rain = prec;
  }
  
  //Hydrologic input
  double NetRain = 0.0, Interception = 0.0;
  if(rain>0.0)  {
    Interception = interceptionGashDay(rain,Cm,LgroundPAR/100.0,er);
    NetRain = rain - Interception; 
  }
  if(modifySoil) {
    soil["SWE"] = swe;
  }
  NumericVector WI = NumericVector::create(_["Rain"] = rain, _["Snow"] = snow,
                                           _["Interception"] = Interception,
                                           _["NetRain"] = NetRain, 
                                           _["Snowmelt"] = melt,
                                           _["Runon"] = runon,
                                           _["Input"] = runon+melt+NetRain);
  return(WI);
}
// [[Rcpp::export("hydrology_soilInfiltrationPercolation")]]
NumericVector soilInfiltrationPercolation(List soil, String soilFunctions, 
                                          double waterInput,
                                          bool rockyLayerDrainage = true, bool modifySoil = true) {
  //Soil input
  NumericVector W = clone(Rcpp::as<Rcpp::NumericVector>(soil["W"])); //Access to soil state variable
  NumericVector dVec = soil["dVec"];
  NumericVector macro = soil["macro"];
  NumericVector rfc = soil["rfc"];
  double Kdrain = soil["Kdrain"];
  NumericVector Water_FC = waterFC(soil, soilFunctions);
  NumericVector Water_SAT = waterSAT(soil, soilFunctions);
  int nlayers = W.size();
  

  //Hydrologic input
  double Infiltration= 0.0, Runoff= 0.0, DeepDrainage= 0.0;
  if(waterInput>0.0) {
    //Interception
    //Net Runoff and infiltration
    Infiltration = infiltrationAmount(waterInput, Water_FC[0]);
    Runoff = waterInput - Infiltration;
    //Decide infiltration repartition among layers
    NumericVector Ivec = infiltrationRepartition(Infiltration, dVec, macro);
    //Input of the first soil layer is infiltration
    double percolationExcess = 0.0;
    double Wn;
    for(int l=0;l<nlayers;l++) {
      if((dVec[l]>0.0) & (Ivec[l]>0.0)) {
        Wn = W[l]*Water_FC[l] + Ivec[l]; //Update water volume
        if(l<(nlayers-1)) {
          Ivec[l+1] = Ivec[l+1] + std::max(Wn - Water_FC[l],0.0); //update Ivec adding the excess to the infiltrating water (saturated flow)
          W[l] = std::max(0.0,std::min(Wn, Water_FC[l])/Water_FC[l]); //Update theta (this modifies 'soil')
        } else {
          if((rfc[l]<95.0) | rockyLayerDrainage) { //If not a rock layer or rocky layer drainage is allowed
            W[l] = std::max(0.0,std::min(Wn, Water_FC[l])/Water_FC[l]); //Update theta (this modifies 'soil')
            percolationExcess = std::max(Wn - Water_FC[l],0.0); //Set excess of the bottom layer using field capacity
          } else {
            W[l] = std::max(0.0,std::min(Wn, Water_SAT[l])/Water_FC[l]); //Update theta (this modifies 'soil')
            percolationExcess = std::max(Wn - Water_SAT[l],0.0); //Set excess of the bottom layer using saturation
          }
        }
      } 
    } 
    //If there still excess fill layers over field capacity
    if(percolationExcess>0.0) {
      for(int l=(nlayers-1);l>=0;l--) {
        if((dVec[l]>0.0) & (percolationExcess>0.0)) {
          Wn = W[l]*Water_FC[l] + percolationExcess; //Update water volume
          percolationExcess = std::max(Wn - Water_SAT[l],0.0); //Update excess, using the excess of water over saturation
          W[l] = std::max(0.0,std::min(Wn, Water_SAT[l])/Water_FC[l]); //Update theta (this modifies 'soil') here no upper
        }
      }
      //If soil is completely saturated increase surface Runoff
      if(percolationExcess>0.0) { 
        Runoff = Runoff + percolationExcess;
      }
    }
  }
  //If there is still room for additional drainage (water in macropores accumulated from previous days)
  double head = 0.0;
  for(int l=0;l<nlayers;l++) { //Add mm over field capacity
    if((l<(nlayers-1)) | rockyLayerDrainage) {
      head += Water_FC[l]*std::max(W[l] - 1.0, 0.0);
    }
  }
  // Rcout<<head<<"\n";
  if(head>0.0) {
    double maxDrainage = head*Kdrain;
    for(int l=0;l<nlayers;l++) {
      if(maxDrainage>0.0) {
        double Wn = W[l]*Water_FC[l];
        double toDrain = std::min(std::max(Wn - Water_FC[l], 0.0), maxDrainage);
        if((l==(nlayers-1)) & (rfc[l] >= 95.0) & (!rockyLayerDrainage)) { //Prevent drainage for last rocky layer if not allowed
          toDrain = 0.0;
        }
        if(toDrain > 0.0) {
          DeepDrainage +=toDrain;
          maxDrainage -=toDrain;
          Wn -= toDrain;
          W[l] = std::max(0.0, Wn/Water_FC[l]); //Update theta (this modifies 'soil') here no upper          
        }
      }
    }
  }
  if(modifySoil) {
    NumericVector Ws = soil["W"];
    for(int l=0;l<nlayers;l++) Ws[l] = W[l];
  }
  NumericVector DB = NumericVector::create(_["Infiltration"] = Infiltration, _["Runoff"] = Runoff, _["DeepDrainage"] = DeepDrainage);
  return(DB);
}