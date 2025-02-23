/**
    @file atmesc.c
    @brief Subroutines that control the integration of the
    atmospheric escape model.
    @author Rodrigo Luger ([rodluger@gmail.com](mailto:rodluger@gmail.com>))
    @date May 12 2015

    @par Description
    \rst
        This module defines differential equations controlling the evolution
        of planetary atmospheres under intense extreme ultraviolet (XUV)
        stellar irradiation. The `atmesc <atmesc.html>`_ module implements energy-limited
        and diffusion-limited escape for hydrogen/helium atmospheres and water
        vapor atmospheres following
        :cite:`Luger2015`, :cite:`LugerBarnes2015`, and :cite:`LehmerCatling17`.
    \endrst

*/

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "vplanet.h"

/**
Create a copy of the body at index \p iBody. Used during integration
of the differential equations.

@param dest The body copy
@param src The original body instance
@param foo Who knows!?
@param iNumBodies Number of bodies
@param iBody Current body index
*/
void BodyCopyAtmEsc(BODY *dest,BODY *src,int foo,int iNumBodies,int iBody) {
  dest[iBody].dSurfaceWaterMass = src[iBody].dSurfaceWaterMass;
  dest[iBody].dOxygenMass = src[iBody].dOxygenMass;
  dest[iBody].dOxygenMantleMass = src[iBody].dOxygenMantleMass;
  dest[iBody].dEnvelopeMass = src[iBody].dEnvelopeMass;
  dest[iBody].dXFrac = src[iBody].dXFrac;
  dest[iBody].dAtmXAbsEffH = src[iBody].dAtmXAbsEffH;
  dest[iBody].dAtmXAbsEffH2O = src[iBody].dAtmXAbsEffH2O;
  dest[iBody].dMinSurfaceWaterMass = src[iBody].dMinSurfaceWaterMass;
  dest[iBody].dMinEnvelopeMass = src[iBody].dMinEnvelopeMass;
  dest[iBody].iWaterLossModel = src[iBody].iWaterLossModel;
  dest[iBody].iAtmXAbsEffH2OModel = src[iBody].iAtmXAbsEffH2OModel;
  dest[iBody].dKTide = src[iBody].dKTide;
  dest[iBody].dMDotWater = src[iBody].dMDotWater;
  dest[iBody].dFHRef = src[iBody].dFHRef;
  dest[iBody].dOxygenEta = src[iBody].dOxygenEta;
  dest[iBody].dCrossoverMass = src[iBody].dCrossoverMass;
  dest[iBody].bRunaway = src[iBody].bRunaway;
  dest[iBody].iWaterEscapeRegime = src[iBody].iWaterEscapeRegime;
  dest[iBody].dFHDiffLim = src[iBody].dFHDiffLim;
  dest[iBody].iPlanetRadiusModel = src[iBody].iPlanetRadiusModel;
  dest[iBody].bInstantO2Sink = src[iBody].bInstantO2Sink;
  dest[iBody].dRGDuration = src[iBody].dRGDuration;
  dest[iBody].dRadXUV = src[iBody].dRadXUV;
  dest[iBody].dRadSolid = src[iBody].dRadSolid;
  dest[iBody].dPresXUV = src[iBody].dPresXUV;
  dest[iBody].dScaleHeight = src[iBody].dScaleHeight;
  dest[iBody].dThermTemp = src[iBody].dThermTemp;
  dest[iBody].dFlowTemp = src[iBody].dFlowTemp;
  dest[iBody].dAtmGasConst = src[iBody].dAtmGasConst;
  dest[iBody].dFXUV = src[iBody].dFXUV;
  dest[iBody].bCalcFXUV = src[iBody].bCalcFXUV;
  dest[iBody].dJeansTime = src[iBody].dJeansTime;
  dest[iBody].bRocheMessage = src[iBody].bRocheMessage;

}

/**************** ATMESC options ********************/

/**
Read the XUV flux from the input file.

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadFXUV(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile){
  int lTmp=-1;
  double dTmp;

  AddOptionDouble(files->Infile[iFile].cIn,options->cName,&dTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    if (dTmp < 0)
      body[iFile-1].dFXUV = dTmp*dNegativeDouble(*options,files->Infile[iFile].cIn,control->Io.iVerbose);
    else
      body[iFile-1].dFXUV = dTmp;
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else
    if (iFile > 0)
      body[iFile-1].dFXUV = options->dDefault;
}

/**
\rst
Read the thermospheric temperature for the :cite:`LehmerCatling17` atmospheric escape model.
\endrst

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadThermTemp(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile){
  int lTmp=-1;
  double dTmp;

  AddOptionDouble(files->Infile[iFile].cIn,options->cName,&dTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    if (dTmp < 0)
      body[iFile-1].dThermTemp = dTmp*dNegativeDouble(*options,files->Infile[iFile].cIn,control->Io.iVerbose);
    else
      body[iFile-1].dThermTemp = dTmp;
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else
    if (iFile > 0)
      body[iFile-1].dThermTemp = options->dDefault;
}

/**
\rst
Read the temperature of the hydro flow for the Luger+Barnes escape model.
\endrst

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadFlowTemp(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile){
  int lTmp=-1;
  double dTmp;

  AddOptionDouble(files->Infile[iFile].cIn,options->cName,&dTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    if (dTmp < 0)
      body[iFile-1].dFlowTemp = dTmp*dNegativeDouble(*options,files->Infile[iFile].cIn,control->Io.iVerbose);
    else
      body[iFile-1].dFlowTemp = dTmp;
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else
    if (iFile > 0)
      body[iFile-1].dFlowTemp = options->dDefault;
}

/**
Read the atmospheric gas constant the Lehmer and Catling (2017) atmospheric escape model.

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadAtmGasConst(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile){
  int lTmp=-1;
  double dTmp;

  AddOptionDouble(files->Infile[iFile].cIn,options->cName,&dTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    if (dTmp < 0)
      body[iFile-1].dAtmGasConst = dTmp*dNegativeDouble(*options,files->Infile[iFile].cIn,control->Io.iVerbose);
    else
      body[iFile-1].dAtmGasConst = dTmp;
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else
    if (iFile > 0)
      body[iFile-1].dAtmGasConst = options->dDefault;
}

/**
Read the Jeans time, the time at which the flow transitions from hydrodynamic to ballistic.

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadJeansTime(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile) {
  /* This parameter cannot exist in primary file */
  int lTmp=-1;
  double dTmp;

  AddOptionDouble(files->Infile[iFile].cIn,options->cName,&dTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    if (dTmp < 0)
      body[iFile-1].dJeansTime = dTmp*dNegativeDouble(*options,files->Infile[iFile].cIn,control->Io.iVerbose);
    else
      body[iFile-1].dJeansTime = dTmp*fdUnitsTime(control->Units[iFile].iTime);
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else {
    if (iFile > 0) {
      if (control->Io.iVerbose >= VERBINPUT) {
        fprintf(stderr,"INFO: %s not set for body %s, defaulting to %.2e seconds.\n",
          options->cName,body[iFile-1].cName,options->dDefault);
      }
      body[iFile-1].dJeansTime = options->dDefault;
    }
  }
}

/**
Read the effective XUV absorption pressure for the Lehmner and Catling (2017) model.

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadPresXUV(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile){
  int lTmp=-1;
  double dTmp;

  AddOptionDouble(files->Infile[iFile].cIn,options->cName,&dTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    if (dTmp < 0)
      body[iFile-1].dPresXUV = dTmp*dNegativeDouble(*options,files->Infile[iFile].cIn,control->Io.iVerbose);
    else
      body[iFile-1].dPresXUV = dTmp;
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else
    if (iFile > 0)
      body[iFile-1].dPresXUV = options->dDefault;
}

/**
Read the water loss model for the Luger and Barnes (2015) atmospheric escape model.

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadWaterLossModel(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile) {
  /* This parameter cannot exist in primary file */
  int lTmp=-1;
  char cTmp[OPTLEN];

  AddOptionString(files->Infile[iFile].cIn,options->cName,cTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    if (!memcmp(sLower(cTmp),"lb15",4)) {
      body[iFile-1].iWaterLossModel = ATMESC_LB15;
    } else if (!memcmp(sLower(cTmp),"lbex",4)) {
      body[iFile-1].iWaterLossModel = ATMESC_LBEXACT;
    } else if (!memcmp(sLower(cTmp),"tian",4)) {
      body[iFile-1].iWaterLossModel = ATMESC_TIAN;
    } else {
      if (control->Io.iVerbose >= VERBERR)
	      fprintf(stderr,"ERROR: Unknown argument to %s: %s. Options are LB15, LBEXACT, or TIAN.\n",options->cName,cTmp);
      LineExit(files->Infile[iFile].cIn,lTmp);
    }
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else
    if (iFile > 0)
      body[iFile-1].iWaterLossModel = ATMESC_LBEXACT;
}

/**
Read the XUV absorption efficiency model for the Luger and Barnes (2015) atmospheric escape model.

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadAtmXAbsEffH2OModel(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile) {
  /* This parameter cannot exist in primary file */
  int lTmp=-1;
  char cTmp[OPTLEN];

  AddOptionString(files->Infile[iFile].cIn,options->cName,cTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    if (!memcmp(sLower(cTmp),"bolm",4)) {
      body[iFile-1].iAtmXAbsEffH2OModel = ATMESC_BOL16;
    } else if (!memcmp(sLower(cTmp),"none",4)) {
      body[iFile-1].iAtmXAbsEffH2OModel = ATMESC_NONE;
    } else {
      if (control->Io.iVerbose >= VERBERR)
	      fprintf(stderr,"ERROR: Unknown argument to %s: %s. Options are BOLMONT16 or NONE.\n",options->cName,cTmp);
      LineExit(files->Infile[iFile].cIn,lTmp);
    }
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else
    if (iFile > 0)
      body[iFile-1].iAtmXAbsEffH2OModel = ATMESC_NONE;
}

/**
Read the planet radius model.

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadPlanetRadiusModel(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile) {
  /* This parameter cannot exist in primary file */
  int lTmp=-1;
  char cTmp[OPTLEN];

  AddOptionString(files->Infile[iFile].cIn,options->cName,cTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    if (!memcmp(sLower(cTmp),"lo",2)) {
      body[iFile-1].iPlanetRadiusModel = ATMESC_LOP12;
    } else if (!memcmp(sLower(cTmp),"le",2)) {
      body[iFile-1].iPlanetRadiusModel = ATMESC_LEHMER17;
    } else if (!memcmp(sLower(cTmp),"pr",2)) {
      body[iFile-1].iPlanetRadiusModel = ATMESC_PROXCENB;
    } else if (!memcmp(sLower(cTmp),"no",2)) {
      body[iFile-1].iPlanetRadiusModel = ATMESC_NONE;
    }
     else {
      if (control->Io.iVerbose >= VERBERR)
	      fprintf(stderr,"ERROR: Unknown argument to %s: %s. Options are LOPEZ12, PROXCENB, LEHMER17 or NONE.\n",options->cName,cTmp);
      LineExit(files->Infile[iFile].cIn,lTmp);
    }
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else
    if (iFile > 0)
      body[iFile-1].iPlanetRadiusModel = ATMESC_NONE;
}

/**
Read the parameter that controls surface O2 sinks for the Luger and Barnes (2015) model.

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadInstantO2Sink(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile) {
  /* This parameter cannot exist in primary file */
  int lTmp=-1;
  int bTmp;

  AddOptionBool(files->Infile[iFile].cIn,options->cName,&bTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    body[iFile-1].bInstantO2Sink = bTmp;
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else
    if (iFile > 0)
      AssignDefaultInt(options,&body[iFile-1].bInstantO2Sink,files->iNumInputs);
}

/**
Read the planet's effective XUV radius.

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadXFrac(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile) {
  /* This parameter cannot exist in primary file */
  int lTmp=-1;
  double dTmp;

  AddOptionDouble(files->Infile[iFile].cIn,options->cName,&dTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    if (dTmp < 0) {
      if (control->Io.iVerbose >= VERBERR)
	      fprintf(stderr,"ERROR: %s must be >= 0.\n",options->cName);
      LineExit(files->Infile[iFile].cIn,lTmp);
    }
    body[iFile-1].dXFrac = dTmp;
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else
    if (iFile > 0)
      body[iFile-1].dXFrac = options->dDefault;
}

/**
Read the XUV absorption efficiency for hydrogen.

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadAtmXAbsEffH(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile) {
  /* This parameter cannot exist in primary file */
  int lTmp=-1;
  double dTmp;

  AddOptionDouble(files->Infile[iFile].cIn,options->cName,&dTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    if (dTmp < 0) {
      if (control->Io.iVerbose >= VERBERR)
	      fprintf(stderr,"ERROR: %s must be >= 0.\n",options->cName);
      LineExit(files->Infile[iFile].cIn,lTmp);
    }
    body[iFile-1].dAtmXAbsEffH = dTmp;
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else
    if (iFile > 0)
      body[iFile-1].dAtmXAbsEffH = options->dDefault;
}

/**
Read the XUV absorption efficiency for water.

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadAtmXAbsEffH2O(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile) {
  /* This parameter cannot exist in primary file */
  int lTmp=-1;
  double dTmp;

  AddOptionDouble(files->Infile[iFile].cIn,options->cName,&dTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    if (dTmp < 0) {
      if (control->Io.iVerbose >= VERBERR)
	      fprintf(stderr,"ERROR: %s must be >= 0.\n",options->cName);
      LineExit(files->Infile[iFile].cIn,lTmp);
    }
    body[iFile-1].dAtmXAbsEffH2O = dTmp;
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else
    if (iFile > 0)
      body[iFile-1].dAtmXAbsEffH2O = options->dDefault;
}

// ReadEnvelopeMass is in options.c to avoid memory leaks in verifying envelope.

/**
Read the planet's initial atmospheric oxygen mass (Luger and Barnes 2015 model).

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadOxygenMass(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile) {
  /* This parameter cannot exist in primary file */
  int lTmp=-1;
  double dTmp;

  AddOptionDouble(files->Infile[iFile].cIn,options->cName,&dTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    if (dTmp < 0) {
      if (control->Io.iVerbose >= VERBERR)
	      fprintf(stderr,"ERROR: %s must be >= 0.\n",options->cName);
      LineExit(files->Infile[iFile].cIn,lTmp);
    }
    body[iFile-1].dOxygenMass = dTmp;
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else
    if (iFile > 0)
      body[iFile-1].dOxygenMass = options->dDefault;
}

/**
Read the planet's initial mantle oxygen mass (Luger and Barnes 2015 model).

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadOxygenMantleMass(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile) {
  /* This parameter cannot exist in primary file */
  int lTmp=-1;
  double dTmp;

  AddOptionDouble(files->Infile[iFile].cIn,options->cName,&dTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    if (dTmp < 0) {
      if (control->Io.iVerbose >= VERBERR)
	      fprintf(stderr,"ERROR: %s must be >= 0.\n",options->cName);
      LineExit(files->Infile[iFile].cIn,lTmp);
    }
    body[iFile-1].dOxygenMantleMass = dTmp;
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else
    if (iFile > 0)
      body[iFile-1].dOxygenMantleMass = options->dDefault;
}

/* Halts */

/**
Read the parameter that controls whether the code halts when the planet is desiccated.

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadHaltMinSurfaceWaterMass(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile) {
  /* This parameter cannot exist in primary file */
  int lTmp=-1;
  int bTmp;

  AddOptionBool(files->Infile[iFile].cIn,options->cName,&bTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    control->Halt[iFile-1].bSurfaceDesiccated = bTmp;
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else {
    if (iFile > 0)
      AssignDefaultInt(options,&control->Halt[iFile-1].bSurfaceDesiccated,files->iNumInputs);
  }
}

/* ReadMinSurfaceWaterMass is in options.c to avoid memory leaks when verifying
  ocean. */

/* ReadSurfaceWaterMass is in options.c to avoid memory leaks when verifying
  ocean. */

/**
Read the parameter that controls whether the code halts when the planet's envelope is fully evaporated.

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param iFile The current file number
*/
void ReadHaltMinEnvelopeMass(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,int iFile) {
  /* This parameter cannot exist in primary file */
  int lTmp=-1;
  int bTmp;

  AddOptionBool(files->Infile[iFile].cIn,options->cName,&bTmp,&lTmp,control->Io.iVerbose);
  if (lTmp >= 0) {
    NotPrimaryInput(iFile,options->cName,files->Infile[iFile].cIn,lTmp,control->Io.iVerbose);
    control->Halt[iFile-1].bEnvelopeGone = bTmp;
    UpdateFoundOption(&files->Infile[iFile],options,lTmp,iFile);
  } else {
    if (iFile > 0)
      AssignDefaultInt(options,&control->Halt[iFile-1].bEnvelopeGone,files->iNumInputs);
  }
}

// ReadMinEnvelopeMass is in options.c to eliminate memory leak in verifying envelope

/**
Initialize the user options for the atmospheric escape model.

@param options A pointer to the OPTIONS instance
@param fnRead Array of pointers to the functions that read in the options
*/
void InitializeOptionsAtmEsc(OPTIONS *options,fnReadOption fnRead[]) {
  int iOpt,iFile;

  sprintf(options[OPT_XFRAC].cName,"dXFrac");
  sprintf(options[OPT_XFRAC].cDescr,"Fraction of planet radius in X-ray/XUV");
  sprintf(options[OPT_XFRAC].cDefault,"1");
  options[OPT_XFRAC].dDefault = 1;
  options[OPT_XFRAC].iType = 2;
  options[OPT_XFRAC].iMultiFile = 1;
  fnRead[OPT_XFRAC] = &ReadXFrac;

  sprintf(options[OPT_ATMXABSEFFH].cName,"dAtmXAbsEffH");
  sprintf(options[OPT_ATMXABSEFFH].cDescr,"Hydrogen X-ray/XUV absorption efficiency (epsilon)");
  sprintf(options[OPT_ATMXABSEFFH].cDefault,"0.15");
  options[OPT_ATMXABSEFFH].dDefault = 0.15;
  options[OPT_ATMXABSEFFH].iType = 2;
  options[OPT_ATMXABSEFFH].iMultiFile = 1;
  fnRead[OPT_ATMXABSEFFH] = &ReadAtmXAbsEffH;

  sprintf(options[OPT_ATMXABSEFFH2O].cName,"dAtmXAbsEffH2O");
  sprintf(options[OPT_ATMXABSEFFH2O].cDescr,"Water X-ray/XUV absorption efficiency (epsilon)");
  sprintf(options[OPT_ATMXABSEFFH2O].cDefault,"0.30");
  options[OPT_ATMXABSEFFH2O].dDefault = 0.15;
  options[OPT_ATMXABSEFFH2O].iType = 2;
  options[OPT_ATMXABSEFFH2O].iMultiFile = 1;
  fnRead[OPT_ATMXABSEFFH2O] = &ReadAtmXAbsEffH2O;

  sprintf(options[OPT_ATMXABSEFFH2OMODEL].cName,"sAtmXAbsEffH2OModel");
  sprintf(options[OPT_ATMXABSEFFH2OMODEL].cDescr,"Water X-ray/XUV absorption efficiency evolution model");
  sprintf(options[OPT_ATMXABSEFFH2OMODEL].cDefault,"NONE");
  options[OPT_ATMXABSEFFH2OMODEL].iType = 3;
  options[OPT_ATMXABSEFFH2OMODEL].iMultiFile = 1;
  fnRead[OPT_ATMXABSEFFH2OMODEL] = &ReadAtmXAbsEffH2OModel;

  sprintf(options[OPT_OXYGENMASS].cName,"dOxygenMass");
  sprintf(options[OPT_OXYGENMASS].cDescr,"Initial Oxygen Mass");
  sprintf(options[OPT_OXYGENMASS].cDefault,"0");
  options[OPT_OXYGENMASS].dDefault = 0;
  options[OPT_OXYGENMASS].iType = 2;
  options[OPT_OXYGENMASS].iMultiFile = 1;
  fnRead[OPT_OXYGENMASS] = &ReadOxygenMass;

  sprintf(options[OPT_OXYGENMANTLEMASS].cName,"dOxygenMantleMass");
  sprintf(options[OPT_OXYGENMANTLEMASS].cDescr,"Initial Oxygen Mass in the Mantle");
  sprintf(options[OPT_OXYGENMANTLEMASS].cDefault,"0");
  options[OPT_OXYGENMANTLEMASS].dDefault = 0;
  options[OPT_OXYGENMANTLEMASS].iType = 2;
  options[OPT_OXYGENMANTLEMASS].iMultiFile = 1;
  fnRead[OPT_OXYGENMANTLEMASS] = &ReadOxygenMantleMass;

  sprintf(options[OPT_WATERLOSSMODEL].cName,"sWaterLossModel");
  sprintf(options[OPT_WATERLOSSMODEL].cDescr,"Water Loss and Oxygen Buildup Model");
  sprintf(options[OPT_WATERLOSSMODEL].cDefault,"LBEXACT");
  options[OPT_WATERLOSSMODEL].iType = 3;
  options[OPT_WATERLOSSMODEL].iMultiFile = 1;
  fnRead[OPT_WATERLOSSMODEL] = &ReadWaterLossModel;

  sprintf(options[OPT_PLANETRADIUSMODEL].cName,"sPlanetRadiusModel");
  sprintf(options[OPT_PLANETRADIUSMODEL].cDescr,"Gaseous Planet Radius Model");
  sprintf(options[OPT_PLANETRADIUSMODEL].cDefault,"NONE");
  options[OPT_PLANETRADIUSMODEL].iType = 3;
  options[OPT_PLANETRADIUSMODEL].iMultiFile = 1;
  fnRead[OPT_PLANETRADIUSMODEL] = &ReadPlanetRadiusModel;

  sprintf(options[OPT_INSTANTO2SINK].cName,"bInstantO2Sink");
  sprintf(options[OPT_INSTANTO2SINK].cDescr,"Is oxygen absorbed instantaneously at the surface?");
  sprintf(options[OPT_INSTANTO2SINK].cDefault,"0");
  options[OPT_INSTANTO2SINK].iType = 0;
  options[OPT_INSTANTO2SINK].iMultiFile = 1;
  fnRead[OPT_INSTANTO2SINK] = &ReadInstantO2Sink;

  sprintf(options[OPT_HALTDESICCATED].cName,"bHaltSurfaceDesiccated");
  sprintf(options[OPT_HALTDESICCATED].cDescr,"Halt at Desiccation?");
  sprintf(options[OPT_HALTDESICCATED].cDefault,"0");
  options[OPT_HALTDESICCATED].iType = 0;
  fnRead[OPT_HALTDESICCATED] = &ReadHaltMinSurfaceWaterMass;

  sprintf(options[OPT_HALTENVELOPEGONE].cName,"bHaltEnvelopeGone");
  sprintf(options[OPT_HALTENVELOPEGONE].cDescr,"Halt When Envelope Evaporates?");
  sprintf(options[OPT_HALTENVELOPEGONE].cDefault,"0");
  options[OPT_HALTENVELOPEGONE].iType = 0;
  fnRead[OPT_HALTENVELOPEGONE] = &ReadHaltMinEnvelopeMass;

  sprintf(options[OPT_THERMTEMP].cName,"dThermTemp");
  sprintf(options[OPT_THERMTEMP].cDescr,"Thermosphere temperature");
  sprintf(options[OPT_THERMTEMP].cDefault,"880");
  options[OPT_THERMTEMP].dDefault = 880;
  options[OPT_THERMTEMP].iType = 2;
  options[OPT_THERMTEMP].iMultiFile = 1;
  fnRead[OPT_THERMTEMP] = &ReadThermTemp;

  sprintf(options[OPT_FLOWTEMP].cName,"dFlowTemp");
  sprintf(options[OPT_FLOWTEMP].cDescr,"Temperature of the hydrodynamic flow");
  sprintf(options[OPT_FLOWTEMP].cDefault,"400");
  options[OPT_FLOWTEMP].dDefault = 400;
  options[OPT_FLOWTEMP].iType = 2;
  options[OPT_FLOWTEMP].iMultiFile = 1;
  fnRead[OPT_FLOWTEMP] = &ReadFlowTemp;

  sprintf(options[OPT_JEANSTIME].cName,"dJeansTime");
  sprintf(options[OPT_JEANSTIME].cDescr,"Time at which flow transitions to Jeans escape");
  sprintf(options[OPT_JEANSTIME].cDefault,"1 Gyr");
  options[OPT_JEANSTIME].dDefault = 1.e9 * YEARSEC;
  options[OPT_JEANSTIME].iType = 0;
  options[OPT_JEANSTIME].iMultiFile = 1;
  options[OPT_JEANSTIME].dNeg = 1.e9 * YEARSEC;
  sprintf(options[OPT_JEANSTIME].cNeg,"Gyr");
  fnRead[OPT_JEANSTIME] = &ReadJeansTime;

  sprintf(options[OPT_PRESXUV].cName,"dPresXUV");
  sprintf(options[OPT_PRESXUV].cDescr,"Pressure at base of Thermosphere");
  sprintf(options[OPT_PRESXUV].cDefault,"5 Pa");
  options[OPT_PRESXUV].dDefault = 5.0;
  options[OPT_PRESXUV].iType = 2;
  options[OPT_PRESXUV].iMultiFile = 1;
  fnRead[OPT_PRESXUV] = &ReadPresXUV;

  sprintf(options[OPT_ATMGASCONST].cName,"dAtmGasConst");
  sprintf(options[OPT_ATMGASCONST].cDescr,"Atmospheric Gas Constant");
  sprintf(options[OPT_ATMGASCONST].cDefault,"4124");
  options[OPT_ATMGASCONST].dDefault = 4124.0;
  options[OPT_ATMGASCONST].iType = 2;
  options[OPT_ATMGASCONST].iMultiFile = 1;
  fnRead[OPT_ATMGASCONST] = &ReadAtmGasConst;

  sprintf(options[OPT_FXUV].cName,"dFXUV");
  sprintf(options[OPT_FXUV].cDescr,"XUV Flux");
  options[OPT_FXUV].iType = 2;
  options[OPT_FXUV].iMultiFile = 1;
  fnRead[OPT_FXUV] = &ReadFXUV;
}

/**
Loops through the input files and reads all user options for the atmospheric escape model.

@param body A pointer to the current BODY instance
@param control A pointer to the integration CONTROL instance
@param files A pointer to the array of input FILES
@param options A pointer to the OPTIONS instance
@param system A pointer to the SYSTEM instance
@param fnRead Array of pointers to the functions that read in the options
@param iBody The current BODY number
*/
void ReadOptionsAtmEsc(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,SYSTEM *system,fnReadOption fnRead[],int iBody) {
  int iOpt;

  for (iOpt=OPTSTARTATMESC;iOpt<OPTENDATMESC;iOpt++) {
    if (options[iOpt].iType != -1)
      fnRead[iOpt](body,control,files,&options[iOpt],system,iBody+1);
  }
}

/******************* Verify ATMESC ******************/

/**
Initializes the differential equation matrix for the surface water mass.

@param body A pointer to the current BODY instance
@param options A pointer to the OPTIONS instance
@param update A pointer to the UPDATE instance
@param dAge The current age of the system
@param iBody The current BODY number
*/
void VerifySurfaceWaterMass(BODY *body,OPTIONS *options,UPDATE *update,double dAge,int iBody) {

  update[iBody].iaType[update[iBody].iSurfaceWaterMass][0] = 1;
  update[iBody].iNumBodies[update[iBody].iSurfaceWaterMass][0] = 1;
  update[iBody].iaBody[update[iBody].iSurfaceWaterMass][0] = malloc(update[iBody].iNumBodies[update[iBody].iSurfaceWaterMass][0]*sizeof(int));
  update[iBody].iaBody[update[iBody].iSurfaceWaterMass][0][0] = iBody;

  update[iBody].pdDSurfaceWaterMassDtAtmesc = &update[iBody].daDerivProc[update[iBody].iSurfaceWaterMass][0];
}

/**
Initializes the differential equation matrix for the atmospheric oxygen mass.

@param body A pointer to the current BODY instance
@param options A pointer to the OPTIONS instance
@param update A pointer to the UPDATE instance
@param dAge The current age of the system
@param iBody The current BODY number
*/
void VerifyOxygenMass(BODY *body,OPTIONS *options,UPDATE *update,double dAge,int iBody) {

  update[iBody].iaType[update[iBody].iOxygenMass][0] = 1;
  update[iBody].iNumBodies[update[iBody].iOxygenMass][0] = 1;
  update[iBody].iaBody[update[iBody].iOxygenMass][0] = malloc(update[iBody].iNumBodies[update[iBody].iOxygenMass][0]*sizeof(int));
  update[iBody].iaBody[update[iBody].iOxygenMass][0][0] = iBody;

  update[iBody].pdDOxygenMassDtAtmesc = &update[iBody].daDerivProc[update[iBody].iOxygenMass][0];
}

/**
Initializes the differential equation matrix for the mantle oxygen mass.

@param body A pointer to the current BODY instance
@param options A pointer to the OPTIONS instance
@param update A pointer to the UPDATE instance
@param dAge The current age of the system
@param iBody The current BODY number
*/
void VerifyOxygenMantleMass(BODY *body,OPTIONS *options,UPDATE *update,double dAge,int iBody) {

  update[iBody].iaType[update[iBody].iOxygenMantleMass][0] = 1;
  update[iBody].iNumBodies[update[iBody].iOxygenMantleMass][0] = 1;
  update[iBody].iaBody[update[iBody].iOxygenMantleMass][0] = malloc(update[iBody].iNumBodies[update[iBody].iOxygenMantleMass][0]*sizeof(int));
  update[iBody].iaBody[update[iBody].iOxygenMantleMass][0][0] = iBody;

  update[iBody].pdDOxygenMantleMassDtAtmesc = &update[iBody].daDerivProc[update[iBody].iOxygenMantleMass][0];
}

/**
Initializes the differential equation matrix for the gaseous envelope mass.

@param body A pointer to the current BODY instance
@param options A pointer to the OPTIONS instance
@param update A pointer to the UPDATE instance
@param dAge The current age of the system
@param iBody The current BODY number
*/
void VerifyEnvelopeMass(BODY *body,OPTIONS *options,UPDATE *update,double dAge,int iBody) {

  update[iBody].iaType[update[iBody].iEnvelopeMass][0] = 1;
  update[iBody].iNumBodies[update[iBody].iEnvelopeMass][0] = 1;
  update[iBody].iaBody[update[iBody].iEnvelopeMass][0] = malloc(update[iBody].iNumBodies[update[iBody].iEnvelopeMass][0]*sizeof(int));
  update[iBody].iaBody[update[iBody].iEnvelopeMass][0][0] = iBody;

  update[iBody].pdDEnvelopeMassDtAtmesc = &update[iBody].daDerivProc[update[iBody].iEnvelopeMass][0];
}

/**
Initializes the differential equation matrix for the planet mass.

@param body A pointer to the current BODY instance
@param options A pointer to the OPTIONS instance
@param update A pointer to the UPDATE instance
@param dAge The current age of the system
@param iBody The current BODY number
*/
void VerifyMassAtmEsc(BODY *body,OPTIONS *options,UPDATE *update,double dAge,int iBody) {

  update[iBody].iaType[update[iBody].iMass][0] = 1;
  update[iBody].iNumBodies[update[iBody].iMass][0] = 1;
  update[iBody].iaBody[update[iBody].iMass][0] = malloc(update[iBody].iNumBodies[update[iBody].iMass][0]*sizeof(int));
  update[iBody].iaBody[update[iBody].iMass][0][0] = iBody;

  update[iBody].pdDMassDtAtmesc = &update[iBody].daDerivProc[update[iBody].iMass][0];
}

/**
Initializes the differential equation matrix for the planet radius.

@param body A pointer to the current BODY instance
@param control A pointer to the CONTROL instance
@param options A pointer to the OPTIONS instance
@param update A pointer to the UPDATE instance
@param dAge The current age of the system
@param iBody The current BODY number
*/
void VerifyRadiusAtmEsc(BODY *body, CONTROL *control, OPTIONS *options,UPDATE *update,double dAge,int iBody) {

  // Assign radius
  if (body[iBody].iPlanetRadiusModel == ATMESC_LOP12) {
    body[iBody].dRadius = fdLopezRadius(body[iBody].dMass, body[iBody].dEnvelopeMass / body[iBody].dMass, 1., body[iBody].dAge, 0);

    // If there is no envelope and Lopez Radius specified, use Sotin+2007 radius!
    if(body[iBody].dEnvelopeMass <= body[iBody].dMinEnvelopeMass) {
      if (control->Io.iVerbose >= VERBINPUT)
        printf("INFO: Lopez+2012 Radius model specified, but no envelope present. Using Sotin+2007 Mass-radius relation to compute planet's solid radius.\n");

      // Set radius using Sotin+2007 model
      body[iBody].dRadius = fdMassToRad_Sotin07(body[iBody].dMass);
    }

    if (options[OPT_RADIUS].iLine[iBody+1] >= 0) {
      // User specified radius, but we're reading it from the grid!
      if (control->Io.iVerbose >= VERBINPUT)
        printf("INFO: Radius set for body %d, but this value will be computed from the grid.\n", iBody);
    }
  } else if (body[iBody].iPlanetRadiusModel == ATMESC_PROXCENB) {
    body[iBody].dRadius = fdProximaCenBRadius(body[iBody].dEnvelopeMass / body[iBody].dMass, body[iBody].dAge, body[iBody].dMass);
    if (options[OPT_RADIUS].iLine[iBody+1] >= 0) {
      // User specified radius, but we're reading it from the grid!
      if (control->Io.iVerbose >= VERBINPUT)
        printf("INFO: Radius set for body %d, but this value will be computed from the grid.\n", iBody);
    }
  }

  update[iBody].iaType[update[iBody].iRadius][0] = 0;
  update[iBody].iNumBodies[update[iBody].iRadius][0] = 1;
  update[iBody].iaBody[update[iBody].iRadius][0] = malloc(update[iBody].iNumBodies[update[iBody].iRadius][0]*sizeof(int));
  update[iBody].iaBody[update[iBody].iRadius][0][0] = iBody;

  update[iBody].pdRadiusAtmesc = &update[iBody].daDerivProc[update[iBody].iRadius][0];   // NOTE: This points to the VALUE of the radius

}

/**
This function is run during every step of the integrator to
perform checks and force certain non-diffeq behavior.

@param body A pointer to the current BODY instance
@param module A pointer to the MODULE instance
@param evolve A pointer to the EVOLVE instance
@param io A pointer to the IO instance
@param system A pointer to the SYSTEM instance
@param update A pointer to the UPDATE instance
@param fnUpdate A triple-pointer to the function that updates each variable
@param iBody The current BODY number
@param iModule The current MODULE number
*/
void fnForceBehaviorAtmEsc(BODY *body,MODULE *module,EVOLVE *evolve,IO *io,SYSTEM *system,UPDATE *update,fnUpdateVariable ***fnUpdate,int iBody,int iModule) {

  if ((body[iBody].dSurfaceWaterMass <= body[iBody].dMinSurfaceWaterMass) && (body[iBody].dSurfaceWaterMass > 0.)){
    // Let's desiccate this planet.
    body[iBody].dSurfaceWaterMass = 0.;
  }

  if ((body[iBody].dEnvelopeMass <= body[iBody].dMinEnvelopeMass) && (body[iBody].dEnvelopeMass > 0.)) {
    // Let's remove its envelope and prevent further evolution.
    body[iBody].dEnvelopeMass = 0.;
    fnUpdate[iBody][update[iBody].iEnvelopeMass][0] = &fndUpdateFunctionTiny;

    // If using Lopez+2012 radius model, set radius to Sotin+2007 radius
    if(body[iBody].iPlanetRadiusModel == ATMESC_LOP12) {
      // Let user know what's happening
      if (io->iVerbose >= VERBPROG && !body[iBody].bEnvelopeLostMessage) {
        printf("%s's envelope removed. Used Lopez+12 radius models for envelope, switching to Sotin+2007 model for solid planet radius.\n",body[iBody].cName);
        body[iBody].bEnvelopeLostMessage = 1;
      }
      // Update radius
      body[iBody].dRadius = fdMassToRad_Sotin07(body[iBody].dMass);
    }
  }
}


/**
Initializes several helper variables and properties used in the integration.

@param body A pointer to the current BODY instance
@param evolve A pointer to the EVOLVE instance
@param update A pointer to the UPDATE instance
@param iBody The current BODY number
*/
void fnPropsAuxAtmEsc(BODY *body, EVOLVE *evolve, IO *io, UPDATE *update, int iBody) {

  body[iBody].dAge = body[0].dAge;

  if (body[iBody].iPlanetRadiusModel == ATMESC_LEHMER17) {
    body[iBody].dRadSolid = 1.3 * pow(body[iBody].dMass - body[iBody].dEnvelopeMass, 0.27);
    body[iBody].dGravAccel = BIGG * (body[iBody].dMass - body[iBody].dEnvelopeMass) / (body[iBody].dRadSolid * body[iBody].dRadSolid);
    body[iBody].dScaleHeight = body[iBody].dAtmGasConst * body[iBody].dThermTemp / body[iBody].dGravAccel;
    body[iBody].dPresSurf = fdLehmerPres(body[iBody].dEnvelopeMass, body[iBody].dGravAccel, body[iBody].dRadSolid);
    body[iBody].dRadXUV = fdLehmerRadius(body[iBody].dRadSolid, body[iBody].dPresXUV, body[iBody].dScaleHeight,body[iBody].dPresSurf);
  }

  // Ktide (due to body zero only). WARNING: not suited for binary...
  double xi = (pow(body[iBody].dMass / (3. * body[0].dMass), (1. / 3)) *
               body[iBody].dSemi) / (body[iBody].dRadius * body[iBody].dXFrac);

  // For circumbinary planets, assume no Ktide enhancement
  if (body[iBody].bBinary && body[iBody].iBodyType == 0) {
      body[iBody].dKTide = 1.0;
  } else {
      if (xi > 1) {
        body[iBody].dKTide = (1 - 3 / (2 * xi) + 1 / (2 * pow(xi, 3)));
      } else {
        if (!body[iBody].bRocheMessage && io->iVerbose >= VERBINPUT) {
          fprintf(stderr,"WARNING: Roche lobe radius is larger than XUV radius for %s, evolution may not be accurate.\n",
              body[iBody].cName);
          body[iBody].bRocheMessage = 1;
        }
      }
      body[iBody].dKTide = 1.0;
  }

  // The XUV flux
  if (body[iBody].bCalcFXUV){
    body[iBody].dFXUV = fdXUVFlux(body, iBody);
  }

  // The H2O XUV escape efficiency
  if (body[iBody].iAtmXAbsEffH2OModel == ATMESC_BOL16)
    body[iBody].dAtmXAbsEffH2O = fdXUVEfficiencyBolmont2016(body[iBody].dFXUV);

  // Reference hydrogen flux for the water loss
  body[iBody].dFHRef = (body[iBody].dAtmXAbsEffH2O * body[iBody].dFXUV * body[iBody].dRadius) /
                       (4 * BIGG * body[iBody].dMass * body[iBody].dKTide * ATOMMASS);

  // Surface gravity
  double g = (BIGG * body[iBody].dMass) / (body[iBody].dRadius * body[iBody].dRadius);

  // Oxygen mixing ratio
  double XO = fdAtomicOxygenMixingRatio(body[iBody].dSurfaceWaterMass, body[iBody].dOxygenMass);

  // Diffusion-limited H escape rate
  double BDIFF = 4.8e19 * pow(body[iBody].dFlowTemp, 0.75);
  body[iBody].dFHDiffLim = BDIFF * g * ATOMMASS * (QOH - 1.) / (KBOLTZ * body[iBody].dFlowTemp * (1. + XO / (1. - XO)));

  // Is water escaping?
  if (!fbDoesWaterEscape(body, iBody)) {

    body[iBody].dOxygenEta = 0;
    body[iBody].dCrossoverMass = 0;
    body[iBody].bRunaway = 0;
    body[iBody].iWaterEscapeRegime = ATMESC_NONE;
    body[iBody].dMDotWater = 0;

  } else {

    body[iBody].bRunaway = 1;

    // Select an escape/oxygen buildup model
    if (body[iBody].iWaterLossModel == ATMESC_LB15) {

      // Luger and Barnes (2015)
      double x = (KBOLTZ * body[iBody].dFlowTemp * body[iBody].dFHRef) / (10 * BDIFF * g * ATOMMASS);
      if (x < 1) {
        body[iBody].dOxygenEta = 0;
        body[iBody].dCrossoverMass = ATOMMASS + 1.5 * KBOLTZ * body[iBody].dFlowTemp * body[iBody].dFHRef / (BDIFF * g);
      } else {
        body[iBody].dOxygenEta = (x - 1) / (x + 8);
        body[iBody].dCrossoverMass = 43. / 3. * ATOMMASS + KBOLTZ * body[iBody].dFlowTemp * body[iBody].dFHRef / (6 * BDIFF * g);
      }

    } else if ((body[iBody].iWaterLossModel == ATMESC_LBEXACT) | (body[iBody].iWaterLossModel == ATMESC_TIAN)) {

      double x = (QOH - 1.) * (1. - XO) * (BDIFF * g * ATOMMASS) / (KBOLTZ * body[iBody].dFlowTemp);
      double FH;
      double rat;

      // Get the crossover mass
      if (body[iBody].dFHRef < x) {

        // mcross < mo
        body[iBody].dCrossoverMass = ATOMMASS + (1. / (1. - XO)) * (KBOLTZ * body[iBody].dFlowTemp * body[iBody].dFHRef) / (BDIFF * g);
        FH = body[iBody].dFHRef;
        rat = (body[iBody].dCrossoverMass / ATOMMASS - QOH) / (body[iBody].dCrossoverMass / ATOMMASS - 1.);
        body[iBody].dOxygenEta = 0;

      } else {

        // mcross >= mo
        double num = 1. + (XO / (1. - XO)) * QOH * QOH;
        double den = 1. + (XO / (1. - XO)) * QOH;
        body[iBody].dCrossoverMass = ATOMMASS * num / den + (KBOLTZ * body[iBody].dFlowTemp * body[iBody].dFHRef) / ((1 + XO * (QOH - 1)) * BDIFF * g);
        rat = (body[iBody].dCrossoverMass / ATOMMASS - QOH) / (body[iBody].dCrossoverMass / ATOMMASS - 1.);
        FH = body[iBody].dFHRef * pow(1. + (XO / (1. - XO)) * QOH * rat, -1);
        body[iBody].dOxygenEta = 2 * XO / (1. - XO) * rat;

      }

    }

    if ((XO > 0.6) && (body[iBody].iWaterLossModel == ATMESC_LBEXACT)) {
      // Schaefer et al. (2016) prescription, section 2.2
      // NOTE: Perhaps a better criterion is (body[iBody].dOxygenEta > 1),
      // which ensures oxygen never escapes faster than it is being produced?
      body[iBody].iWaterEscapeRegime = ATMESC_DIFFLIM;
      body[iBody].dOxygenEta = 0;
      body[iBody].dMDotWater = body[iBody].dFHDiffLim * (4 * ATOMMASS * PI * body[iBody].dRadius * body[iBody].dRadius * body[iBody].dXFrac * body[iBody].dXFrac);
    } else {
      // In the Tian model, oxygen escapes when it's the dominant species. I think this is wrong...
      body[iBody].iWaterEscapeRegime = ATMESC_ELIM;
      body[iBody].dMDotWater = body[iBody].dFHRef * (4 * ATOMMASS * PI * body[iBody].dRadius * body[iBody].dRadius * body[iBody].dXFrac * body[iBody].dXFrac);
    }
  }

}


/**
Assigns functions returning the time-derivatives of each variable
to the magical matrix of function pointers.

@param body A pointer to the current BODY instance
@param evolve A pointer to the EVOLVE instance
@param update A pointer to the UPDATE instance
@param fnUpdate A triple-pointer to the function that updates each variable
@param iBody The current BODY number
*/
void AssignAtmEscDerivatives(BODY *body,EVOLVE *evolve,UPDATE *update,fnUpdateVariable ***fnUpdate,int iBody) {
  if (body[iBody].dSurfaceWaterMass > 0) {
    fnUpdate[iBody][update[iBody].iSurfaceWaterMass][0] = &fdDSurfaceWaterMassDt;
    fnUpdate[iBody][update[iBody].iOxygenMass][0]       = &fdDOxygenMassDt;
    fnUpdate[iBody][update[iBody].iOxygenMantleMass][0] = &fdDOxygenMantleMassDt;
  }
  if (body[iBody].dEnvelopeMass > 0) {
    fnUpdate[iBody][update[iBody].iEnvelopeMass][0]     = &fdDEnvelopeMassDt;
    fnUpdate[iBody][update[iBody].iMass][0]             = &fdDEnvelopeMassDt;
  }
  fnUpdate[iBody][update[iBody].iRadius][0]             = &fdPlanetRadius; // NOTE: This points to the VALUE of the radius!
}

/**
Assigns null functions to the magical matrix of function pointers
for variables that will not get updated.

@param body A pointer to the current BODY instance
@param evolve A pointer to the EVOLVE instance
@param update A pointer to the UPDATE instance
@param fnUpdate A triple-pointer to the function that updates each variable
@param iBody The current BODY number
*/
void NullAtmEscDerivatives(BODY *body,EVOLVE *evolve,UPDATE *update,fnUpdateVariable ***fnUpdate,int iBody) {
  if (body[iBody].dSurfaceWaterMass > 0) {
    fnUpdate[iBody][update[iBody].iSurfaceWaterMass][0] = &fndUpdateFunctionTiny;
    fnUpdate[iBody][update[iBody].iOxygenMass][0]       = &fndUpdateFunctionTiny;
    fnUpdate[iBody][update[iBody].iOxygenMantleMass][0] = &fndUpdateFunctionTiny;
  }
  if (body[iBody].dEnvelopeMass > 0) {
    fnUpdate[iBody][update[iBody].iEnvelopeMass][0]     = &fndUpdateFunctionTiny;
    fnUpdate[iBody][update[iBody].iMass][0]             = &fndUpdateFunctionTiny;
  }
  fnUpdate[iBody][update[iBody].iRadius][0]             = &fndUpdateFunctionTiny; // NOTE: This points to the VALUE of the radius!
}

/**
Verify all the inputs for the atmospheric escape module.

@param body A pointer to the current BODY instance
@param control A pointer to the CONTROL instance
@param files A pointer to the FILES instance
@param options A pointer to the OPTIONS instance
@param output A pointer to the OUTPUT instance
@param system A pointer to the SYSTEM instance
@param update A pointer to the UPDATE instance
@param iBody The current BODY number
@param iModule The current MODULE number
*/
void VerifyAtmEsc(BODY *body,CONTROL *control,FILES *files,OPTIONS *options,OUTPUT *output,SYSTEM *system,UPDATE *update,int iBody,int iModule) {
  int bAtmEsc=0;

  body[iBody].bEnvelopeLostMessage = 0;
  body[iBody].bRocheMessage = 0;

  // Is FXUV specified in input file?
  if (options[OPT_FXUV].iLine[iBody+1] > -1){
    body[iBody].bCalcFXUV = 0;
  }
  else{
    body[iBody].bCalcFXUV = 1;
  }

  if (body[iBody].iPlanetRadiusModel == ATMESC_LEHMER17) {
    body[iBody].dRadSolid = 1.3 * pow(body[iBody].dMass - body[iBody].dEnvelopeMass, 0.27);
    body[iBody].dGravAccel = BIGG * (body[iBody].dMass - body[iBody].dEnvelopeMass) / (body[iBody].dRadSolid * body[iBody].dRadSolid);
    body[iBody].dScaleHeight = body[iBody].dAtmGasConst * body[iBody].dThermTemp / body[iBody].dGravAccel;
    body[iBody].dPresSurf = fdLehmerPres(body[iBody].dEnvelopeMass, body[iBody].dGravAccel, body[iBody].dRadSolid);
    body[iBody].dRadXUV = fdLehmerRadius(body[iBody].dRadSolid, body[iBody].dPresXUV, body[iBody].dScaleHeight,body[iBody].dPresSurf);
  } else {
    int iCol,bError = 0;
    for (iCol=0;iCol<files->Outfile[iBody].iNumCols;iCol++) {
      if (memcmp(files->Outfile[iBody].caCol[iCol],output[OUT_PLANETRADXUV].cName,strlen(output[OUT_PLANETRADXUV].cName)) == 0) {
        /* Match! */
        fprintf(stderr,"ERROR: Cannot output %s for body %s while using AtmEsc's LOPEZ12 model.\n",
            output[OUT_PLANETRADXUV].cName,body[iBody].cName);
        bError=1;
      }
      if (memcmp(files->Outfile[iBody].caCol[iCol],output[OUT_RADSOLID].cName,strlen(output[OUT_RADSOLID].cName)) == 0) {
        /* Match! */
        fprintf(stderr,"ERROR: Cannot output %s for body %s while using AtmEsc's LOPEZ12 model.\n",
            output[OUT_RADSOLID].cName,body[iBody].cName);
        bError=1;
      }
      if (memcmp(files->Outfile[iBody].caCol[iCol],output[OUT_SCALEHEIGHT].cName,strlen(output[OUT_SCALEHEIGHT].cName)) == 0) {
        /* Match! */
        fprintf(stderr,"ERROR: Cannot output %s for body %s while using AtmEsc's LOPEZ12 model.\n",
            output[OUT_SCALEHEIGHT].cName,body[iBody].cName);
        bError=1;
      }
      if (memcmp(files->Outfile[iBody].caCol[iCol],output[OUT_PRESSURF].cName,strlen(output[OUT_PRESSURF].cName)) == 0) {
        /* Match! */
        fprintf(stderr,"ERROR: Cannot output %s for body %s while using AtmEsc's LOPEZ12 model.\n",
            output[OUT_PRESSURF].cName,body[iBody].cName);
        bError=1;
      }
    }

    if (bError) {
      LineExit(files->Infile[iBody+1].cIn,options[OPT_OUTPUTORDER].iLine[iBody+1]);
    }
    /* Must initialized the above values to avoid memory leaks. */
    body[iBody].dRadXUV = -1;
    body[iBody].dRadSolid = -1;
    body[iBody].dScaleHeight = -1;
    body[iBody].dPresSurf = -1;
  }

  if (body[iBody].dSurfaceWaterMass > 0) {
    VerifySurfaceWaterMass(body,options,update,body[iBody].dAge,iBody);
    VerifyOxygenMass(body,options,update,body[iBody].dAge,iBody);
    VerifyOxygenMantleMass(body,options,update,body[iBody].dAge,iBody);
    bAtmEsc = 1;
  }

  if (body[iBody].dEnvelopeMass > 0) {
    VerifyEnvelopeMass(body,options,update,body[iBody].dAge,iBody);
    VerifyMassAtmEsc(body,options,update,body[iBody].dAge,iBody);
    bAtmEsc = 1;
  }

  // Ensure envelope mass is physical
  if (body[iBody].dEnvelopeMass > body[iBody].dMass) {
    if (control->Io.iVerbose >= VERBERR)
      fprintf(stderr,"ERROR: %s cannot be greater than %s in file %s.\n",options[OPT_ENVELOPEMASS].cName,options[OPT_MASS].cName,files->Infile[iBody+1].cIn);
    exit(EXIT_INPUT);
  }

  // Initialize rg duration
  body[iBody].dRGDuration = 0.;

  if (!bAtmEsc && control->Io.iVerbose >= VERBINPUT)
    fprintf(stderr,"WARNING: AtmEsc called for body %s, but no atmosphere/water present!\n",body[iBody].cName);

  // Radius evolution
  if (update[iBody].iNumRadius > 1) {
    if (control->Io.iVerbose >= VERBERR)
      fprintf(stderr,"ERROR: More than one module is trying to set dRadius for body %d!", iBody);
    exit(EXIT_INPUT);
  }
  VerifyRadiusAtmEsc(body,control,options,update,body[iBody].dAge,iBody);

  control->fnForceBehavior[iBody][iModule] = &fnForceBehaviorAtmEsc;
  control->fnPropsAux[iBody][iModule] = &fnPropsAuxAtmEsc;
  control->Evolve.fnBodyCopy[iBody][iModule] = &BodyCopyAtmEsc;

}

/**************** ATMESC update ****************/

/**
Internal housekeeping function that determines
which variables get updated every time step.

@param body A pointer to the current BODY instance
@param update A pointer to the UPDATE instance
@param iBody The current BODY number
*/
void InitializeUpdateAtmEsc(BODY *body,UPDATE *update,int iBody) {
  if (body[iBody].dSurfaceWaterMass > 0) {
    if (update[iBody].iNumSurfaceWaterMass == 0)
      update[iBody].iNumVars++;
    update[iBody].iNumSurfaceWaterMass++;

    if (update[iBody].iNumOxygenMass == 0)
      update[iBody].iNumVars++;
    update[iBody].iNumOxygenMass++;

    if (update[iBody].iNumOxygenMantleMass == 0)
      update[iBody].iNumVars++;
    update[iBody].iNumOxygenMantleMass++;
  }

  if (body[iBody].dEnvelopeMass > 0) {
    if (update[iBody].iNumEnvelopeMass == 0)
      update[iBody].iNumVars++;
    update[iBody].iNumEnvelopeMass++;

    if (update[iBody].iNumMass == 0)
      update[iBody].iNumVars++;
    update[iBody].iNumMass++;
  }

  if (body[iBody].dRadius > 0) {
    if (update[iBody].iNumRadius == 0)
      update[iBody].iNumVars++;
    update[iBody].iNumRadius++;
  }

}

/**
Internal housekeeping function that determines
which variables get updated every time step.

@param body A pointer to the current BODY instance
@param update A pointer to the UPDATE instance
@param iEqn The current equation number
@param iVar The current variable number
@param iBody The current BODY number
@param iFoo ?!
*/
void FinalizeUpdateEccAtmEsc(BODY *body,UPDATE *update,int *iEqn,int iVar,int iBody,int iFoo) {
  /* Nothing */
}

/**
Internal housekeeping function that determines
which variables get updated every time step.

@param body A pointer to the current BODY instance
@param update A pointer to the UPDATE instance
@param iEqn The current equation number
@param iVar The current variable number
@param iBody The current BODY number
@param iFoo ?!
*/
void FinalizeUpdateSurfaceWaterMassAtmEsc(BODY *body,UPDATE*update,int *iEqn,int iVar,int iBody,int iFoo) {
  update[iBody].iaModule[iVar][*iEqn] = ATMESC;
  update[iBody].iNumSurfaceWaterMass = (*iEqn)++;
}

/**
Internal housekeeping function that determines
which variables get updated every time step.

@param body A pointer to the current BODY instance
@param update A pointer to the UPDATE instance
@param iEqn The current equation number
@param iVar The current variable number
@param iBody The current BODY number
@param iFoo ?!
*/
void FinalizeUpdateOxygenMassAtmEsc(BODY *body,UPDATE*update,int *iEqn,int iVar,int iBody,int iFoo) {
  update[iBody].iaModule[iVar][*iEqn] = ATMESC;
  update[iBody].iNumOxygenMass = (*iEqn)++;
}

/**
Internal housekeeping function that determines
which variables get updated every time step.

@param body A pointer to the current BODY instance
@param update A pointer to the UPDATE instance
@param iEqn The current equation number
@param iVar The current variable number
@param iBody The current BODY number
@param iFoo ?!
*/
void FinalizeUpdateOxygenMantleMassAtmEsc(BODY *body,UPDATE*update,int *iEqn,int iVar,int iBody,int iFoo) {
  update[iBody].iaModule[iVar][*iEqn] = ATMESC;
  update[iBody].iNumOxygenMantleMass = (*iEqn)++;
}

/**
Internal housekeeping function that determines
which variables get updated every time step.

@param body A pointer to the current BODY instance
@param update A pointer to the UPDATE instance
@param iEqn The current equation number
@param iVar The current variable number
@param iBody The current BODY number
@param iFoo ?!
*/
void FinalizeUpdateEnvelopeMassAtmEsc(BODY *body,UPDATE*update,int *iEqn,int iVar,int iBody,int iFoo) {
  update[iBody].iaModule[iVar][*iEqn] = ATMESC;
  update[iBody].iNumEnvelopeMass = (*iEqn)++;
}

/**
Internal housekeeping function that determines
which variables get updated every time step.

@param body A pointer to the current BODY instance
@param update A pointer to the UPDATE instance
@param iEqn The current equation number
@param iVar The current variable number
@param iBody The current BODY number
@param iFoo ?!
*/
void FinalizeUpdateMassAtmEsc(BODY *body,UPDATE*update,int *iEqn,int iVar,int iBody,int iFoo) {
  update[iBody].iaModule[iVar][*iEqn] = ATMESC;
  update[iBody].iNumMass = (*iEqn)++;
}

/**
Internal housekeeping function that determines
which variables get updated every time step.

@param body A pointer to the current BODY instance
@param update A pointer to the UPDATE instance
@param iEqn The current equation number
@param iVar The current variable number
@param iBody The current BODY number
@param iFoo ?!
*/
void FinalizeUpdateOblAtmEsc(BODY *body,UPDATE *update,int *iEqn,int iVar,int iBody,int iFoo) {
  /* Nothing */
}

/**
Internal housekeeping function that determines
which variables get updated every time step.

@param body A pointer to the current BODY instance
@param update A pointer to the UPDATE instance
@param iEqn The current equation number
@param iVar The current variable number
@param iBody The current BODY number
@param iFoo ?!
*/
void FinalizeUpdateRotAtmEsc(BODY *body,UPDATE *update,int *iEqn,int iVar,int iBody,int iFoo) {
  /* Nothing */
}

/**
Internal housekeeping function that determines
which variables get updated every time step.

@param body A pointer to the current BODY instance
@param update A pointer to the UPDATE instance
@param iEqn The current equation number
@param iVar The current variable number
@param iBody The current BODY number
@param iFoo ?!
*/
void FinalizeUpdateSemiAtmEsc(BODY *body,UPDATE *update,int *iEqn,int iVar,int iBody,int iFoo) {
  /* Nothing */
}

/**
Internal housekeeping function that determines
which variables get updated every time step.

@param body A pointer to the current BODY instance
@param update A pointer to the UPDATE instance
@param iEqn The current equation number
@param iVar The current variable number
@param iBody The current BODY number
@param iFoo ?!
*/
void FinalizeUpdateRadiusAtmEsc(BODY *body,UPDATE*update,int *iEqn,int iVar,int iBody,int iFoo) {
  update[iBody].iaModule[iVar][*iEqn] = ATMESC;
  update[iBody].iNumRadius = (*iEqn)++;
}

/***************** ATMESC Halts *****************/

/**
Checks for surface desiccation and halts if necessary.

@param body A pointer to the current BODY instance
@param evolve A pointer to the EVOLVE instance
@param halt A pointer to the HALT instance
@param io A pointer to the IO instance
@param update A pointer to the UPDATE instance
@param iBody The current BODY number
*/
int fbHaltSurfaceDesiccated(BODY *body,EVOLVE *evolve,HALT *halt,IO *io,UPDATE *update,int iBody) {

  if (body[iBody].dSurfaceWaterMass <= body[iBody].dMinSurfaceWaterMass) {
    if (io->iVerbose >= VERBPROG) {
      printf("HALT: %s's surface water mass =  ",body[iBody].cName);
      fprintd(stdout,body[iBody].dSurfaceWaterMass/TOMASS,io->iSciNot,io->iDigits);
      printf("TO.\n");
    }
    return 1;
  }
  return 0;
}

/**
Checks for envelope evaporation and halts if necessary.

@param body A pointer to the current BODY instance
@param evolve A pointer to the EVOLVE instance
@param halt A pointer to the HALT instance
@param io A pointer to the IO instance
@param update A pointer to the UPDATE instance
@param iBody The current BODY number
*/
int fbHaltEnvelopeGone(BODY *body,EVOLVE *evolve,HALT *halt,IO *io,UPDATE *update,int iBody) {

  if (body[iBody].dEnvelopeMass <= body[iBody].dMinEnvelopeMass) {
    if (io->iVerbose >= VERBPROG) {
      printf("HALT: %s's envelope mass =  ",body[iBody].cName);
      fprintd(stdout,body[iBody].dEnvelopeMass/MEARTH,io->iSciNot,io->iDigits);
      printf("Earth Masses.\n");
    }
    return 1;
  }
  return 0;
}

/**
Count the number of halting conditions.

@param halt A pointer to the HALT instance
@param iHalt The current HALT number
*/
void CountHaltsAtmEsc(HALT *halt,int *iHalt) {
  if (halt->bSurfaceDesiccated)
    (*iHalt)++;
  if (halt->bEnvelopeGone)
    (*iHalt)++;
}

/**
Check whether the user wants to halt on certain conditions.

@param body A pointer to the current BODY instance
@param control A pointer to the CONTROL instance
@param options A pointer to the OPTIONS instance
@param iBody The current BODY number
@param iHalt The current HALT number
*/
void VerifyHaltAtmEsc(BODY *body,CONTROL *control,OPTIONS *options,int iBody,int *iHalt) {

  if (control->Halt[iBody].bSurfaceDesiccated)
    control->fnHalt[iBody][(*iHalt)++] = &fbHaltSurfaceDesiccated;

  if (control->Halt[iBody].bEnvelopeGone)
    control->fnHalt[iBody][(*iHalt)++] = &fbHaltEnvelopeGone;
}

/************* ATMESC Outputs ******************/

/**
Logs the surface water mass.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteSurfaceWaterMass(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dSurfaceWaterMass;

  if (output->bDoNeg[iBody]) {
    *dTmp *= output->dNeg;
    strcpy(cUnit,output->cNeg);
  } else {
    *dTmp /= fdUnitsMass(units->iMass);
    fsUnitsMass(units->iMass,cUnit);
  }

}

/**
Logs the atmospheric oxygen mass.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteOxygenMass(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dOxygenMass;

  if (output->bDoNeg[iBody]) {
    *dTmp *= 1.e-5 * ((BIGG * body[iBody].dMass) / (4. * PI * pow(body[iBody].dRadius, 4)));
    strcpy(cUnit,output->cNeg);
  } else {
    *dTmp /= fdUnitsMass(units->iMass);
    fsUnitsMass(units->iMass,cUnit);
  }

}

/**
Logs the mantle oxygen mass.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteOxygenMantleMass(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dOxygenMantleMass;

  if (output->bDoNeg[iBody]) {
    *dTmp *= 1.e-5 * ((BIGG * body[iBody].dMass) / (4. * PI * pow(body[iBody].dRadius, 4)));
    strcpy(cUnit,output->cNeg);
  } else {
    *dTmp /= fdUnitsMass(units->iMass);
    fsUnitsMass(units->iMass,cUnit);
  }

}

/**
Logs the planet radius.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WritePlanetRadius(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dRadius;

  if (output->bDoNeg[iBody]) {
    *dTmp *= output->dNeg;
    strcpy(cUnit,output->cNeg);
  } else {
    *dTmp /= fdUnitsLength(units->iLength);
    fsUnitsLength(units->iLength,cUnit);
  }

}

/**
Logs the envelope mass.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteEnvelopeMass(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dEnvelopeMass;

  if (output->bDoNeg[iBody]) {
    *dTmp *= output->dNeg;
    strcpy(cUnit,output->cNeg);
  } else {
    *dTmp /= fdUnitsMass(units->iMass);
    fsUnitsMass(units->iMass,cUnit);
  }
}

/**
Logs the semi-major axis corresponding to the current runaway greenhouse limit.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteRGLimit(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {

  // Get the RG flux
  double flux = fdHZRG14(body,iBody);

  // Convert to semi-major axis *at current eccentricity!*
  *dTmp = pow(4 * PI * flux /  (body[0].dLuminosity * pow((1 - body[iBody].dEcc * body[iBody].dEcc), 0.5)), -0.5);

  if (output->bDoNeg[iBody]) {
    *dTmp *= output->dNeg;
    strcpy(cUnit,output->cNeg);
  } else {
    *dTmp /= fdUnitsLength(units->iLength);
    fsUnitsLength(units->iLength,cUnit);
  }
}

/**
Logs the oxygen mixing ratio at the base of the hydrodynamic wind.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteOxygenMixingRatio(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = fdAtomicOxygenMixingRatio(body[iBody].dSurfaceWaterMass, body[iBody].dOxygenMass);
  strcpy(cUnit,"");
}

/**
Logs the oxygen eta parameter from Luger and Barnes (2015).

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteOxygenEta(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dOxygenEta;
  strcpy(cUnit,"");
}

/**
Logs the XUV absorption efficiency for water.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteAtmXAbsEffH2O(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dAtmXAbsEffH2O;
  strcpy(cUnit,"");
}

/**
Logs the planet's radius in the XUV.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WritePlanetRadXUV(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dRadXUV;

  if (output->bDoNeg[iBody]) {
    *dTmp *= output->dNeg;
    strcpy(cUnit,output->cNeg);
  } else {
    *dTmp /= fdUnitsLength(units->iLength);
    fsUnitsLength(units->iLength,cUnit);
  }
}

/**
Logs the atmospheric mass loss rate.

\warning This routine is currently broken.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteDEnvMassDt(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]){
  double dDeriv;

  *dTmp = -1;
  /* BROKEN!!!!
  dDeriv = *(update[iBody].pdDEnvelopeMassDtAtmesc);
  *dTmp = dDeriv;
  *dTmp *= fdUnitsTime(units->iTime)/fdUnitsMass(units->iMass);
  */
}

/**
Logs the thermospheric temperature.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteThermTemp(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
 *dTmp = body[iBody].dThermTemp;

 if (output->bDoNeg[iBody]) {
   *dTmp *= output->dNeg;
   strcpy(cUnit,output->cNeg);
 } else { }
}

/**
Logs the temperature of the flow.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteFlowTemp(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
 *dTmp = body[iBody].dFlowTemp;

 if (output->bDoNeg[iBody]) {
   *dTmp *= output->dNeg;
   strcpy(cUnit,output->cNeg);
 } else { }
}

/**
Logs the surface pressure.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WritePresSurf(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dPresSurf;

  if (output->bDoNeg[iBody]){
    *dTmp *= output->dNeg;
    strcpy(cUnit,output->cNeg);
  } else { }
}

/**
Logs the pressure at the XUV absorption radius.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WritePresXUV(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dPresXUV;

  if (output->bDoNeg[iBody]){
    *dTmp *= output->dNeg;
    strcpy(cUnit,output->cNeg);
  } else { }
}

/**
Logs the time at which the flow transitioned to Jeans escape.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteJeansTime(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dJeansTime;

  if (output->bDoNeg[iBody]){
    *dTmp *= output->dNeg;
    strcpy(cUnit,output->cNeg);
  } else { }
}

/**
Logs the atmospheric scale height.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteScaleHeight(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dScaleHeight;

  if (output->bDoNeg[iBody]){
    *dTmp *= output->dNeg;
    strcpy(cUnit,output->cNeg);
  } else { }
}

/**
Logs the gas constant.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteAtmGasConst(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dAtmGasConst;

  if (output->bDoNeg[iBody]){
    *dTmp *= output->dNeg;
    strcpy(cUnit,output->cNeg);
  } else { }
}

/**
Logs the planet's solid radius.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteRadSolid(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dRadSolid;

  if (output->bDoNeg[iBody]) {
    *dTmp *= output->dNeg;
    strcpy(cUnit,output->cNeg);
  } else {
    *dTmp /= fdUnitsLength(units->iLength);
    fsUnitsLength(units->iLength,cUnit);
  }
}

/**
Logs the XUV flux received by the planet.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param units A pointer to the current UNITS instance
@param update A pointer to the current UPDATE instance
@param iBody The current body Number
@param dTmp Temporary variable used for unit conversions
@param cUnit The unit for this variable
*/
void WriteFXUV(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UNITS *units,UPDATE *update,int iBody,double *dTmp,char cUnit[]) {
  *dTmp = body[iBody].dFXUV;

  if (output->bDoNeg[iBody]){
    *dTmp *= output->dNeg;
    strcpy(cUnit,output->cNeg);
  } else {
    strcpy(cUnit,"W/m^2");
  }
}

/**
Set up stuff to be logged for atmesc.

@param output A pointer to the current OUTPUT instance
@param fnWrite A pointer to the function that does the logging
*/
void InitializeOutputAtmEsc(OUTPUT *output,fnWriteOutput fnWrite[]) {

  sprintf(output[OUT_SURFACEWATERMASS].cName,"SurfWaterMass");
  sprintf(output[OUT_SURFACEWATERMASS].cDescr,"Surface Water Mass");
  sprintf(output[OUT_SURFACEWATERMASS].cNeg,"TO");
  output[OUT_SURFACEWATERMASS].bNeg = 1;
  output[OUT_SURFACEWATERMASS].dNeg = 1./TOMASS;
  output[OUT_SURFACEWATERMASS].iNum = 1;
  output[OUT_SURFACEWATERMASS].iModuleBit = ATMESC;
  fnWrite[OUT_SURFACEWATERMASS] = &WriteSurfaceWaterMass;

  sprintf(output[OUT_PLANETRADIUS].cName,"PlanetRadius");
  sprintf(output[OUT_PLANETRADIUS].cDescr,"Planet Radius");
  sprintf(output[OUT_PLANETRADIUS].cNeg,"Earth Radii");
  output[OUT_PLANETRADIUS].bNeg = 1;
  output[OUT_PLANETRADIUS].dNeg = 1./REARTH;
  output[OUT_PLANETRADIUS].iNum = 1;
  output[OUT_PLANETRADIUS].iModuleBit = ATMESC;
  fnWrite[OUT_PLANETRADIUS] = &WritePlanetRadius;

  sprintf(output[OUT_OXYGENMASS].cName,"OxygenMass");
  sprintf(output[OUT_OXYGENMASS].cDescr,"Oxygen Mass");
  sprintf(output[OUT_OXYGENMASS].cNeg,"bars");
  output[OUT_OXYGENMASS].bNeg = 1;
  output[OUT_OXYGENMASS].dNeg = 1;
  output[OUT_OXYGENMASS].iNum = 1;
  output[OUT_OXYGENMASS].iModuleBit = ATMESC;
  fnWrite[OUT_OXYGENMASS] = &WriteOxygenMass;

  sprintf(output[OUT_OXYGENMANTLEMASS].cName,"OxygenMantleMass");
  sprintf(output[OUT_OXYGENMANTLEMASS].cDescr,"Mass of Oxygen in Mantle");
  sprintf(output[OUT_OXYGENMANTLEMASS].cNeg,"bars");
  output[OUT_OXYGENMANTLEMASS].bNeg = 1;
  output[OUT_OXYGENMANTLEMASS].dNeg = 1;
  output[OUT_OXYGENMANTLEMASS].iNum = 1;
  output[OUT_OXYGENMANTLEMASS].iModuleBit = ATMESC;
  fnWrite[OUT_OXYGENMANTLEMASS] = &WriteOxygenMantleMass;

  sprintf(output[OUT_RGLIMIT].cName,"RGLimit");
  sprintf(output[OUT_RGLIMIT].cDescr,"Runaway Greenhouse Semi-Major Axis");
  sprintf(output[OUT_RGLIMIT].cNeg,"AU");
  output[OUT_RGLIMIT].bNeg = 1;
  output[OUT_RGLIMIT].dNeg = 1. / AUM;
  output[OUT_RGLIMIT].iNum = 1;
  output[OUT_RGLIMIT].iModuleBit = ATMESC;
  fnWrite[OUT_RGLIMIT] = &WriteRGLimit;

  sprintf(output[OUT_XO].cName,"XO");
  sprintf(output[OUT_XO].cDescr,"Atomic Oxygen Mixing Ratio in Upper Atmosphere");
  output[OUT_XO].bNeg = 0;
  output[OUT_XO].iNum = 1;
  output[OUT_XO].iModuleBit = ATMESC;
  fnWrite[OUT_XO] = &WriteOxygenMixingRatio;

  sprintf(output[OUT_ETAO].cName,"EtaO");
  sprintf(output[OUT_ETAO].cDescr,"Oxygen Eta Parameter (Luger and Barnes 2015)");
  output[OUT_ETAO].bNeg = 0;
  output[OUT_ETAO].iNum = 1;
  output[OUT_ETAO].iModuleBit = ATMESC;
  fnWrite[OUT_ETAO] = &WriteOxygenEta;

  sprintf(output[OUT_EPSH2O].cName,"AtmXAbsEffH2O");
  sprintf(output[OUT_EPSH2O].cDescr,"XUV Atmospheric Escape Efficiency for H2O");
  output[OUT_EPSH2O].bNeg = 0;
  output[OUT_EPSH2O].iNum = 1;
  output[OUT_EPSH2O].iModuleBit = ATMESC;
  fnWrite[OUT_EPSH2O] = &WriteAtmXAbsEffH2O;

  sprintf(output[OUT_FXUV].cName,"XUVFlux");
  sprintf(output[OUT_FXUV].cDescr,"XUV Flux Incident on Planet");
  sprintf(output[OUT_FXUV].cNeg,"erg/cm^2/s");
  output[OUT_FXUV].dNeg = 1.e3;
  output[OUT_FXUV].bNeg = 1;
  output[OUT_FXUV].iNum = 1;
  output[OUT_FXUV].iModuleBit = ATMESC;
  fnWrite[OUT_FXUV] = &WriteFXUV;

  sprintf(output[OUT_ENVELOPEMASS].cName,"EnvelopeMass");
  sprintf(output[OUT_ENVELOPEMASS].cDescr,"Envelope Mass");
  sprintf(output[OUT_ENVELOPEMASS].cNeg,"Earth");
  output[OUT_ENVELOPEMASS].bNeg = 1;
  output[OUT_ENVELOPEMASS].dNeg = 1./MEARTH;
  output[OUT_ENVELOPEMASS].iNum = 1;
  output[OUT_ENVELOPEMASS].iModuleBit = ATMESC;
  fnWrite[OUT_ENVELOPEMASS] = &WriteEnvelopeMass;

  sprintf(output[OUT_PLANETRADXUV].cName,"RadXUV");
  sprintf(output[OUT_PLANETRADXUV].cDescr,"XUV Radius separating hydro. dyn. escape and equilibrium");
  sprintf(output[OUT_PLANETRADXUV].cNeg,"Earth Radii");
  output[OUT_PLANETRADXUV].bNeg = 1;
  output[OUT_PLANETRADXUV].dNeg = 1./REARTH;
  output[OUT_PLANETRADXUV].iNum = 1;
  output[OUT_PLANETRADXUV].iModuleBit = ATMESC;
  fnWrite[OUT_PLANETRADXUV] = &WritePlanetRadXUV;

  sprintf(output[OUT_DENVMASSDT].cName,"DEnvMassDt");
  sprintf(output[OUT_DENVMASSDT].cDescr,"Envelope Mass Loss Rate");
  sprintf(output[OUT_DENVMASSDT].cNeg,"kg/s");
  output[OUT_DENVMASSDT].bNeg = 1;
  output[OUT_DENVMASSDT].iNum = 1;
  output[OUT_DENVMASSDT].iModuleBit = ATMESC;
  fnWrite[OUT_DENVMASSDT] = &WriteDEnvMassDt;

  sprintf(output[OUT_THERMTEMP].cName,"ThermTemp");
  sprintf(output[OUT_THERMTEMP].cDescr,"Isothermal Atmospheric Temperature");
  sprintf(output[OUT_THERMTEMP].cNeg,"K");
  output[OUT_THERMTEMP].bNeg = 1;
  output[OUT_THERMTEMP].dNeg = 1; // default units are K.
  output[OUT_THERMTEMP].iNum = 1;
  output[OUT_THERMTEMP].iModuleBit = ATMESC;
  fnWrite[OUT_THERMTEMP] = &WriteThermTemp;

  sprintf(output[OUT_PRESSURF].cName,"PresSurf");
  sprintf(output[OUT_PRESSURF].cDescr,"Surface Pressure due to Atmosphere");
  sprintf(output[OUT_PRESSURF].cNeg,"Pa");
  output[OUT_PRESSURF].bNeg = 1;
  output[OUT_PRESSURF].dNeg = 1;
  output[OUT_PRESSURF].iNum = 1;
  output[OUT_PRESSURF].iModuleBit = ATMESC;
  fnWrite[OUT_PRESSURF] = &WritePresSurf;

  sprintf(output[OUT_PRESXUV].cName,"PresXUV");
  sprintf(output[OUT_PRESXUV].cDescr,"Pressure at base of Thermosphere");
  sprintf(output[OUT_PRESXUV].cNeg,"Pa");
  output[OUT_PRESXUV].bNeg = 1;
  output[OUT_PRESXUV].dNeg = 1;
  output[OUT_PRESXUV].iNum = 1;
  output[OUT_PRESXUV].iModuleBit = ATMESC;
  fnWrite[OUT_PRESXUV] = &WritePresXUV;

  sprintf(output[OUT_SCALEHEIGHT].cName,"ScaleHeight");
  sprintf(output[OUT_SCALEHEIGHT].cDescr,"Scaling factor in fdLehmerRadius");
  sprintf(output[OUT_SCALEHEIGHT].cNeg,"J s^2 / kg m");
  output[OUT_SCALEHEIGHT].bNeg = 1;
  output[OUT_SCALEHEIGHT].dNeg = 1;
  output[OUT_SCALEHEIGHT].iNum = 1;
  output[OUT_SCALEHEIGHT].iModuleBit = ATMESC;
  fnWrite[OUT_SCALEHEIGHT] = &WriteScaleHeight;

  sprintf(output[OUT_ATMGASCONST].cName,"AtmGasConst");
  sprintf(output[OUT_ATMGASCONST].cDescr,"Atmospheric Gas Constant");
  sprintf(output[OUT_ATMGASCONST].cNeg,"J / K kg");
  output[OUT_ATMGASCONST].bNeg = 1;
  output[OUT_ATMGASCONST].dNeg = 1;
  output[OUT_ATMGASCONST].iNum = 1;
  output[OUT_ATMGASCONST].iModuleBit = ATMESC;
  fnWrite[OUT_ATMGASCONST] = &WriteAtmGasConst;

  sprintf(output[OUT_RADSOLID].cName,"RadSolid");
  sprintf(output[OUT_RADSOLID].cDescr,"Radius to the solid surface");
  sprintf(output[OUT_RADSOLID].cNeg,"Earth Radii");
  output[OUT_RADSOLID].bNeg = 1;
  output[OUT_RADSOLID].dNeg = 1./REARTH;
  output[OUT_RADSOLID].iNum = 1;
  output[OUT_RADSOLID].iModuleBit = ATMESC;
  fnWrite[OUT_RADSOLID] = &WriteRadSolid;

  sprintf(output[OUT_FXUV].cName,"FXUV");
  sprintf(output[OUT_FXUV].cDescr,"XUV Flux");
  sprintf(output[OUT_FXUV].cNeg,"W/m^2");
  output[OUT_FXUV].bNeg = 1;
  output[OUT_FXUV].dNeg = 1;
  output[OUT_FXUV].iNum = 1;
  output[OUT_FXUV].iModuleBit = ATMESC;
  fnWrite[OUT_FXUV] = &WriteFXUV;

}

/************ ATMESC Logging Functions **************/

/**
Log the global atmesc options.

\warning This routine currently does nothing!

@param control A pointer to the current CONTROL instance
@param fp A FILE pointer
*/
void LogOptionsAtmEsc(CONTROL *control, FILE *fp) {

  /* Anything here?

  fprintf(fp,"-------- ATMESC Options -----\n\n");
  */
}

/**
Log the global atmesc parameters.

\warning This routine currently does nothing!

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param update A pointer to the current UPDATE instance
@param fnWrite A pointer to the function doing the logging
@param fp A FILE pointer
*/
void LogAtmEsc(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UPDATE *update,fnWriteOutput fnWrite[],FILE *fp) {

  /* Anything here?
  int iOut;

  fprintf(fp,"\n----- ATMESC PARAMETERS ------\n");
  for (iOut=OUTSTARTATMESC;iOut<OUTBODYSTARTATMESC;iOut++) {
    if (output[iOut].iNum > 0)
      WriteLogEntry(control,output[iOut],body,system,fnWrite[iOut],fp,update,0);
  }
  */
}

/**
Log the body-specific atmesc parameters.

@param body A pointer to the current BODY instance
@param control A pointer to the current CONTROL instance
@param output A pointer to the current OUTPUT instance
@param system A pointer to the current SYSTEM instance
@param update A pointer to the current UPDATE instance
@param fnWrite A pointer to the function doing the logging
@param fp A FILE pointer
@param iBody The current BODY number
*/
void LogBodyAtmEsc(BODY *body,CONTROL *control,OUTPUT *output,SYSTEM *system,UPDATE *update,fnWriteOutput fnWrite[],FILE *fp,int iBody) {
  int iOut;
  fprintf(fp,"----- ATMESC PARAMETERS (%s)------\n",body[iBody].cName);

  for (iOut=OUTSTARTATMESC;iOut<OUTENDATMESC;iOut++) {
    if (output[iOut].iNum > 0) {
      WriteLogEntry(body,control,&output[iOut],system,update,fnWrite[iOut],fp,iBody);
    }
  }

  // TODO: Log this the standard way
  fprintf(fp,"(RGDuration) Runaway Greenhouse Duration [years]: %.5e\n", body[iBody].dRGDuration / YEARSEC);

}

/**
Adds atmesc to the current array of MODULEs.

@param module A pointer to the current array of MODULE instances
@param iBody The current BODY number
@param iModule The current MODULE number
*/
void AddModuleAtmEsc(CONTROL *control,MODULE *module,int iBody,int iModule) {

  module->iaModule[iBody][iModule]                         = ATMESC;

  module->fnCountHalts[iBody][iModule]                     = &CountHaltsAtmEsc;
  module->fnReadOptions[iBody][iModule]                    = &ReadOptionsAtmEsc;
  module->fnLogBody[iBody][iModule]                        = &LogBodyAtmEsc;
  module->fnVerify[iBody][iModule]                         = &VerifyAtmEsc;
  module->fnAssignDerivatives[iBody][iModule]              = &AssignAtmEscDerivatives;
  module->fnNullDerivatives[iBody][iModule]                = &NullAtmEscDerivatives;
  module->fnVerifyHalt[iBody][iModule]                     = &VerifyHaltAtmEsc;

  module->fnInitializeUpdate[iBody][iModule]               = &InitializeUpdateAtmEsc;
  module->fnFinalizeUpdateSurfaceWaterMass[iBody][iModule] = &FinalizeUpdateSurfaceWaterMassAtmEsc;
  module->fnFinalizeUpdateOxygenMass[iBody][iModule]       = &FinalizeUpdateOxygenMassAtmEsc;
  module->fnFinalizeUpdateOxygenMantleMass[iBody][iModule] = &FinalizeUpdateOxygenMantleMassAtmEsc;
  module->fnFinalizeUpdateEnvelopeMass[iBody][iModule]     = &FinalizeUpdateEnvelopeMassAtmEsc;
  module->fnFinalizeUpdateMass[iBody][iModule]             = &FinalizeUpdateEnvelopeMassAtmEsc;
  module->fnFinalizeUpdateRadius[iBody][iModule]           = &FinalizeUpdateRadiusAtmEsc;
}

/************* ATMESC Functions ************/

/**
The rate of change of the surface water mass.

@param body A pointer to the current BODY instance
@param system A pointer to the current SYSTEM instance
@param iaBody An array of body indices. The current body is index 0.
*/
double fdDSurfaceWaterMassDt(BODY *body,SYSTEM *system,int *iaBody) {

  if ((body[iaBody[0]].bRunaway) && (body[iaBody[0]].dSurfaceWaterMass > 0)) {

    // This takes care of both energy-limited and diffusion limited escape!
    return -(9. / (1 + 8 * body[iaBody[0]].dOxygenEta)) * body[iaBody[0]].dMDotWater;

  } else {

    return 0.;

  }
}

/**
The rate of change of the oxygen mass in the atmosphere.

@param body A pointer to the current BODY instance
@param system A pointer to the current SYSTEM instance
@param iaBody An array of body indices. The current body is index 0.
*/
double fdDOxygenMassDt(BODY *body,SYSTEM *system,int *iaBody) {

  if ((body[iaBody[0]].bRunaway) && (!body[iaBody[0]].bInstantO2Sink) && (body[iaBody[0]].dSurfaceWaterMass > 0)) {

    if (body[iaBody[0]].iWaterLossModel == ATMESC_LB15) {

      // Rodrigo and Barnes (2015)
      if (body[iaBody[0]].dCrossoverMass >= 16 * ATOMMASS) {
        double BDIFF = 4.8e19 * pow(body[iaBody[0]].dFlowTemp, 0.75);
        return (320. * PI * BIGG * ATOMMASS * ATOMMASS * BDIFF * body[iaBody[0]].dMass) / (KBOLTZ * body[iaBody[0]].dFlowTemp);
      } else {
        return (8 - 8 * body[iaBody[0]].dOxygenEta) / (1 + 8 * body[iaBody[0]].dOxygenEta) * body[iaBody[0]].dMDotWater;
      }
    } else {

      // Exact
      return (8 - 8 * body[iaBody[0]].dOxygenEta) / (1 + 8 * body[iaBody[0]].dOxygenEta) * body[iaBody[0]].dMDotWater;

    }
  } else {

    return 0.;

  }

}

/**
The rate of change of the oxygen mass in the mantle.

@param body A pointer to the current BODY instance
@param system A pointer to the current SYSTEM instance
@param iaBody An array of body indices. The current body is index 0.
*/
double fdDOxygenMantleMassDt(BODY *body,SYSTEM *system,int *iaBody) {

  if ((body[iaBody[0]].bRunaway) && (body[iaBody[0]].bInstantO2Sink) && (body[iaBody[0]].dSurfaceWaterMass > 0)) {

    if (body[iaBody[0]].iWaterLossModel == ATMESC_LB15) {

      // Rodrigo and Barnes (2015)
      if (body[iaBody[0]].dCrossoverMass >= 16 * ATOMMASS) {
        double BDIFF = 4.8e19 * pow(body[iaBody[0]].dFlowTemp, 0.75);
        return (320. * PI * BIGG * ATOMMASS * ATOMMASS * BDIFF * body[iaBody[0]].dMass) / (KBOLTZ * body[iaBody[0]].dFlowTemp);
      } else {
        return (8 - 8 * body[iaBody[0]].dOxygenEta) / (1 + 8 * body[iaBody[0]].dOxygenEta) * body[iaBody[0]].dMDotWater;
      }
    } else {

      // Exact
      return (8 - 8 * body[iaBody[0]].dOxygenEta) / (1 + 8 * body[iaBody[0]].dOxygenEta) * body[iaBody[0]].dMDotWater;

    }
  } else {

    return 0.;

  }

}

/**
The rate of change of the envelope mass.

@param body A pointer to the current BODY instance
@param system A pointer to the current SYSTEM instance
@param iaBody An array of body indices. The current body is index 0.
*/
double fdDEnvelopeMassDt(BODY *body,SYSTEM *system,int *iaBody) {

  // TODO: This needs to be moved. Ideally we'd just remove this equation from the matrix.
  // RB: move to ForceBehaviorAtmesc
  if ((body[iaBody[0]].dEnvelopeMass <= 0) || (body[iaBody[0]].dAge > body[iaBody[0]].dJeansTime)) {
    return dTINY;
  }

  if (body[iaBody[0]].iPlanetRadiusModel == ATMESC_LEHMER17) {

  	return -body[iaBody[0]].dAtmXAbsEffH * PI * body[iaBody[0]].dFXUV
        * pow(body[iaBody[0]].dRadXUV, 3.0) / ( BIGG * (body[iaBody[0]].dMass
          - body[iaBody[0]].dEnvelopeMass));

  } else {
  	return -body[iaBody[0]].dFHRef * (body[iaBody[0]].dAtmXAbsEffH
      / body[iaBody[0]].dAtmXAbsEffH2O) * (4 * ATOMMASS * PI
        * body[iaBody[0]].dRadius * body[iaBody[0]].dRadius
          * body[iaBody[0]].dXFrac * body[iaBody[0]].dXFrac);
  }

}

/**
This function does nothing in atmesc.

@param body A pointer to the current BODY instance
@param system A pointer to the current SYSTEM instance
@param update A pointer to the current UPDATE instance
@param iBody The current body index
@param iFoo An example of pretty lousy programming
*/
double fdSurfEnFluxAtmEsc(BODY *body,SYSTEM *system,UPDATE *update,int iBody,int iFoo) {
  // This is silly, but necessary!
  return 0;
}


/**
Returns the planet radius at the current time.

@param body A pointer to the current BODY instance
@param system A pointer to the current SYSTEM instance
@param iaBody An array of body indices. The current body is index 0.
*/
double fdPlanetRadius(BODY *body,SYSTEM *system,int *iaBody) {

  if (body[iaBody[0]].iPlanetRadiusModel == ATMESC_LEHMER17) {
    body[iaBody[0]].dPresSurf = fdLehmerPres(body[iaBody[0]].dEnvelopeMass, body[iaBody[0]].dGravAccel, body[iaBody[0]].dRadSolid);
    body[iaBody[0]].dRadXUV = fdLehmerRadius(body[iaBody[0]].dRadSolid, body[iaBody[0]].dPresXUV, body[iaBody[0]].dScaleHeight,body[iaBody[0]].dPresSurf);
  }

  double foo;
  if (body[iaBody[0]].iPlanetRadiusModel == ATMESC_LOP12) {
    // If no envelope, return solid body radius according to Sotin+2007 model
    if(body[iaBody[0]].dEnvelopeMass <= body[iaBody[0]].dMinEnvelopeMass) {
      foo = fdMassToRad_Sotin07(body[iaBody[0]].dMass);
    }
    // Envelope present: estimate planetary radius using Lopez models
    else {
      foo = fdLopezRadius(body[iaBody[0]].dMass, body[iaBody[0]].dEnvelopeMass / body[iaBody[0]].dMass, 1., body[iaBody[0]].dAge, 0);
    }
    if (!isnan(foo))
      return foo;
    else
      return body[iaBody[0]].dRadius;
  } else if (body[iaBody[0]].iPlanetRadiusModel == ATMESC_PROXCENB) {
    return fdProximaCenBRadius(body[iaBody[0]].dEnvelopeMass / body[iaBody[0]].dMass, body[iaBody[0]].dAge, body[iaBody[0]].dMass);
  } else
    return body[iaBody[0]].dRadius;


}

/************* ATMESC Helper Functions ************/

/**
Computes whether or not water is escaping.

@param body A pointer to the current BODY instance
@param iBody The current BODY index
*/
int fbDoesWaterEscape(BODY *body, int iBody) {
  // TODO: The checks below need to be moved. Ideally we'd
  // just remove this equation from the matrix if the
  // escape conditions are not met.

  // 1. Check if there's hydrogen to be lost; this happens first
  if (body[iBody].dEnvelopeMass > 0) {
    // (But let's still check whether the RG phase has ended)
    if ((body[iBody].dRGDuration == 0.) && (fdInstellation(body, iBody) < fdHZRG14(body,iBody)))
      body[iBody].dRGDuration = body[iBody].dAge;
    return 0;
  }

  // 2. Check if planet is beyond RG limit; otherwise, assume the
  // cold trap prevents water loss.
  // NOTE: The RG flux limit below is calculated based on body zero's
  // spectrum! The Kopparapu+14 limit is for a single star only. This
  // approximation for a binary is only valid if the two stars have
  // similar spectral types, or if body zero dominates the flux.
  else if (fdInstellation(body, iBody) < fdHZRG14(body,iBody)) {
    if (body[iBody].dRGDuration == 0.)
      body[iBody].dRGDuration = body[iBody].dAge;
    return 0;
  }

  // 3. Is there still water to be lost?
  else if (body[iBody].dSurfaceWaterMass <= 0)
    return 0;

  // 4. Are we in the ballistic (Jeans) escape limit?
  else if (body[iBody].dAge > body[iBody].dJeansTime)
    return 0;

  else
    return 1;

}

/**
Computes the atomic oxygen mixing ratio in the hydrodynamic flow.

@param dSurfaceWaterMass The amount of water in the atmosphere
@param dOxygenMass The amount of oxygen in the atmosphere
*/
double fdAtomicOxygenMixingRatio(double dSurfaceWaterMass, double dOxygenMass) {
  // Mixing ratio X_O of atomic oxygen in the upper atmosphere
  // assuming atmosphere is well-mixed up to the photolysis layer
  double NO2 = dOxygenMass / (32 * ATOMMASS);
  double NH2O = dSurfaceWaterMass / (18 * ATOMMASS);
  if (NH2O > 0)
    return 1. / (1 + 1. / (0.5 + NO2 / NH2O));
  else {
    if (NO2 > 0)
      return 1.;
    else
      return 0.;
  }
}

/**
Performs a simple log-linear fit to the Kopparapu et al. (2014) mass-dependent
runaway greenhouse limit.

\warning  Something is wrong with this linear fit in the first 5 Myr or so, as it diverges.

@param dLuminosity The stellar luminosity
@param dTeff The stellar effective temperature
@param dEcc The planet's eccentricity -- RB: Why is ecc passed?
@param dPlanetMass The planet mass
*/

// Why isn't this in system.c?
double fdHZRG14(BODY *body,int iBody) {
  //double dLuminosity, double dTeff, double dEcc, double dPlanetMass) {
  // Do a simple log-linear fit to the Kopparapu+14 mass-dependent RG limit
  int i;
  double seff[3];
  double daCoeffs[2];
  double dHZRG14Limit;

  double tstar = body[0].dTemperature - 5780;
  double daLogMP[3] = {-1.0, 0., 0.69897};
  double seffsun[3] = {0.99, 1.107, 1.188};
  double a[3] = {1.209e-4, 1.332e-4, 1.433e-4};
  double b[3] = {1.404e-8, 1.58e-8, 1.707e-8};
  double c[3] = {-7.418e-12, -8.308e-12, -8.968e-12};
  double d[3] = {-1.713e-15, -1.931e-15, -2.084e-15};

  for (i=0;i<3;i++) {
  	seff[i] = seffsun[i] + a[i]*tstar + b[i]*tstar*tstar + c[i]*pow(tstar,3) + d[i]*pow(tstar,4);
  }

  fvLinearFit(daLogMP,seff,3,daCoeffs);

  dHZRG14Limit =  (daCoeffs[0]*log10(body[iBody].dMass/MEARTH) + daCoeffs[1]) * LSUN / (4 * PI * AUM * AUM);

  return dHZRG14Limit;
}

/**
Computes the XUV absorption efficiency for a water vapor atmosphere
based on a fit to the figure in Bolmont et al. (2017).

@param dFXUV The XUV flux incident on the planet.
*/
double fdXUVEfficiencyBolmont2016(double dFXUV) {

  // Based on a piecewise polynomial fit to Figure 2
  // in Bolmont et al. (2017), the XUV escape efficiency
  // as a function of XUV flux for the TRAPPIST-1 planets.

  // Polynomial coefficients
  double a0 = 1.49202;
  double a1 = 5.57875;
  double a2 = 2.27482;
  double b0 = 0.59182134;
  double b1 = -0.36140798;
  double b2 = -0.04011933;
  double b3 = -0.8988;
  double c0 = -0.00441536;
  double c1 = -0.03068399;
  double c2 = 0.04946948;
  double c3 = -0.89880083;

  // Convert to erg/cm^2/s and take the log
  double x = log10(dFXUV * 1.e3);
  double y;

  // Piecewise polynomial fit
  if ((x >= -2) && (x < -1))
    y = pow(10, a0 * x * x + a1 * x + a2);
  else if ((x >= -1) && (x < 0))
    y = pow(10, b0 * x * x * x + b1 * x * x + b2 * x + b3);
  else if ((x >= 0) && (x <= 5))
    y = pow(10, c0 * x * x * x + c1 * x * x + c2 * x + c3);
  else
    y = 0;
  return y;

}

/**
Performs a really simple linear least-squares fit on data.

@param x The independent coordinates
@param y The dependent coordinates
@param iLen The length of the arrays
@param daCoeffs The slope and the intercept of the fit
*/

// should be in another file. control?
void fvLinearFit(double *x, double *y, int iLen, double *daCoeffs){
	// Simple least squares linear regression, y(x) = mx + b
	// from http://en.wikipedia.org/wiki/Simple_linear_regression
	double num = 0, den = 0;
	double xavg = 0,yavg = 0;
	double m,b;
	int i;
	for (i=0;i<iLen;i++){
		xavg += x[i];
		yavg += y[i];
	}
	xavg /= iLen;
	yavg /= iLen;
	for (i=0;i<iLen;i++){
		num += (x[i]-xavg)*(y[i]-yavg);
		den += (x[i]-xavg)*(x[i]-xavg);
	}
	daCoeffs[0] = num/den;									// Slope
	daCoeffs[1] = yavg-daCoeffs[0]*xavg;		// Intercept
}
