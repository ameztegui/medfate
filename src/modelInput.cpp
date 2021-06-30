#include <Rcpp.h>
#include "spwb.h"
#include "growth.h"
#include "carbon.h"
#include "root.h"
#include "soil.h"
#include "woodformation.h"
#include "forestutils.h"
#include "paramutils.h"
#include "tissuemoisture.h"
#include "hydraulics.h"
#include "stdlib.h"

using namespace Rcpp;


DataFrame paramsPhenology(DataFrame above, DataFrame SpParams, bool fillMissingSpParams) {
  IntegerVector SP = above["SP"];
  NumericVector LAI_expanded = above["LAI_expanded"];
  NumericVector LAI_live = above["LAI_live"];
  NumericVector LAI_dead = above["LAI_dead"];
  int numCohorts = SP.size();
  CharacterVector phenoType = speciesCharacterParameter(SP, SpParams, "PhenologyType");
  NumericVector leafDuration  = speciesNumericParameter(SP, SpParams, "LeafDuration");
  NumericVector Sgdd  = speciesNumericParameter(SP, SpParams, "Sgdd");
  NumericVector Tbgdd = speciesNumericParameter(SP, SpParams, "Tbgdd");
  NumericVector Ssen = speciesNumericParameter(SP, SpParams, "Ssen");
  NumericVector Phsen = speciesNumericParameter(SP, SpParams, "Phsen");
  NumericVector Tbsen  = speciesNumericParameter(SP, SpParams, "Tbsen");
  if(fillMissingSpParams) {
    for(int j=0; j<numCohorts;j++) {
      if(NumericVector::is_na(Tbgdd[j])) Tbgdd[j]= 5.0;
      if(phenoType[j] == "winter-deciduous" || phenoType[j] == "winter-semideciduous") { //Delpierre et al 2009
        if(NumericVector::is_na(Ssen[j])) Ssen[j] = 8268.0;
        if(NumericVector::is_na(Phsen[j])) Phsen[j] = 12.5;
        if(NumericVector::is_na(Tbsen[j])) Tbsen[j] = 28.5;
        LAI_expanded[j] = 0.0; //Set initial LAI to zero, assuming simulations start at Jan 1st
        if(phenoType[j] == "winter-semideciduous") LAI_dead[j] = LAI_live[j];
      } else if(phenoType[j] == "oneflush-evergreen") {
        if(NumericVector::is_na(Ssen[j])) Ssen[j] = 8268.0;
        if(NumericVector::is_na(Phsen[j])) Phsen[j] = 12.5;
        if(NumericVector::is_na(Tbsen[j])) Tbsen[j] = 28.5;
      }
    }
  }
  DataFrame paramsPhenologydf = DataFrame::create(
    _["PhenologyType"] = phenoType,
    _["LeafDuration"] = leafDuration,
    _["Sgdd"] = Sgdd, _["Tbgdd"] = Tbgdd, 
    _["Ssen"] = Ssen, _["Phsen"] = Phsen, _["Tbsen"] = Tbsen 
  );
  paramsPhenologydf.attr("row.names") = above.attr("row.names");
  return(paramsPhenologydf);
}

DataFrame paramsInterception(DataFrame above, DataFrame SpParams, List control) {
  IntegerVector SP = above["SP"];
  int numCohorts = SP.size();
  
  String transpirationMode = control["transpirationMode"];
  bool fillMissingSpParams = control["fillMissingSpParams"];
  
  NumericVector alphaSWR = speciesNumericParameterWithImputation(SP, SpParams, "alphaSWR", fillMissingSpParams);
  NumericVector gammaSWR = speciesNumericParameterWithImputation(SP, SpParams, "gammaSWR", fillMissingSpParams);
  NumericVector kPAR = speciesNumericParameterWithImputation(SP, SpParams, "kPAR", fillMissingSpParams);
  NumericVector g = speciesNumericParameterWithImputation(SP, SpParams, "g", fillMissingSpParams);
  DataFrame paramsInterceptiondf;
  if(transpirationMode=="Granier") {
    paramsInterceptiondf = DataFrame::create(_["kPAR"] = kPAR, 
                                             _["g"] = g);
  } else {
    paramsInterceptiondf = DataFrame::create(_["alphaSWR"] = alphaSWR,
                                             _["gammaSWR"] = gammaSWR, 
                                             _["kPAR"] = kPAR, 
                                             _["g"] = g);
  }
  paramsInterceptiondf.attr("row.names") = above.attr("row.names");
  return(paramsInterceptiondf);
}


DataFrame paramsAnatomy(DataFrame above, DataFrame SpParams, bool fillMissingSpParams) {
  IntegerVector SP = above["SP"];
  int numCohorts = SP.size();
  
  CharacterVector Group = speciesCharacterParameter(SP, SpParams, "Group");
  
  NumericVector Hmax = speciesNumericParameter(SP, SpParams, "Hmax");
  NumericVector Hmed = speciesNumericParameter(SP, SpParams, "Hmed"); //To correct conductivity
  NumericVector Al2As = speciesNumericParameter(SP, SpParams, "Al2As");
  NumericVector SLA = speciesNumericParameterWithImputation(SP, SpParams, "SLA", fillMissingSpParams);
  NumericVector LeafDensity = speciesNumericParameter(SP, SpParams, "LeafDensity");
  NumericVector WoodDensity = speciesNumericParameter(SP, SpParams, "WoodDensity");
  NumericVector FineRootDensity = speciesNumericParameter(SP, SpParams, "FineRootDensity");
  NumericVector conduit2sapwood(numCohorts, NA_REAL);
  if(SpParams.containsElementNamed("conduit2sapwood")) {
    conduit2sapwood = speciesNumericParameter(SP, SpParams, "conduit2sapwood");
  }
  NumericVector r635 = speciesNumericParameterWithImputation(SP, SpParams, "r635", fillMissingSpParams);
  NumericVector leafwidth = speciesNumericParameterWithImputation(SP, SpParams, "LeafWidth", fillMissingSpParams);
  
  NumericVector SRL = speciesNumericParameter(SP, SpParams, "SRL");  
  NumericVector RLD = speciesNumericParameter(SP, SpParams, "RLD");  
  
  CharacterVector LeafSize = speciesCharacterParameter(SP, SpParams, "LeafSize");
  CharacterVector LeafShape = speciesCharacterParameter(SP, SpParams, "LeafShape");
  
  if(fillMissingSpParams) {
    for(int c=0;c<numCohorts;c++){ //default values for missing data
      if(NumericVector::is_na(WoodDensity[c])) WoodDensity[c] = 0.652;
      if(NumericVector::is_na(LeafDensity[c])) LeafDensity[c] = 0.7;
      if(NumericVector::is_na(FineRootDensity[c])) FineRootDensity[c] = 0.165; 
      if(NumericVector::is_na(conduit2sapwood[c])) {
        if(Group[c]=="Angiosperm") conduit2sapwood[c] = 0.70; //20-40% parenchyma in angiosperms.
        else conduit2sapwood[c] = 0.925; //5-10% parenchyma in gymnosperms (https://link.springer.com/chapter/10.1007/978-3-319-15783-2_8)
      }
      if(NumericVector::is_na(Al2As[c]) && !CharacterVector::is_na(LeafShape[c]) && !CharacterVector::is_na(LeafSize[c])) {
        if(LeafShape[c]=="Linear") {
          Al2As[c]= 2156.0;
        } else if(LeafShape[c]=="Needle") {
          Al2As[c]= 2751.7;
        } else if(LeafShape[c]=="Broad") {
          if(LeafSize[c]=="Small") {
            Al2As[c] = 2284.9;
          } else if(LeafSize[c]=="Medium") {
            Al2As[c] = 2446.1;
          } else if(LeafSize[c]=="Large") {
            Al2As[c]= 4768.7;
          }
        } else if(LeafShape[c]=="Scale") { 
          Al2As[c] = 1696.6;
        }
      }
      if(NumericVector::is_na(SRL[c])) SRL[c] = 3870.0; 
      if(NumericVector::is_na(RLD[c])) RLD[c] = 10.0;
    }
  }
  DataFrame paramsAnatomydf = DataFrame::create(
    _["Hmax"] = Hmax,_["Hmed"] = Hmed,
    _["Al2As"] = Al2As, _["SLA"] = SLA, _["LeafWidth"] = leafwidth, 
    _["LeafDensity"] = LeafDensity, _["WoodDensity"] = WoodDensity, _["FineRootDensity"] = LeafDensity, 
    _["conduit2sapwood"] = conduit2sapwood,
    _["SRL"] = SRL, _["RLD"] = RLD,  
    _["r635"] = r635
  );
  paramsAnatomydf.attr("row.names") = above.attr("row.names");
  return(paramsAnatomydf);
}

DataFrame paramsWaterStorage(DataFrame above, DataFrame SpParams,
                             DataFrame paramsAnatomydf, bool fillMissingSpParams) {
  IntegerVector SP = above["SP"];
  NumericVector H = above["H"];
  int numCohorts = SP.size();
  
  NumericVector LeafPI0 = speciesNumericParameter(SP, SpParams, "LeafPI0");
  NumericVector LeafEPS = speciesNumericParameter(SP, SpParams, "LeafEPS");
  NumericVector LeafAF = speciesNumericParameter(SP, SpParams, "LeafAF");
  NumericVector StemPI0 = speciesNumericParameter(SP, SpParams, "StemPI0");
  NumericVector StemEPS = speciesNumericParameter(SP, SpParams, "StemEPS");
  NumericVector StemAF = speciesNumericParameter(SP, SpParams, "StemAF");
  
  NumericVector Vsapwood(numCohorts), Vleaf(numCohorts);
  
  NumericVector WoodDensity = paramsAnatomydf["WoodDensity"];
  NumericVector LeafDensity = paramsAnatomydf["LeafDensity"];
  NumericVector SLA = paramsAnatomydf["SLA"];
  NumericVector Al2As = paramsAnatomydf["Al2As"];
  
  if(fillMissingSpParams) {
    for(int c=0;c<numCohorts;c++){
      //From: Christoffersen, B.O., Gloor, M., Fauset, S., Fyllas, N.M., Galbraith, D.R., Baker, T.R., Rowland, L., Fisher, R.A., Binks, O.J., Sevanto, S.A., Xu, C., Jansen, S., Choat, B., Mencuccini, M., McDowell, N.G., & Meir, P. 2016. Linking hydraulic traits to tropical forest function in a size-structured and trait-driven model (TFS v.1-Hydro). Geoscientific Model Development Discussions 0: 1–60.
      if(NumericVector::is_na(StemPI0[c])) StemPI0[c] = 0.52 - 4.16*WoodDensity[c]; 
      if(NumericVector::is_na(StemEPS[c])) StemEPS[c] = sqrt(1.02*exp(8.5*WoodDensity[c])-2.89); 
      if(NumericVector::is_na(StemAF[c])) StemAF[c] = 0.8;
      //From: Bartlett MK, Scoffoni C, Sack L (2012) The determinants of leaf turgor loss point and prediction of drought tolerance of species and biomes: a global meta-analysis. Ecol Lett 15:393–405. doi: 10.1111/j.1461-0248.2012.01751.x
      if(NumericVector::is_na(LeafPI0[c])) LeafPI0[c] = -2.0; //Average values for Mediterranean climate species
      if(NumericVector::is_na(LeafEPS[c])) LeafEPS[c] = 17.0;
      if(NumericVector::is_na(LeafAF[c])) LeafAF[c] = 0.29;
    }
  }
  //Calculate stem and leaf capacity per leaf area (in m3·m-2)
  for(int c=0;c<numCohorts;c++){
    Vsapwood[c] = stemWaterCapacity(Al2As[c], H[c], WoodDensity[c]); 
    Vleaf[c] = leafWaterCapacity(SLA[c], LeafDensity[c]); 
  }
  DataFrame paramsWaterStoragedf = DataFrame::create(
    _["LeafPI0"] = LeafPI0, _["LeafEPS"] = LeafEPS, _["LeafAF"] = LeafAF, _["Vleaf"] = Vleaf,
      _["StemPI0"] = StemPI0, _["StemEPS"] = StemEPS, _["StemAF"] = StemAF, _["Vsapwood"] = Vsapwood);
  paramsWaterStoragedf.attr("row.names") = above.attr("row.names");
  
  return(paramsWaterStoragedf);
}

DataFrame paramsTranspirationGranier(DataFrame above,  DataFrame SpParams, bool fillMissingSpParams) {
  IntegerVector SP = above["SP"];
  NumericVector WUE = speciesNumericParameter(SP, SpParams, "WUE");
  NumericVector Psi_Extract = speciesNumericParameter(SP, SpParams, "Psi_Extract");
  NumericVector Psi_Critic = speciesNumericParameter(SP, SpParams, "Psi_Critic");
  NumericVector pRootDisc = speciesNumericParameter(SP, SpParams, "pRootDisc");
  NumericVector Tmax_LAI(SP.size(), NA_REAL);
  
  CharacterVector Group = speciesCharacterParameter(SP, SpParams, "Group");
  CharacterVector Order = speciesCharacterParameter(SP, SpParams, "Order");
  CharacterVector GrowthForm = speciesCharacterParameter(SP, SpParams, "GrowthForm");
  CharacterVector phenoType = speciesCharacterParameter(SP, SpParams, "PhenologyType");
  CharacterVector LeafSize = speciesCharacterParameter(SP, SpParams, "LeafSize");
  CharacterVector LeafShape = speciesCharacterParameter(SP, SpParams, "LeafShape");
  
  //Granier's parameters will not raise error messages when missing and will be filled always 
  if(SpParams.containsElementNamed("Tmax_LAI")) Tmax_LAI = speciesNumericParameter(SP, SpParams, "Tmax_LAI");
  NumericVector Tmax_LAIsq(SP.size(), NA_REAL);
  if(SpParams.containsElementNamed("Tmax_LAIsq")) Tmax_LAIsq = speciesNumericParameter(SP, SpParams, "Tmax_LAIsq");
  for(int c=0;c<SP.size();c++) {
    if(NumericVector::is_na(Tmax_LAI[c])) Tmax_LAI[c] = 0.134; //Granier coefficient for LAI
    if(NumericVector::is_na(Tmax_LAIsq[c])) Tmax_LAIsq[c] =-0.006; //Granier coefficient for LAI^2
  }
  
  if(fillMissingSpParams) {
    for(int c=0;c<SP.size();c++) {
      if(NumericVector::is_na(WUE[c]) && !CharacterVector::is_na(LeafShape[c]) && !CharacterVector::is_na(LeafSize[c])) {
        if(LeafShape[c]=="Linear") {
          WUE[c]= 3.707131;
        } else if(LeafShape[c]=="Needle") {
          WUE[c]= 3.707131;
        } else if(LeafShape[c]=="Broad") {
          if(LeafSize[c]=="Small") {
            WUE[c] = 4.289629;
          } else if(LeafSize[c]=="Medium") {
            WUE[c] = 3.982086;
          } else if(LeafSize[c]=="Large") {
            WUE[c]= 3.027647;
          }
        } else if(LeafShape[c]=="Scale") { 
          WUE[c] = 1.665034;
        }
      }
      if(NumericVector::is_na(pRootDisc[c])) pRootDisc[c] = 0.0;
      if(NumericVector::is_na(Psi_Critic[c])) {
        // From: Maherali H, Pockman W, Jackson R (2004) Adaptive variation in the vulnerability of woody plants to xylem cavitation. Ecology 85:2184–2199
        if(Group[c]=="Angiosperm") {
          if((GrowthForm[c]=="Shrub") && (phenoType[c] != "winter-deciduous") && (phenoType[c] != "winter-semideciduous")) {
            Psi_Critic[c] = -5.09; //Angiosperm evergreen shrub
          } else if((GrowthForm[c]!="Shrub") && (phenoType[c] == "winter-deciduous" | phenoType[c] == "winter-semideciduous")) {
            Psi_Critic[c] = -2.34; //Angiosperm winter-deciduous tree
          } else { 
            Psi_Critic[c] = -1.51; //Angiosperm evergreen tree
          }
        } else {
          if(GrowthForm[c]=="Shrub") {
            Psi_Critic[c] = -8.95; //Gymnosperm shrub
          } else {
            Psi_Critic[c] = -4.17; //Gymnosperm tree
          }
        }
      }
    }
  }
  DataFrame paramsTranspirationdf = DataFrame::create(_["Tmax_LAI"] = Tmax_LAI,
                                                      _["Tmax_LAIsq"] = Tmax_LAIsq,
                                                      _["Psi_Extract"]=Psi_Extract,
                                                      _["Psi_Critic"] = Psi_Critic,
                                                      _["WUE"] = WUE, _["pRootDisc"] = pRootDisc);
  paramsTranspirationdf.attr("row.names") = above.attr("row.names");
  return(paramsTranspirationdf);
}
DataFrame paramsTranspirationSperry(DataFrame above, List soil, DataFrame SpParams, 
                              DataFrame paramsAnatomydf, List control) {
  IntegerVector SP = above["SP"];
  NumericVector H = above["H"];
  int numCohorts = SP.size();
  double fracRootResistance = control["fracRootResistance"];
  double fracLeafResistance = control["fracLeafResistance"];
  String transpirationMode = control["transpirationMode"];
  
  bool fillMissingSpParams = control["fillMissingSpParams"];
  
  NumericVector dVec = soil["dVec"];
  
  CharacterVector Group = speciesCharacterParameter(SP, SpParams, "Group");
  CharacterVector Order = speciesCharacterParameter(SP, SpParams, "Order");
  CharacterVector GrowthForm = speciesCharacterParameter(SP, SpParams, "GrowthForm");
  CharacterVector phenoType = speciesCharacterParameter(SP, SpParams, "PhenologyType");
  
  NumericVector Hmed = speciesNumericParameter(SP, SpParams, "Hmed"); //To correct conductivity
  NumericVector Gswmin = speciesNumericParameter(SP, SpParams, "Gswmin");
  NumericVector Gswmax = speciesNumericParameter(SP, SpParams, "Gswmax");
  NumericVector VCleaf_kmax = speciesNumericParameter(SP, SpParams, "VCleaf_kmax");
  NumericVector Kmax_stemxylem = speciesNumericParameter(SP, SpParams, "Kmax_stemxylem");
  NumericVector VCleaf_c = speciesNumericParameter(SP, SpParams, "VCleaf_c");
  NumericVector VCleaf_d = speciesNumericParameter(SP, SpParams, "VCleaf_d");
  NumericVector VCstem_c = speciesNumericParameter(SP, SpParams, "VCstem_c");
  NumericVector VCstem_d = speciesNumericParameter(SP, SpParams, "VCstem_d");
  NumericVector Kmax_rootxylem = speciesNumericParameter(SP, SpParams, "Kmax_rootxylem");
  NumericVector VCroot_c = speciesNumericParameter(SP, SpParams, "VCroot_c");
  NumericVector VCroot_d = speciesNumericParameter(SP, SpParams, "VCroot_d");
  NumericVector Narea = speciesNumericParameter(SP, SpParams, "Narea");
  NumericVector Vmax298 = speciesNumericParameter(SP, SpParams, "Vmax298");
  NumericVector Jmax298 = speciesNumericParameter(SP, SpParams, "Jmax298");
  NumericVector pRootDisc = speciesNumericParameter(SP, SpParams, "pRootDisc");
  
  NumericVector Al2As = paramsAnatomydf["Al2As"];
  NumericVector SLA = paramsAnatomydf["SLA"];
  
  NumericVector VCstem_kmax(numCohorts);
  NumericVector VCroottot_kmax(numCohorts, 0.0);
  NumericVector VGrhizotot_kmax(numCohorts, 0.0);
  NumericVector Plant_kmax(numCohorts, 0.0);
  
  
  if(fillMissingSpParams) {
    for(int c=0;c<numCohorts;c++){
      if(NumericVector::is_na(pRootDisc[c])) pRootDisc[c] = 0.0;
      
      if(NumericVector::is_na(Kmax_stemxylem[c])) {
        // From: Maherali H, Pockman W, Jackson R (2004) Adaptive variation in the vulnerability of woody plants to xylem cavitation. Ecology 85:2184–2199
        if(Group[c]=="Angiosperm") {
          if((GrowthForm[c]=="Shrub") && (phenoType[c] == "winter-deciduous" | phenoType[c] == "winter-semideciduous")) {
            Kmax_stemxylem[c] = 1.55; //Angiosperm deciduous shrub
          } else if((GrowthForm[c]=="Tree" | GrowthForm[c]=="Tree/Shrub") && (phenoType[c] == "winter-deciduous" | phenoType[c] == "winter-semideciduous")) {
            Kmax_stemxylem[c] = 1.58; //Angiosperm winter-deciduous tree
          } else { 
            Kmax_stemxylem[c] = 2.43; //Angiosperm evergreen tree
          }
        } else {
          if(GrowthForm[c]=="Shrub") {
            Kmax_stemxylem[c] = 0.24; //Gymnosperm shrub
          } else {
            Kmax_stemxylem[c] = 0.48; //Gymnosperm tree
          }
        }
      }
      //Oliveras I, Martínez-Vilalta J, Jimenez-Ortiz T, et al (2003) Hydraulic architecture of Pinus halepensis, P . pinea and Tetraclinis articulata in a dune ecosystem of Eastern Spain. Plant Ecol 131–141
      if(NumericVector::is_na(Kmax_rootxylem[c])) Kmax_rootxylem[c] = 4.0*Kmax_stemxylem[c];
      
      
      //Xylem vulnerability curve
      if(NumericVector::is_na(VCstem_d[c]) | NumericVector::is_na(VCstem_c[c])) {
        double psi50 = NA_REAL;
        // From: Maherali H, Pockman W, Jackson R (2004) Adaptive variation in the vulnerability of woody plants to xylem cavitation. Ecology 85:2184–2199
        if(Group[c]=="Angiosperm") {
          if((GrowthForm[c]=="Shrub") && (phenoType[c] != "winter-deciduous") && (phenoType[c] != "winter-semideciduous")) {
            psi50 = -5.09; //Angiosperm evergreen shrub
          } else if((GrowthForm[c]!="Shrub") && (phenoType[c] == "winter-deciduous" | phenoType[c] == "winter-semideciduous")) {
            psi50 = -2.34; //Angiosperm winter-deciduous tree
          } else { 
            psi50 = -1.51; //Angiosperm evergreen tree
          }
        } else {
          if(GrowthForm[c]=="Shrub") {
            psi50 = -8.95; //Gymnosperm shrub
          } else {
            psi50 = -4.17; //Gymnosperm tree
          }
        }
        double psi88 = 1.2593*psi50 - 1.4264; //Regression using data from Choat et al. 2012
        NumericVector par = psi2Weibull(psi50, psi88);
        if(NumericVector::is_na(VCstem_c[c])) VCstem_c[c] = par["c"];
        if(NumericVector::is_na(VCstem_d[c])) VCstem_d[c] = par["d"];
      }
      
      //Default root vulnerability curve parameters if missing
      if(NumericVector::is_na(VCroot_d[c]) | NumericVector::is_na(VCroot_c[c])) {
        double psi50stem = VCstem_d[c]*pow(0.6931472,1.0/VCstem_c[c]);
        double psi50root = 0.742*psi50stem + 0.4892; //Regression using data from Bartlett et al. 2016
        double psi88root = 1.2593*psi50root - 1.4264; //Regression using data from Choat et al. 2012
        NumericVector par = psi2Weibull(psi50root, psi88root);
        if(NumericVector::is_na(VCroot_c[c])) VCroot_c[c] = par["c"];
        if(NumericVector::is_na(VCroot_d[c])) VCroot_d[c] = par["d"];
      }
      //Sack, L., & Holbrook, N.M. 2006. Leaf Hydraulics. Annual Review of Plant Biology 57: 361–381.
      if(NumericVector::is_na(VCleaf_kmax[c])) { 
        if(Group[c]=="Angiosperm") {
          VCleaf_kmax[c] = 8.0;
        } else {
          VCleaf_kmax[c] = 6.0;
        }
      } 
      //Default vulnerability curve parameters if missing
      if(NumericVector::is_na(VCleaf_c[c])) VCleaf_c[c] = VCstem_c[c];
      if(NumericVector::is_na(VCleaf_d[c])) VCleaf_d[c] = VCstem_d[c]/1.5;
      //Duursma RA, Blackman CJ, Lopéz R, et al (2018) On the minimum leaf conductance: its role in models of plant water use, and ecological and environmental controls. New Phytol. doi: 10.1111/nph.15395
      if(NumericVector::is_na(Gswmin[c])) {
        if(Order[c]=="Pinales") {
          Gswmin[c] = 0.003;
        } else if(Order[c]=="Araucariales") {
          Gswmin[c] = 0.003;
        } else if(Order[c]=="Ericales") {
          Gswmin[c] = 0.004;
        } else if(Order[c]=="Fagales") {
          Gswmin[c] = 0.0045;
        } else if(Order[c]=="Rosales") {
          Gswmin[c] = 0.0045;
        } else if(Order[c]=="Cupressales") {
          Gswmin[c] = 0.0045;
        } else if(Order[c]=="Lamiales") {
          Gswmin[c] = 0.0055;
        } else if(Order[c]=="Fabales") {
          Gswmin[c] = 0.0065;
        } else if(Order[c]=="Myrtales") {
          Gswmin[c] = 0.0075;
        } else if(Order[c]=="Poales") {
          Gswmin[c] = 0.0110;
        } else {
          Gswmin[c] = 0.0049;
        }
      }
      //Mencuccini M (2003) The ecological significance of long-distance water transport : short-term regulation , long-term acclimation and the hydraulic costs of stature across plant life forms. Plant Cell Environ 26:163–182
      if(NumericVector::is_na(Gswmax[c])) Gswmax[c] = 0.12115*pow(VCleaf_kmax[c], 0.633);
      

      if(NumericVector::is_na(Vmax298[c]))  {
        if(!NumericVector::is_na(SLA[c]) & !NumericVector::is_na(Narea[c]))  {
          //Walker AP, Beckerman AP, Gu L, et al (2014) The relationship of leaf photosynthetic traits - Vcmax and Jmax - to leaf nitrogen, leaf phosphorus, and specific leaf area: A meta-analysis and modeling study. Ecol Evol 4:3218–3235. doi: 10.1002/ece3.1173
          double lnN = log(Narea[c]);
          double lnSLA = log(SLA[c]/1000.0); //SLA in m2*g-1
          Vmax298[c] = exp(1.993 + 2.555*lnN - 0.372*lnSLA + 0.422*lnN*lnSLA);
        } else {
          Vmax298[c] = 100.0; //Default value of Vmax298 = 100.0
        }
      }
      //Walker AP, Beckerman AP, Gu L, et al (2014) The relationship of leaf photosynthetic traits - Vcmax and Jmax - to leaf nitrogen, leaf phosphorus, and specific leaf area: A meta-analysis and modeling study. Ecol Evol 4:3218–3235. doi: 10.1002/ece3.1173
      if(NumericVector::is_na(Jmax298[c])) Jmax298[c] = exp(1.197 + 0.847*log(Vmax298[c])); 
      
    }
  }

  // Scaled conductance parameters parameters
  for(int c=0;c<numCohorts;c++){
    //Stem maximum conductance (in mmol·m-2·s-1·MPa-1)
    VCstem_kmax[c]=maximumStemHydraulicConductance(Kmax_stemxylem[c], Hmed[c], Al2As[c],H[c],control["taper"]); 
  
    //Root maximum conductance
    double rstem = (1.0/VCstem_kmax[c]);
    double rleaf = (1.0/VCleaf_kmax[c]);
    double rtot = (rstem+rleaf)/(1.0 - fracRootResistance);
    double VCroot_kmaxc = 1.0/(rtot - rstem - rleaf);
    VCroottot_kmax[c] = VCroot_kmaxc;
    
    //Leaf maximum conductance
    if(!NumericVector::is_na(fracLeafResistance)) {
      double rstem = (1.0/VCstem_kmax[c]);
      double rtot = rstem/(1.0-fracRootResistance - fracLeafResistance);
      VCleaf_kmax[c] = 1.0/(rtot*fracLeafResistance);
    }
    //Plant kmax
    Plant_kmax[c] = 1.0/((1.0/VCleaf_kmax[c])+(1.0/VCstem_kmax[c])+(1.0/VCroottot_kmax[c]));
  }
  
  DataFrame paramsTranspirationdf = DataFrame::create(
    _["Gswmin"]=Gswmin, _["Gswmax"]=Gswmax,_["Vmax298"]=Vmax298,
      _["Jmax298"]=Jmax298, _["Kmax_stemxylem"] = Kmax_stemxylem, _["Kmax_rootxylem"] = Kmax_rootxylem,
        _["VCleaf_kmax"]=VCleaf_kmax,_["VCleaf_c"]=VCleaf_c,_["VCleaf_d"]=VCleaf_d,
        _["VCstem_kmax"]=VCstem_kmax,_["VCstem_c"]=VCstem_c,_["VCstem_d"]=VCstem_d, 
        _["VCroot_kmax"] = VCroottot_kmax ,_["VCroot_c"]=VCroot_c,_["VCroot_d"]=VCroot_d,
        _["VGrhizo_kmax"] = VGrhizotot_kmax,
        _["Plant_kmax"] = Plant_kmax);
  paramsTranspirationdf.attr("row.names") = above.attr("row.names");
  return(paramsTranspirationdf);
}

// [[Rcpp::export(".paramsBelow")]]
List paramsBelow(DataFrame above, NumericVector Z50, NumericVector Z95, List soil, 
                 DataFrame paramsAnatomydf, DataFrame paramsTranspirationdf, List control) {

  NumericVector dVec = soil["dVec"];
  // NumericVector bd = soil["bd"];
  NumericVector rfc = soil["rfc"];
  NumericVector VG_alpha = soil["VG_alpha"];
  NumericVector VG_n = soil["VG_n"];
  int nlayers = dVec.size();

  NumericVector LAI_live = above["LAI_live"];
  NumericVector N = above["N"];
  int numCohorts = N.size();
  
  
  bool plantWaterPools = control["plantWaterPools"];
  String transpirationMode = control["transpirationMode"];
  double averageFracRhizosphereResistance = control["averageFracRhizosphereResistance"];
  

  NumericMatrix V = ldrDistribution(Z50, Z95, dVec);
  V.attr("dimnames") = List::create(above.attr("row.names"), layerNames(nlayers));
  // CharacterVector slnames(V.ncol());
  // for(int i=0;i<V.ncol();i++) slnames[i] = i+1;
  
  NumericVector Wsoil = soil["W"];
  NumericMatrix Wpool = NumericMatrix(numCohorts, nlayers);
  Wpool.attr("dimnames") = V.attr("dimnames");
  
  NumericMatrix L = NumericMatrix(numCohorts, nlayers);
  L.attr("dimnames") = V.attr("dimnames");
  
  for(int c=0;c<numCohorts;c++){
    for(int l=0;l<nlayers;l++) Wpool(c,l) = Wsoil[l]; //Init from soil state
  }
  
  NumericVector poolProportions = LAI_live/sum(LAI_live);
  
  List belowLayers;
  DataFrame belowdf;
  
  if(transpirationMode == "Granier") {
    NumericVector CRSV(numCohorts);
    for(int c=0;c<numCohorts;c++){
      L(c,_) = coarseRootLengths(V(c,_), dVec, 0.5); //Arbitrary ratio (to revise some day)
      CRSV[c] = coarseRootSoilVolume(V(c,_), dVec, 0.5);
    }
    if(plantWaterPools) {
      belowdf = DataFrame::create(_["Z50"] = Z50,
                                  _["Z95"] = Z95,
                                  _["coarseRootSoilVolume"] = CRSV,
                                  _["poolProportions"] = poolProportions);
      List RHOP = horizontalProportions(poolProportions, CRSV, N, V, dVec, rfc);
      belowLayers = List::create(_["V"] = V,
                                 _["L"] = L,
                                 _["Wpool"] = Wpool,
                                 _["RHOP"] = RHOP);
    } else {
      belowdf = DataFrame::create(_["Z50"] = Z50,
                                  _["Z95"] = Z95,
                                  _["coarseRootSoilVolume"] = CRSV);
      belowLayers = List::create(_["V"] = V,
                                 _["L"] = L,
                                 _["Wpool"] = Wpool);
    }
  } else {
    NumericVector Al2As = paramsAnatomydf["Al2As"];
    NumericVector FineRootDensity = paramsAnatomydf["FineRootDensity"];
    NumericVector SRL = paramsAnatomydf["SRL"];
    NumericVector RLD = paramsAnatomydf["RLD"];
    
    NumericVector Kmax_stemxylem = paramsTranspirationdf["Kmax_stemxylem"];
    NumericVector VCroottot_kmax = paramsTranspirationdf["VCroot_kmax"];
    NumericVector VCroot_c = paramsTranspirationdf["VCroot_c"];
    NumericVector VCroot_d = paramsTranspirationdf["VCroot_d"];
    NumericVector VCstem_kmax = paramsTranspirationdf["VCstem_kmax"];
    NumericVector VCstem_c = paramsTranspirationdf["VCstem_c"];
    NumericVector VCstem_d = paramsTranspirationdf["VCstem_d"];
    NumericVector VCleaf_kmax = paramsTranspirationdf["VCleaf_kmax"];
    NumericVector VCleaf_c = paramsTranspirationdf["VCleaf_c"];
    NumericVector VCleaf_d = paramsTranspirationdf["VCleaf_d"];
    NumericVector VGrhizotot_kmax = paramsTranspirationdf["VGrhizo_kmax"];
    
    
    NumericMatrix RhizoPsi =  NumericMatrix(numCohorts, nlayers);
    RhizoPsi.attr("dimnames") = List::create(above.attr("row.names"), seq(1,nlayers));
    std::fill(RhizoPsi.begin(), RhizoPsi.end(), 0.0);

    
    NumericVector FRB(numCohorts), CRSV(numCohorts),FRAI(numCohorts);
    NumericVector Ksat = soil["Ksat"];
    for(int c=0;c<numCohorts;c++)  {
      //We use Kmax_stemxylem instead of Kmax_rootxylem because of reliability
      CRSV[c] = coarseRootSoilVolumeFromConductance(Kmax_stemxylem[c], VCroottot_kmax[c], Al2As[c],
                                                    V(c,_), dVec, rfc);
    }
    
    
    NumericMatrix VCroot_kmax(numCohorts, nlayers); 
    NumericMatrix VGrhizo_kmax(numCohorts, nlayers);
    VGrhizo_kmax.attr("dimnames") = V.attr("dimnames");
    VCroot_kmax.attr("dimnames") = V.attr("dimnames");
    NumericVector Vc;
    for(int c=0;c<numCohorts;c++){
      Vc = V(c,_);
      L(c,_) = coarseRootLengthsFromVolume(CRSV[c], V(c,_), dVec, rfc); 
      NumericVector xp = rootxylemConductanceProportions(L(c,_), V(c,_));
      for(int l=0;l<nlayers;l++) {
        VCroot_kmax(c,_) = VCroottot_kmax[c]*xp;
        VGrhizo_kmax(c,l) = V(c,l)*findRhizosphereMaximumConductance(averageFracRhizosphereResistance*100.0, VG_n[l], VG_alpha[l],
                     VCroottot_kmax[c], VCroot_c[c], VCroot_d[c],
                     VCstem_kmax[c], VCstem_c[c], VCstem_d[c],
                     VCleaf_kmax[c], VCleaf_c[c], VCleaf_d[c]);
        VGrhizotot_kmax[c] += VGrhizo_kmax(c,l); 
      }
      FRB[c] = fineRootBiomassPerIndividual(Ksat, VGrhizo_kmax(c,_), LAI_live[c], N[c], 
                                            SRL[c], FineRootDensity[c], RLD[c]);
    }
    belowLayers = List::create(_["V"] = V,
                               _["L"] = L,
                               _["VGrhizo_kmax"] = VGrhizo_kmax,
                               _["VCroot_kmax"] = VCroot_kmax,
                               _["Wpool"] = Wpool,
                               _["RhizoPsi"] = RhizoPsi);
    if(plantWaterPools) {
      belowdf = DataFrame::create(_["Z50"]=Z50,
                                  _["Z95"]=Z95,
                                  _["fineRootBiomass"] = FRB,
                                  _["coarseRootSoilVolume"] = CRSV,
                                  _["poolProportions"] = poolProportions);
      List RHOP = horizontalProportions(poolProportions, CRSV, N, V, dVec, rfc);
      belowLayers["RHOP"] = RHOP;
    } else {
      belowdf = DataFrame::create(_["Z50"]=Z50,
                                  _["Z95"]=Z95,
                                  _["fineRootBiomass"] = FRB,
                                  _["coarseRootSoilVolume"] = CRSV);
    }
  } 
  belowdf.attr("row.names") = above.attr("row.names");
  
  List below = List::create(_["below"] = belowdf,
                            _["belowLayers"] = belowLayers);
  return(below);
}

DataFrame paramsGrowth(DataFrame above, DataFrame SpParams, List control) {
  
  bool fillMissingSpParams = control["fillMissingSpParams"];
  
  IntegerVector SP = above["SP"];
  int numCohorts = SP.size();
  
  NumericVector RERleaf = speciesNumericParameter(SP, SpParams, "RERleaf");
  NumericVector RERsapwood = speciesNumericParameter(SP, SpParams, "RERsapwood");
  NumericVector RERfineroot = speciesNumericParameter(SP, SpParams, "RERfineroot");
  NumericVector RGRleafmax = speciesNumericParameter(SP, SpParams, "RGRleafmax");
  NumericVector RGRsapwoodmax = speciesNumericParameter(SP, SpParams, "RGRsapwoodmax");
  NumericVector RGRfinerootmax = speciesNumericParameter(SP, SpParams, "RGRfinerootmax");
  NumericVector WoodC = speciesNumericParameter(SP, SpParams, "WoodC");
  NumericVector fHDmin = speciesNumericParameter(SP, SpParams, "fHDmin");
  NumericVector fHDmax = speciesNumericParameter(SP, SpParams, "fHDmax");
  
  List maximumRelativeGrowthRates = control["maximumRelativeGrowthRates"];
  double RGRleafmax_default = maximumRelativeGrowthRates["leaf"];
  double RGRsapwoodmax_default = maximumRelativeGrowthRates["sapwood"];
  double RGRfinerootmax_default = maximumRelativeGrowthRates["fineroot"];
  
  List respirationRates = control["respirationRates"];
  double RERleaf_default = respirationRates["leaf"];
  double RERsapwood_default = respirationRates["sapwood"];
  double RERfineroot_default = respirationRates["fineroot"];
  
  if(fillMissingSpParams) {
    for(int c=0;c<numCohorts;c++){
      if(NumericVector::is_na(RGRleafmax[c])) RGRleafmax[c] = RGRleafmax_default;
      if(NumericVector::is_na(RGRsapwoodmax[c])) RGRsapwoodmax[c] = RGRsapwoodmax_default;
      if(NumericVector::is_na(RGRfinerootmax[c])) RGRfinerootmax[c] = RGRfinerootmax_default;
      if(NumericVector::is_na(RERleaf[c])) RERleaf[c] = RERleaf_default;
      if(NumericVector::is_na(RERsapwood[c])) RERsapwood[c] = RERsapwood_default;
      if(NumericVector::is_na(RERfineroot[c])) RERfineroot[c] = RERfineroot_default;
      
      if(NumericVector::is_na(WoodC[c])) WoodC[c] = 0.5;
    }
  }
  
  DataFrame paramsGrowthdf = DataFrame::create(_["RERleaf"] = RERleaf,
                                               _["RERsapwood"] = RERsapwood,
                                               _["RERfineroot"] = RERfineroot,
                                               _["RGRleafmax"] = RGRleafmax,
                                               _["RGRsapwoodmax"] = RGRsapwoodmax,
                                               _["RGRfinerootmax"] = RGRfinerootmax,
                                               _["fHDmin"] = fHDmin,
                                               _["fHDmax"] = fHDmax,
                                               _["WoodC"] = WoodC);
  paramsGrowthdf.attr("row.names") = above.attr("row.names");
  return(paramsGrowthdf);
}


DataFrame paramsAllometries(DataFrame above, DataFrame SpParams, bool fillMissingSpParams) {
  IntegerVector SP = above["SP"];
  
  NumericVector Aash = shrubAllometricCoefficientWithImputation(SP, SpParams, "a_ash");
  NumericVector Bash = shrubAllometricCoefficientWithImputation(SP, SpParams, "b_ash");
  NumericVector Absh = shrubAllometricCoefficientWithImputation(SP, SpParams, "a_bsh");
  NumericVector Bbsh = shrubAllometricCoefficientWithImputation(SP, SpParams, "b_bsh");
  NumericVector Acr = treeAllometricCoefficientWithImputation(SP, SpParams, "a_cr");
  NumericVector B1cr = treeAllometricCoefficientWithImputation(SP, SpParams, "b_1cr");
  NumericVector B2cr = treeAllometricCoefficientWithImputation(SP, SpParams, "b_2cr");
  NumericVector B3cr = treeAllometricCoefficientWithImputation(SP, SpParams, "b_3cr");
  NumericVector C1cr = treeAllometricCoefficientWithImputation(SP, SpParams, "c_1cr");
  NumericVector C2cr = treeAllometricCoefficientWithImputation(SP, SpParams, "c_2cr");
  NumericVector Acw = treeAllometricCoefficientWithImputation(SP, SpParams, "a_cw");
  NumericVector Bcw = treeAllometricCoefficientWithImputation(SP, SpParams, "b_cw");

  DataFrame paramsAllometriesdf = DataFrame::create(_["Aash"] = Aash, _["Bash"] = Bash, _["Absh"] = Absh, _["Bbsh"] = Bbsh,
                                                    _["Acr"] = Acr, _["B1cr"] = B1cr, _["B2cr"] = B2cr, _["B3cr"] = B3cr,
                                                    _["C1cr"] = C1cr, _["C2cr"] = C2cr, 
                                                    _["Acw"] = Acw, _["Bcw"] = Bcw);
  paramsAllometriesdf.attr("row.names") = above.attr("row.names");
  return(paramsAllometriesdf);
}

DataFrame internalPhenologyDataFrame(DataFrame above) {
  int numCohorts = above.nrow();
  NumericVector phi(numCohorts,0.0);
  NumericVector gdd(numCohorts,0.0);
  NumericVector sen(numCohorts,0.0);
  LogicalVector budFormation(numCohorts, false);
  LogicalVector leafUnfolding(numCohorts, false);
  LogicalVector leafSenescence(numCohorts, false);
  LogicalVector leafDormancy(numCohorts, false);
  
  DataFrame df = DataFrame::create(Named("gdd") = gdd,
                                   Named("sen") = sen,
                                   Named("budFormation") = budFormation,
                                   Named("leafUnfolding") = leafUnfolding,
                                   Named("leafSenescence") = leafSenescence,
                                   Named("leafDormancy") = leafDormancy,
                                   Named("phi") = phi);
  df.attr("row.names") = above.attr("row.names");
  return(df);
}
DataFrame internalCarbonDataFrame(DataFrame above, 
                                  DataFrame belowdf,
                                  List belowLayers,
                                  DataFrame paramsAnatomydf,
                                  DataFrame paramsWaterStoragedf,
                                  DataFrame paramsGrowthdf,
                                  List control) {
  int numCohorts = above.nrow();

  double nonSugarConcentration = control["nonSugarConcentration"];
  String transpirationMode = control["transpirationMode"];
  
  NumericVector WoodDensity = paramsAnatomydf["WoodDensity"];
  NumericVector LeafDensity = paramsAnatomydf["LeafDensity"];
  NumericVector SLA = paramsAnatomydf["SLA"];
  NumericVector conduit2sapwood = paramsAnatomydf["conduit2sapwood"];
  NumericVector LeafPI0 = paramsWaterStoragedf["LeafPI0"];
  NumericVector StemPI0 = paramsWaterStoragedf["StemPI0"];
  
  NumericVector WoodC = paramsGrowthdf["WoodC"];

  NumericMatrix V = belowLayers["V"];
  NumericMatrix L = belowLayers["L"];
  
  NumericVector Z95 = belowdf["Z95"];

  IntegerVector SP = above["SP"];
  NumericVector LAI_live = above["LAI_live"];
  NumericVector LAI_expanded = above["LAI_expanded"];
  NumericVector LAI_dead = above["LAI_dead"];
  NumericVector N = above["N"];
  NumericVector DBH = above["DBH"];
  NumericVector Cover = above["Cover"];
  NumericVector H = above["H"];
  NumericVector CR = above["CR"];
  NumericVector SA = above["SA"];

  // NumericVector sugar(numCohorts,0.0);
  // NumericVector starch(numCohorts,0.0);
  NumericVector sugarLeaf(numCohorts,0.0);
  NumericVector starchLeaf(numCohorts,0.0);
  NumericVector sugarSapwood(numCohorts,0.0);
  NumericVector starchSapwood(numCohorts,0.0);
  // NumericVector longtermStorage(numCohorts,0.0);
  for(int c=0;c<numCohorts;c++){
    double lvol = leafStorageVolume(LAI_expanded[c],  N[c], SLA[c], LeafDensity[c]);
    double svol = sapwoodStorageVolume(SA[c], H[c], L(c,_), V(c,_),WoodDensity[c], conduit2sapwood[c]);
    
    // 50% in starch storage
    if(LAI_expanded[c]>0.0) starchLeaf[c] = (0.5/(lvol*glucoseMolarMass))*leafStarchCapacity(LAI_expanded[c], N[c], SLA[c], LeafDensity[c]);
    starchSapwood[c] = (0.5/(svol*glucoseMolarMass))*sapwoodStarchCapacity(SA[c], H[c], L, V(c,_), WoodDensity[c], conduit2sapwood[c]);
    // starch[c] = starchLeaf[c]+starchSapwood[c];
    
    //Sugar storage from PI0
    if(LAI_expanded[c]>0.0) {
      double lconc = sugarConcentration(LeafPI0[c],20.0, nonSugarConcentration);
      sugarLeaf[c] = lconc;
    }
    double sconc = sugarConcentration(StemPI0[c],20.0, nonSugarConcentration);
    sugarSapwood[c] = sconc;
    // sugar[c] = sugarLeaf[c] + sugarSapwood[c];
  }
  DataFrame df;
  // if(transpirationMode=="Granier"){
  //   df = DataFrame::create(Named("sugar") = sugar,
  //                          Named("starch") = starch);
  // } else {
    df = DataFrame::create(Named("sugarLeaf") = sugarLeaf,
                           Named("starchLeaf") = starchLeaf,
                           Named("sugarSapwood") = sugarSapwood,
                           Named("starchSapwood") = starchSapwood);
  // }
  df.attr("row.names") = above.attr("row.names");
  return(df);
}  

DataFrame internalMortalityDataFrame(DataFrame above) {
  int numCohorts = above.nrow();
  NumericVector N_dead(numCohorts, 0.0);
  NumericVector Cover_dead(numCohorts, 0.0);
  DataFrame df = DataFrame::create(Named("N_dead") = N_dead,
                                   Named("Cover_dead") = Cover_dead);
  df.attr("row.names") = above.attr("row.names");
  return(df);
}
DataFrame internalAllocationDataFrame(DataFrame above, 
                                      DataFrame belowdf, 
                                      DataFrame paramsAnatomydf,
                                      DataFrame paramsTranspirationdf,
                                      List control) {
  int numCohorts = above.nrow();

  NumericVector allocationTarget(numCohorts,0.0);
  NumericVector leafAreaTarget(numCohorts,0.0);
  NumericVector fineRootBiomassTarget(numCohorts, 0.0);
  
  String transpirationMode = control["transpirationMode"];
  NumericVector SA = above["SA"];
  NumericVector Al2As = paramsAnatomydf["Al2As"];
  DataFrame df;
  if(transpirationMode=="Granier") {
    for(int c=0;c<numCohorts;c++){
      leafAreaTarget[c] = Al2As[c]*(SA[c]/10000.0);
      allocationTarget[c] = Al2As[c];
    }
    df = DataFrame::create(Named("allocationTarget") = allocationTarget,
                           Named("leafAreaTarget") = leafAreaTarget);
  } else {
    String allocationStrategy = control["allocationStrategy"];
    NumericVector Plant_kmax = paramsTranspirationdf["Plant_kmax"];
    NumericVector VGrhizo_kmax = paramsTranspirationdf["VGrhizo_kmax"];
    NumericVector fineRootBiomass = belowdf["fineRootBiomass"];
    // NumericVector longtermStorage(numCohorts,0.0);
    for(int c=0;c<numCohorts;c++){
      leafAreaTarget[c] = Al2As[c]*(SA[c]/10000.0);
      if(allocationStrategy=="Plant_kmax") {
        allocationTarget[c] = Plant_kmax[c];
      } else if(allocationStrategy=="Al2As") {
        allocationTarget[c] = Al2As[c];
      }
      fineRootBiomassTarget[c] = fineRootBiomass[c];
    }
    
    df = DataFrame::create(Named("allocationTarget") = allocationTarget,
                           Named("leafAreaTarget") = leafAreaTarget,
                           Named("fineRootBiomassTarget") = fineRootBiomassTarget);
  }
  df.attr("row.names") = above.attr("row.names");
  return(df);
}  


DataFrame internalWaterDataFrame(DataFrame above, String transpirationMode) {
  int numCohorts = above.nrow();
  DataFrame df;
  if(transpirationMode=="Granier") {
    df = DataFrame::create(Named("PlantPsi") = NumericVector(numCohorts, 0.0),
                           Named("StemPLC") = NumericVector(numCohorts, 0.0));
  } else {
    df = DataFrame::create(Named("Einst") = NumericVector(numCohorts, 0.0),
                           Named("RootCrownPsi") = NumericVector(numCohorts, 0.0),
                           Named("Stem1Psi") = NumericVector(numCohorts, 0.0),
                           Named("Stem2Psi") = NumericVector(numCohorts, 0.0),
                           Named("LeafPsi") = NumericVector(numCohorts, 0.0),
                           Named("StemSympPsi") = NumericVector(numCohorts, 0.0),
                           Named("LeafSympPsi") = NumericVector(numCohorts, 0.0),
                           Named("StemPLC") = NumericVector(numCohorts, 0.0),
                           Named("NSPL") = NumericVector(numCohorts, 1.0));
  }
  df.attr("row.names") = above.attr("row.names");
  return(df);
}

DataFrame paramsCanopy(DataFrame above, List control) {
  NumericVector LAI_live = above["LAI_live"];
  NumericVector LAI_expanded = above["LAI_expanded"];
  NumericVector LAI_dead = above["LAI_dead"];
  NumericVector H = above["H"];
  int numCohorts = H.size();
  //Determine number of vertical layers
  double verticalLayerSize = control["verticalLayerSize"];
  double boundaryLayerSize = control["boundaryLayerSize"];
  double canopyHeight = 0.0;
  for(int c=0;c<numCohorts;c++) {
    if((canopyHeight<H[c]) & ((LAI_live[c]+LAI_dead[c])>0.0)) canopyHeight = H[c];
  }
  int nz = ceil((canopyHeight+boundaryLayerSize)/verticalLayerSize); //Number of vertical layers (adding 2 m above to match wind measurement height)
  NumericVector zlow(nz,0.0);
  NumericVector zmid(nz, verticalLayerSize/2.0);
  NumericVector zup(nz, verticalLayerSize);
  for(int i=1;i<nz;i++) {
    zlow[i] = zlow[i-1] + verticalLayerSize;
    zmid[i] = zmid[i-1] + verticalLayerSize;
    zup[i] = zup[i-1] + verticalLayerSize;
  }
  DataFrame paramsCanopy = DataFrame::create(_["zlow"] = zlow,
                                             _["zmid"] = zmid,
                                             _["zup"] = zup,
                                             _["Tair"] = NumericVector(nz, NA_REAL),
                                             _["Cair"] = NumericVector(nz, control["Catm"]),
                                             _["VPair"] = NumericVector(nz, NA_REAL));
  return(paramsCanopy);
}
/**
 *  Prepare Soil Water Balance input
 */
// [[Rcpp::export("spwbInput")]]
List spwbInput(DataFrame above, NumericVector Z50, NumericVector Z95, List soil, DataFrame SpParams, List control) {
  
  
  IntegerVector SP = above["SP"];
  NumericVector LAI_live = above["LAI_live"];
  NumericVector LAI_expanded = above["LAI_expanded"];
  NumericVector LAI_dead = above["LAI_dead"];
  NumericVector H = above["H"];
  NumericVector DBH = above["DBH"];
  NumericVector N = above["N"];
  NumericVector CR = above["CR"];
  
  String transpirationMode = control["transpirationMode"];
  if((transpirationMode!="Granier") & (transpirationMode!="Sperry")) stop("Wrong Transpiration mode ('transpirationMode' should be either 'Granier' or 'Sperry')");

  bool fillMissingSpParams = control["fillMissingSpParams"];
  String soilFunctions = control["soilFunctions"]; 
  if((soilFunctions!="SX") & (soilFunctions!="VG")) stop("Wrong soil functions ('soilFunctions' should be either 'SX' or 'VG')");
  
  int numCohorts = SP.size();
  for(int c=0;c<numCohorts;c++){
    if(NumericVector::is_na(CR[c])) CR[c] = 0.5; //PATCH TO AVOID MISSING VALUES!!!!
  }
  

  //Cohort description
  CharacterVector nsp = speciesCharacterParameter(SP, SpParams, "Name");
  DataFrame cohortDescdf = DataFrame::create(_["SP"] = SP, _["Name"] = nsp);
  cohortDescdf.attr("row.names") = above.attr("row.names");
  
  //Above 
  DataFrame plantsdf = DataFrame::create(_["H"]=H, _["CR"]=CR, _["N"] = N,
                                         _["LAI_live"]=LAI_live, 
                                         _["LAI_expanded"] = LAI_expanded, 
                                         _["LAI_dead"] = LAI_dead);
  plantsdf.attr("row.names") = above.attr("row.names");
  
  DataFrame paramsAnatomydf;
  DataFrame paramsTranspirationdf;
  if(transpirationMode=="Granier") {
    paramsAnatomydf = DataFrame::create();
    paramsTranspirationdf = paramsTranspirationGranier(above,SpParams, fillMissingSpParams);
  } else {
    paramsAnatomydf = paramsAnatomy(above, SpParams, fillMissingSpParams);
    paramsTranspirationdf = paramsTranspirationSperry(above, soil, SpParams, paramsAnatomydf, control);
  }

  List below = paramsBelow(above, Z50, Z95, soil, 
                           paramsAnatomydf, paramsTranspirationdf, control);
  List belowLayers = below["belowLayers"];
  DataFrame belowdf = Rcpp::as<Rcpp::DataFrame>(below["below"]);

  List input;
  if(transpirationMode=="Granier") {
    input = List::create(_["control"] = clone(control),
                         _["soil"] = clone(soil),
                         _["canopy"] = List::create(),
                         _["cohorts"] = cohortDescdf,
                         _["above"] = plantsdf,
                         _["below"] = belowdf,
                         _["belowLayers"] = belowLayers,
                         _["paramsPhenology"] = paramsPhenology(above, SpParams, fillMissingSpParams),
                         _["paramsInterception"] = paramsInterception(above, SpParams, control),
                         _["paramsTranspiration"] = paramsTranspirationdf,
                         _["internalPhenology"] = internalPhenologyDataFrame(above),
                         _["internalWater"] = internalWaterDataFrame(above, transpirationMode));
  } else if(transpirationMode =="Sperry"){
    
    DataFrame paramsWaterStoragedf = paramsWaterStorage(above, SpParams, paramsAnatomydf, fillMissingSpParams);

    List ctl = clone(control);
    if(soilFunctions=="SX") {
      soilFunctions = "VG"; 
      ctl["soilFunctions"] = soilFunctions;
      Rcerr<<"Soil pedotransfer functions set to Van Genuchten ('VG').\n";
    }

    input = List::create(_["control"] = ctl,
                         _["soil"] = clone(soil),
                         _["canopy"] = paramsCanopy(above, control),
                         _["cohorts"] = cohortDescdf,
                         _["above"] = plantsdf,
                         _["below"] = belowdf,
                         _["belowLayers"] = belowLayers,
                         _["paramsPhenology"] = paramsPhenology(above, SpParams, fillMissingSpParams),
                         _["paramsAnatomy"] = paramsAnatomydf,
                         _["paramsInterception"] = paramsInterception(above, SpParams, control),
                         _["paramsTranspiration"] = paramsTranspirationdf,
                         _["paramsWaterStorage"] = paramsWaterStoragedf,
                         _["internalPhenology"] = internalPhenologyDataFrame(above),
                         _["internalWater"] = internalWaterDataFrame(above, transpirationMode));
  }

  input.attr("class") = CharacterVector::create("spwbInput","list");
  return(input);
}



/**
 *  Prepare Forest growth input
 */
// [[Rcpp::export("growthInput")]]
List growthInput(DataFrame above, NumericVector Z50, NumericVector Z95, List soil, DataFrame SpParams, List control) {

  IntegerVector SP = above["SP"];
  NumericVector LAI_live = above["LAI_live"];
  NumericVector LAI_expanded = above["LAI_expanded"];
  NumericVector LAI_dead = above["LAI_dead"];
  NumericVector N = above["N"];
  NumericVector DBH = above["DBH"];
  NumericVector Cover = above["Cover"];
  NumericVector H = above["H"];
  NumericVector CR = above["CR"];
  
  control["cavitationRefill"] = "growth";
  
  String transpirationMode = control["transpirationMode"];
  if((transpirationMode!="Granier") & (transpirationMode!="Sperry")) stop("Wrong Transpiration mode ('transpirationMode' should be either 'Granier' or 'Sperry')");

  bool fillMissingSpParams = control["fillMissingSpParams"];
  
  String soilFunctions = control["soilFunctions"]; 
  if((soilFunctions!="SX") & (soilFunctions!="VG")) stop("Wrong soil functions ('soilFunctions' should be either 'SX' or 'VG')");
  
  
  DataFrame paramsAnatomydf = paramsAnatomy(above, SpParams, fillMissingSpParams);
  NumericVector WoodDensity = paramsAnatomydf["WoodDensity"];
  NumericVector SLA = paramsAnatomydf["SLA"];
  NumericVector Al2As = paramsAnatomydf["Al2As"];
  
  DataFrame paramsGrowthdf = paramsGrowth(above, SpParams, control);

  DataFrame paramsWaterStoragedf = paramsWaterStorage(above, SpParams, paramsAnatomydf, fillMissingSpParams);
  
  DataFrame paramsAllometriesdf = paramsAllometries(above, SpParams, fillMissingSpParams);
  
  
  DataFrame paramsTranspirationdf;
  if(transpirationMode=="Granier") {
    paramsTranspirationdf = paramsTranspirationGranier(above,SpParams, fillMissingSpParams);
  } else {
    paramsTranspirationdf = paramsTranspirationSperry(above, soil, SpParams, paramsAnatomydf, control);
  }


  int numCohorts = SP.size();
  for(int c=0;c<numCohorts;c++){
    if(NumericVector::is_na(CR[c])) CR[c] = 0.5; //PATCH TO AVOID MISSING VALUES!!!!
  }
  
  
  NumericVector SA(numCohorts);
  for(int c=0;c<numCohorts;c++){
    SA[c] = 10000.0*(LAI_live[c]/(N[c]/10000.0))/Al2As[c];//Individual SA in cm2
  }
  

  
  //Cohort description
  CharacterVector nsp = speciesCharacterParameter(SP, SpParams, "Name");
  DataFrame cohortDescdf = DataFrame::create(_["SP"] = SP, _["Name"] = nsp);
  cohortDescdf.attr("row.names") = above.attr("row.names");
  
  DataFrame plantsdf = DataFrame::create(_["SP"]=SP, 
                                         _["N"]=N,
                                         _["DBH"]=DBH, 
                                         _["Cover"] = Cover, 
                                         _["H"]=H, 
                                         _["CR"]=CR,
                                         _["SA"] = SA, 
                                         _["LAI_live"]=LAI_live, 
                                         _["LAI_expanded"]=LAI_expanded, 
                                         _["LAI_dead"] = LAI_dead);
  plantsdf.attr("row.names") = above.attr("row.names");
  

  List ringList(numCohorts);
  for(int i=0;i<numCohorts;i++) ringList[i] = initialize_ring();
  ringList.attr("names") = above.attr("row.names");
  
  List below = paramsBelow(above, Z50, Z95, soil, 
                           paramsAnatomydf, paramsTranspirationdf, control);
  List belowLayers = below["belowLayers"];
  DataFrame belowdf = Rcpp::as<Rcpp::DataFrame>(below["below"]);
  
  
  List input;
  if(transpirationMode=="Granier") {
    input = List::create(_["control"] = clone(control),
                         _["soil"] = clone(soil),
                         _["canopy"] = List::create(),
                         _["cohorts"] = cohortDescdf,
                         _["above"] = plantsdf,
                         _["below"] = belowdf,
                         _["belowLayers"] = belowLayers,
                         _["paramsPhenology"] = paramsPhenology(above, SpParams, fillMissingSpParams),
                         _["paramsAnatomy"] = paramsAnatomydf,
                         _["paramsInterception"] = paramsInterception(above, SpParams, control),
                         _["paramsTranspiration"] = paramsTranspirationdf,
                         _["paramsWaterStorage"] = paramsWaterStoragedf,
                         _["paramsGrowth"]= paramsGrowthdf,
                         _["paramsAllometries"] = paramsAllometriesdf,
                         _["internalPhenology"] = internalPhenologyDataFrame(above),
                         _["internalWater"] = internalWaterDataFrame(above, transpirationMode),
                         _["internalCarbon"] = internalCarbonDataFrame(plantsdf, belowdf, belowLayers,
                                                         paramsAnatomydf, 
                                                         paramsWaterStoragedf,
                                                         paramsGrowthdf, control),
                        _["internalAllocation"] = internalAllocationDataFrame(plantsdf, belowdf,
                                                            paramsAnatomydf,
                                                            paramsTranspirationdf, control),
                        _["internalMortality"] = internalMortalityDataFrame(plantsdf),
                        _["internalRings"] = ringList);
  } else if(transpirationMode =="Sperry"){
    if(soilFunctions=="SX") {
      soilFunctions = "VG"; 
      warning("Soil pedotransfer functions set to Van Genuchten ('VG').");
    }

    input = List::create(_["control"] = clone(control),
                         _["soil"] = clone(soil),
                         _["canopy"] = paramsCanopy(above, control),
                         _["cohorts"] = cohortDescdf,
                         _["above"] = plantsdf,
                         _["below"] = belowdf,
                         _["belowLayers"] = belowLayers,
                         _["paramsPhenology"] = paramsPhenology(above, SpParams, fillMissingSpParams),
                         _["paramsAnatomy"] = paramsAnatomydf,
                         _["paramsInterception"] = paramsInterception(above, SpParams, control),
                         _["paramsTranspiration"] = paramsTranspirationdf,
                         _["paramsWaterStorage"] = paramsWaterStoragedf,
                         _["paramsGrowth"]= paramsGrowthdf,
                         _["paramsAllometries"] = paramsAllometriesdf,
                         _["internalPhenology"] = internalPhenologyDataFrame(above),
                         _["internalWater"] = internalWaterDataFrame(above, transpirationMode),
                         _["internalCarbon"] = internalCarbonDataFrame(plantsdf, belowdf, belowLayers,
                                                         paramsAnatomydf, 
                                                         paramsWaterStoragedf,
                                                         paramsGrowthdf, control),
                         _["internalAllocation"] = internalAllocationDataFrame(plantsdf, belowdf,
                                                         paramsAnatomydf,
                                                         paramsTranspirationdf, control),
                         _["internalMortality"] = internalMortalityDataFrame(plantsdf),
                         _["internalRings"] = ringList);
    
  } 
  
  input.attr("class") = CharacterVector::create("growthInput","list");
  return(input);
}

// [[Rcpp::export(".cloneInput")]]
List cloneInput(List input) {
  return(clone(input));
}

// [[Rcpp::export("forest2spwbInput")]]
List forest2spwbInput(List x, List soil, DataFrame SpParams, List control, String mode = "MED") {
  DataFrame treeData = Rcpp::as<Rcpp::DataFrame>(x["treeData"]);
  DataFrame shrubData = Rcpp::as<Rcpp::DataFrame>(x["shrubData"]);
  int ntree = treeData.nrows();
  int nshrub = shrubData.nrows();
  NumericVector treeZ95 = treeData["Z95"];
  NumericVector shrubZ95 = shrubData["Z95"];  
  NumericVector treeZ50 = treeData["Z50"];
  NumericVector shrubZ50 = shrubData["Z50"];  
  NumericVector Z95(ntree+nshrub), Z50(ntree+nshrub);
  for(int i=0;i<ntree;i++) {
    Z95[i] = treeZ95[i];
    Z50[i] = treeZ50[i];
  }
  for(int i=0;i<nshrub;i++) {
    Z95[ntree+i] = shrubZ95[i]; 
    Z50[ntree+i] = shrubZ50[i]; 
  }
  DataFrame above = forest2aboveground(x, SpParams, NA_REAL, mode);
  return(spwbInput(above, Z50, Z95, soil, SpParams, control));
}


// [[Rcpp::export("forest2growthInput")]]
List forest2growthInput(List x, List soil, DataFrame SpParams, List control) {
  DataFrame treeData = Rcpp::as<Rcpp::DataFrame>(x["treeData"]);
  DataFrame shrubData = Rcpp::as<Rcpp::DataFrame>(x["shrubData"]);
  int ntree = treeData.nrows();
  int nshrub = shrubData.nrows();
  NumericVector treeZ95 = treeData["Z95"];
  NumericVector shrubZ95 = shrubData["Z95"];  
  NumericVector treeZ50 = treeData["Z50"];
  NumericVector shrubZ50 = shrubData["Z50"];  
  NumericVector Z95(ntree+nshrub), Z50(ntree+nshrub);
  for(int i=0;i<ntree;i++) {
    Z95[i] = treeZ95[i];
    Z50[i] = treeZ50[i];
  }
  for(int i=0;i<nshrub;i++) {
    Z95[ntree+i] = shrubZ95[i]; 
    Z50[ntree+i] = shrubZ50[i]; 
  }
  DataFrame above = forest2aboveground(x, SpParams, NA_REAL);
  return(growthInput(above,  Z50, Z95, soil, SpParams, control));
}

// [[Rcpp::export("resetInputs")]]
void resetInputs(List x) {
  List control = x["control"];
  List soil = x["soil"];
  String transpirationMode = control["transpirationMode"];
  //Reset of canopy layer state variables 
  if(transpirationMode=="Sperry") {
    DataFrame can = Rcpp::as<Rcpp::DataFrame>(x["canopy"]);
    NumericVector Tair = can["Tair"];
    NumericVector Cair = can["Cair"];
    NumericVector VPair = can["VPair"];
    int ncanlayers = can.nrow();
    for(int i=0;i<ncanlayers;i++) {
      Tair[i] = NA_REAL;
      Cair[i] = control["Catm"];
      VPair[i] = NA_REAL;
    }
  }
  //Reset of soil state variables
  NumericVector Wsoil = soil["W"];
  NumericVector Temp = soil["Temp"];
  List belowLayers = x["belowLayers"];
  NumericMatrix Wpool = belowLayers["Wpool"];
  int nlayers = Wsoil.size();
  for(int i=0;i<nlayers;i++) {
    Wsoil[i] = 1.0; //Defaults to soil at field capacity
    Temp[i] = NA_REAL;
  }
  //Reset of cohort-based state variables
  int numCohorts = Wpool.nrow();
  for(int c=0;c<numCohorts;c++) {
    for(int l=0;l<nlayers;l++) {
      Wpool(c,l) = 1.0;
    }
  }
  DataFrame internalWater = Rcpp::as<Rcpp::DataFrame>(x["internalWater"]);
  NumericVector StemPLC = Rcpp::as<Rcpp::NumericVector>(internalWater["StemPLC"]);
  
  if(transpirationMode=="Sperry") {
    NumericMatrix RhizoPsi = Rcpp::as<Rcpp::NumericMatrix>(belowLayers["RhizoPsi"]);
    NumericVector RootCrownPsi = Rcpp::as<Rcpp::NumericVector>(internalWater["RootCrownPsi"]);
    NumericVector Stem1Psi = Rcpp::as<Rcpp::NumericVector>(internalWater["Stem1Psi"]);
    NumericVector Stem2Psi = Rcpp::as<Rcpp::NumericVector>(internalWater["Stem2Psi"]);
    NumericVector StemSympPsi = Rcpp::as<Rcpp::NumericVector>(internalWater["StemSympPsi"]);
    NumericVector LeafSympPsi = Rcpp::as<Rcpp::NumericVector>(internalWater["LeafSympPsi"]);
    NumericVector LeafPsi = Rcpp::as<Rcpp::NumericVector>(internalWater["LeafPsi"]);
    NumericVector Einst = Rcpp::as<Rcpp::NumericVector>(internalWater["Einst"]);
    NumericVector NSPL = Rcpp::as<Rcpp::NumericVector>(internalWater["NSPL"]);
    for(int i=0;i<LeafPsi.size();i++) {
      Einst[i] = 0.0;
      RootCrownPsi[i] = 0.0;
      Stem1Psi[i] = 0.0;
      Stem2Psi[i] = 0.0;
      LeafPsi[i] = 0.0;
      LeafSympPsi[i] = 0.0;
      StemSympPsi[i] = 0.0;
      StemPLC[i] = 0.0;
      NSPL[i] = 1.0;
      for(int j=0;j<RhizoPsi.ncol();j++) RhizoPsi(i,j) = 0.0;
    }
  } else {
    NumericVector PlantPsi = Rcpp::as<Rcpp::NumericVector>(internalWater["PlantPsi"]);
    for(int i=0;i<StemPLC.length();i++) {
      PlantPsi[i] = 0.0;
      StemPLC[i] = 0.0;
    }
  }
}

void updatePlantKmax(List x) {
  List control = x["control"];
  String transpirationMode = control["transpirationMode"];
  
  if(transpirationMode=="Sperry") {
    DataFrame paramsTranspirationdf =  Rcpp::as<Rcpp::DataFrame>(x["paramsTranspiration"]);
    NumericVector Plant_kmax = paramsTranspirationdf["Plant_kmax"];
    NumericVector VCleaf_kmax = paramsTranspirationdf["VCleaf_kmax"];
    NumericVector VCstem_kmax = paramsTranspirationdf["VCstem_kmax"];
    NumericVector VCroot_kmax = paramsTranspirationdf["VCroot_kmax"];
    int numCohorts = Plant_kmax.size();
    for(int i=0;i<numCohorts;i++) {
      Plant_kmax[i] = 1.0/((1.0/VCleaf_kmax[i])+(1.0/VCstem_kmax[i])+(1.0/VCroot_kmax[i]));
    }
  }
}
void updateBelowgroundConductances(List x) {
  List soil = x["soil"];
  NumericVector dVec = soil["dVec"];
  NumericVector rfc = soil["dVec"];
  List belowLayers = x["belowLayers"];
  NumericMatrix V = belowLayers["V"];
  NumericMatrix L = belowLayers["L"];
  int numCohorts = V.nrow();
  int nlayers = V.ncol();
  List control = x["control"];
  String transpirationMode = control["transpirationMode"];
  if(transpirationMode=="Sperry") {
    DataFrame paramsTranspirationdf = Rcpp::as<Rcpp::DataFrame>(x["paramsTranspiration"]);
    NumericVector VCroot_kmax = belowLayers["VCroot_kmax"];
    NumericVector VGrhizo_kmax = belowLayers["VGrhizo_kmax"];
    NumericVector VCroottot_kmax = paramsTranspirationdf["VCroot_kmax"];
    NumericVector VGrhizotot_kmax = paramsTranspirationdf["VGrhizo_kmax"];
    for(int c=0;c<numCohorts;c++) {
      NumericVector xp = rootxylemConductanceProportions(L(c,_), V(c,_));
      for(int l=0;l<nlayers;l++)  {
        VCroot_kmax(c,l) = VCroottot_kmax[c]*xp[l]; 
        VGrhizo_kmax(c,l) = VGrhizo_kmax[c]*V(c,l);
      }
    }
  }
}
void updateFineRootDistribution(List x) {
  List soil = x["soil"];
  NumericVector dVec = soil["dVec"];
  DataFrame belowdf =  Rcpp::as<Rcpp::DataFrame>(x["below"]);
  NumericVector Z50 = belowdf["Z50"];
  NumericVector Z95 = belowdf["Z95"];
  List belowLayers = x["belowLayers"];
  NumericMatrix V = belowLayers["V"];
  int numCohorts = V.nrow();
  int nlayers = V.ncol();
  for(int c=0;c<numCohorts;c++) {
    NumericVector PC = ldrRS_one(Z50[c], Z95[c], dVec);
    for(int l=0;l<nlayers;l++) V(c,l) = PC[l]; 
  }
  updateBelowgroundConductances(x);
}
// [[Rcpp::export(".updateBelow")]]
void updateBelow(List x) {
  List control = x["control"];
  List soil = x["soil"];

  DataFrame above = Rcpp::as<Rcpp::DataFrame>(x["above"]);
  DataFrame belowdf = Rcpp::as<Rcpp::DataFrame>(x["below"]);
  
  DataFrame paramsAnatomydf = DataFrame::create();
  if(x.containsElementNamed("paramsAnatomy")) paramsAnatomydf = Rcpp::as<Rcpp::DataFrame>(x["paramsAnatomy"]);
  DataFrame paramsTranspirationdf = Rcpp::as<Rcpp::DataFrame>(x["paramsTranspiration"]);
  NumericVector Z50 = belowdf["Z50"];
  NumericVector Z95 = belowdf["Z95"];
  List newBelowList = paramsBelow(above, Z50, Z95, soil, 
                               paramsAnatomydf, paramsTranspirationdf, control);
  x["below"] = newBelowList["below"];
  x["belowLayers"] = newBelowList["belowLayers"];
}

double getInputParamValue(List x, String paramType, String paramName, int cohort) {
  DataFrame paramdf = Rcpp::as<Rcpp::DataFrame>(x[paramType]);
  NumericVector param = paramdf[paramName];
  return(param[cohort]);
}

void modifyMessage(String paramName, String cohortName, double newValue) {
  Rcerr<< "[Message] Modifying parameter "<< paramName.get_cstring()<< "' of cohort '" << cohortName.get_cstring() << "' to value " << newValue <<".\n";
}
void multiplyMessage(String paramName, String cohortName, double factor) {
  Rcerr<< "[Message] Multiplying parameter "<< paramName.get_cstring()<< "' of cohort '" << cohortName.get_cstring() << "' by factor " << factor <<".\n";
}

void modifyInputParamSingle(List x, String paramType, String paramName, int cohort, double newValue) {
  DataFrame paramdf = Rcpp::as<Rcpp::DataFrame>(x[paramType]);
  NumericVector param = paramdf[paramName];
  param[cohort] = newValue;
}
void multiplyInputParamSingle(List x, String paramType, String paramName, int cohort, double f) {
  DataFrame paramdf = Rcpp::as<Rcpp::DataFrame>(x[paramType]);
  NumericVector param = paramdf[paramName];
  param[cohort] = param[cohort]*f;
}

// [[Rcpp::export(".multiplyInputParam")]]
void multiplyInputParam(List x, String paramType, String paramName, 
                        int cohort, double f,
                        bool message) {
  List control = x["control"];
  String transpirationMode = control["transpirationMode"];
  DataFrame cohorts = Rcpp::as<Rcpp::DataFrame>(x["cohorts"]);
  CharacterVector cohNames = cohorts.attr("row.names");
  if(paramName=="Z50/Z95") {
    multiplyInputParamSingle(x, "below", "Z50", cohort, f);
    multiplyInputParamSingle(x, "below", "Z95", cohort, f);
    if(message) Rcerr<< "[Message] Updating fine root distribution for cohort " << cohNames[cohort] <<".\n";
    updateFineRootDistribution(x);
  } else  if(paramName=="WaterStorage") {
    multiplyInputParamSingle(x, "paramsWaterStorage", "Vsapwood", cohort, f);
    multiplyInputParamSingle(x, "paramsWaterStorage", "Vleaf", cohort, f);
  } else if(paramName=="Plant_kmax") {
    multiplyInputParamSingle(x, "paramsTranspiration", "Plant_kmax", cohort, f);
    if(message) multiplyMessage("VCleaf_kmax", cohNames[cohort], f);
    multiplyInputParamSingle(x, "paramsTranspiration", "VCleaf_kmax", cohort, f);
    if(message) multiplyMessage("VCstem_kmax", cohNames[cohort], f);
    multiplyInputParamSingle(x, "paramsTranspiration", "VCstem_kmax", cohort, f);
    if(message) multiplyMessage("VCroot_kmax", cohNames[cohort], f);
    multiplyInputParamSingle(x, "paramsTranspiration", "VCroot_kmax", cohort, f);
    if(message) Rcerr<< "[Message] Updating below-ground conductances for cohort " << cohNames[cohort] <<".\n";
    updateBelowgroundConductances(x);
  } else if(paramName=="LAI_live") {
    multiplyInputParamSingle(x, "above", "LAI_live", cohort, f);
    if(message) multiplyMessage("LAI_expanded", cohNames[cohort], f);
    multiplyInputParamSingle(x, "above", "LAI_expanded", cohort, f);
  } else if(paramName=="c") {
    multiplyInputParamSingle(x, "paramsTranspiration", "VCleaf_c", cohort, f);
    multiplyInputParamSingle(x, "paramsTranspiration", "VCstem_c", cohort, f);
    multiplyInputParamSingle(x, "paramsTranspiration", "VCroot_c", cohort, f);
  } else if(paramName=="d") {
    multiplyInputParamSingle(x, "paramsTranspiration", "VCleaf_d", cohort, f);
    multiplyInputParamSingle(x, "paramsTranspiration", "VCstem_d", cohort, f);
    multiplyInputParamSingle(x, "paramsTranspiration", "VCroot_d", cohort, f);
  }else if(paramName=="Al2As") {
    multiplyInputParamSingle(x, "paramsAnatomy", "Al2As", cohort, f);
    if(message) multiplyMessage("Vsapwood", cohNames[cohort], f);
    multiplyInputParamSingle(x, "paramsWaterStorage", "Vsapwood", cohort, 1.0/f);
    if(transpirationMode=="Sperry") {
      if(message) multiplyMessage("VCstem_kmax", cohNames[cohort], f);
      multiplyInputParamSingle(x, "paramsTranspiration", "VCstem_kmax", cohort, 1.0/f);
      if(message) multiplyMessage("VCroot_kmax", cohNames[cohort], f);
      multiplyInputParamSingle(x, "paramsTranspiration", "VCroot_kmax", cohort, 1.0/f);
    }
    if(x.containsElementNamed("internalAllocation")) {
      DataFrame above = Rcpp::as<Rcpp::DataFrame>(x["above"]);
      DataFrame belowdf = Rcpp::as<Rcpp::DataFrame>(x["below"]);
      DataFrame paramsTranspirationdf = Rcpp::as<Rcpp::DataFrame>(x["paramsTranspiration"]);
      DataFrame paramsAnatomydf = Rcpp::as<Rcpp::DataFrame>(x["paramsAnatomy"]);
      if(message) Rcerr<< "[Message] Rebuilding allocation targets for cohort " << cohNames[cohort] <<".\n";
      x["internalAllocation"]  = internalAllocationDataFrame(above, 
                                           belowdf, 
                                           paramsAnatomydf,
                                           paramsTranspirationdf,
                                           control);
    }
  } else if(paramName=="Vmax298/Jmax298") {
    multiplyInputParamSingle(x, "paramsTranspiration", "Vmax298", cohort, f);
    multiplyInputParamSingle(x, "paramsTranspiration", "Jmax298", cohort, f);
  } else {
    multiplyInputParamSingle(x, paramType, paramName, cohort, f);
  }
  if(transpirationMode=="Sperry") {
    if(message) Rcerr<< "[Message] Recalculating plant maximum conductances.\n";
    updatePlantKmax(x);
  }
  if(message) Rcerr<< "[Message] Updating below-ground parameters.\n";
  updateBelow(x);
}


// [[Rcpp::export(".modifyInputParam")]]
void modifyInputParam(List x, String paramType, String paramName, 
                      int cohort, double newValue,
                      bool message) {
  List control = x["control"];
  DataFrame cohorts = Rcpp::as<Rcpp::DataFrame>(x["cohorts"]);
  DataFrame above = Rcpp::as<Rcpp::DataFrame>(x["above"]);
  CharacterVector cohNames = cohorts.attr("row.names");
  
  String transpirationMode = control["transpirationMode"];
  if(paramName=="LAI_live") {
    double old = getInputParamValue(x, "above", "LAI_live", cohort);
    double f = newValue/old;
    if(message) modifyMessage("LAI_live", cohNames[cohort], newValue);
    modifyInputParamSingle(x, "above", "LAI_live", cohort, newValue);
    if(message) multiplyMessage("LAI_expanded", cohNames[cohort], f);
    multiplyInputParamSingle(x, "above", "LAI_expanded", cohort, f);
    if(above.containsElementNamed("SA")) {
      if(message) multiplyMessage("SA", cohNames[cohort], f);
      multiplyInputParamSingle(x, "above", "SA", cohort, f);
    }
  } else if(paramName=="SLA") {
    double old = getInputParamValue(x, "paramsAnatomy", "SLA", cohort);
    double f = newValue/old;
    if(message) modifyMessage("SLA", cohNames[cohort], newValue);
    modifyInputParamSingle(x, "paramsAnatomy", "SLA", cohort, newValue);
    if(message) multiplyMessage("LAI_live", cohNames[cohort], f);
    multiplyInputParamSingle(x, "above", "LAI_live", cohort, f);
    if(message) multiplyMessage("LAI_expanded", cohNames[cohort], f);
    multiplyInputParamSingle(x, "above", "LAI_expanded", cohort, f);
    if(above.containsElementNamed("SA")) {
      if(message) multiplyMessage("SA", cohNames[cohort], f);
      multiplyInputParamSingle(x, "above", "SA", cohort, f);
    }
    if(x.containsElementNamed("paramsWaterStorage")) {
      if(message) multiplyMessage("Vleaf", cohNames[cohort], 1.0/f);
      multiplyInputParamSingle(x, "paramsWaterStorage", "Vleaf", cohort, 1.0/f);
    }
  } else if(paramName=="Al2As") {
    double old = getInputParamValue(x, "paramsAnatomy", "Al2As", cohort);
    double f = newValue/old;
    if(message) modifyMessage("Al2As", cohNames[cohort], newValue);
    modifyInputParamSingle(x, "paramsAnatomy", "Al2As", cohort, newValue);
    if(x.containsElementNamed("paramsWaterStorage")) {
      if(message) multiplyMessage("Vsapwood", cohNames[cohort], 1.0/f);
      multiplyInputParamSingle(x, "paramsWaterStorage", "Vsapwood", cohort, 1.0/f);
    }
    if(above.containsElementNamed("SA")) {
      if(message) multiplyMessage("SA", cohNames[cohort], 1.0/f);
      multiplyInputParamSingle(x, "above", "SA", cohort, 1.0/f);
    }
    if(transpirationMode=="Sperry") {
      if(message) multiplyMessage("VCstem_kmax", cohNames[cohort], 1.0/f);
      multiplyInputParamSingle(x, "paramsTranspiration", "VCstem_kmax", cohort, 1.0/f);
      if(message) multiplyMessage("VCroot_kmax", cohNames[cohort], 1.0/f);
      multiplyInputParamSingle(x, "paramsTranspiration", "VCroot_kmax", cohort, 1.0/f);
    }
  } else {
    if(message) modifyMessage(paramName, cohNames[cohort], newValue);
    modifyInputParamSingle(x, paramType, paramName, cohort, newValue);
  }
  if(transpirationMode=="Sperry") {
    if(message) Rcerr<< "[Message] Recalculating plant maximum conductances.\n";
    updatePlantKmax(x);
  }
  if(message) Rcerr<< "[Message] Updating below-ground parameters.\n";
  updateBelow(x);
  if(x.containsElementNamed("internalAllocation")) {
    DataFrame belowdf = Rcpp::as<Rcpp::DataFrame>(x["below"]);
    DataFrame paramsTranspirationdf = Rcpp::as<Rcpp::DataFrame>(x["paramsTranspiration"]);
    DataFrame paramsAnatomydf = Rcpp::as<Rcpp::DataFrame>(x["paramsAnatomy"]);
    if(message) Rcerr<< "[Message] Rebuilding allocation targets.\n";
    x["internalAllocation"]  = internalAllocationDataFrame(above, 
                                         belowdf, 
                                         paramsAnatomydf,
                                         paramsTranspirationdf,
                                         control);
  }
  if(x.containsElementNamed("internalCarbon")) {
    DataFrame belowdf = Rcpp::as<Rcpp::DataFrame>(x["below"]);
    List belowLayers = Rcpp::as<Rcpp::List>(x["belowLayers"]);
    DataFrame paramsGrowthdf = Rcpp::as<Rcpp::DataFrame>(x["paramsGrowth"]);
    DataFrame paramsWaterStoragedf = Rcpp::as<Rcpp::DataFrame>(x["paramsWaterStorage"]);
    DataFrame paramsTranspirationdf = Rcpp::as<Rcpp::DataFrame>(x["paramsTranspiration"]);
    DataFrame paramsAnatomydf = Rcpp::as<Rcpp::DataFrame>(x["paramsAnatomy"]);
    if(message) Rcerr<< "[Message] Rebuilding internal carbon.\n";
    x["internalCarbon"]  = internalCarbonDataFrame(above, 
                                     belowdf,
                                     belowLayers,
                                     paramsAnatomydf,
                                     paramsWaterStoragedf,
                                     paramsGrowthdf,
                                     control);
  }
}
