/**
  @file evolve.c

  @brief This file contains all the core VPLANET integration routines including the
         timestepping algorithm and the Runge-Kutta Integration scheme.

  @author Rory Barnes ([RoryBarnes](https://github.com/RoryBarnes/))

  @date May 2014

*/

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include "vplanet.h"

void PropsAuxGeneral(BODY *body,CONTROL *control) {
/* Recompute the mean motion, necessary for most modules */
  int iBody; // Dummy counting variable

  for (iBody=0;iBody<control->Evolve.iNumBodies;iBody++) {
    if (iBody != 0 && body[iBody].bBinary == 0) {
      body[iBody].dMeanMotion = fdSemiToMeanMotion(body[iBody].dSemi,(body[0].dMass+body[iBody].dMass));
    }
  }
}

void PropertiesAuxiliary(BODY *body,CONTROL *control,UPDATE *update) {
/* Evaluate single and multi-module auxialliary functions to update parameters
 * of interest such as mean motion.
 */
  int iBody,iModule; // Dummy counter variables

  PropsAuxGeneral(body,control);

  /* Get properties from each module */
  for (iBody=0;iBody<control->Evolve.iNumBodies;iBody++) {
    // Uni-module properties
    for (iModule=0;iModule<control->Evolve.iNumModules[iBody];iModule++)
      control->fnPropsAux[iBody][iModule](body,&control->Evolve,&control->Io,update,iBody);

    // Multi-module properties
    for (iModule=0;iModule<control->iNumMultiProps[iBody];iModule++)
      control->fnPropsAuxMulti[iBody][iModule](body,&control->Evolve,&control->Io,update,iBody);
  }
}

/*
 * Integration Control
 */

double AssignDt(double dMin,double dNextOutput,double dEta) {
/* Compute the next timestep, dt, making sure it's not larger than the output
 * cadence */

  dMin = dEta * dMin;
  if (dNextOutput < dMin)
  {
    dMin = dNextOutput;
  }
  return dMin;
}

double fdNextOutput(double dTime,double dOutputInterval) {
/* Compute when the next timestep occurs. */
  int nSteps; // Number of outputs so far

  /* Number of output so far */
  nSteps = (int)(dTime/dOutputInterval);
  /* Next output is one more */
  return (nSteps+1.0)*dOutputInterval;
}

double fdGetTimeStep(BODY *body,CONTROL *control,SYSTEM *system,UPDATE *update,fnUpdateVariable ***fnUpdate) {
  /* Fills the Update arrays with the derivatives
   * or new values. It returns the smallest timescale for use
   * in variable timestepping. Uses either a 4th order Runge-Kutte integrator or
   * an Euler step.
   */

    int iBody,iVar,iEqn; // Dummy counting variables
    EVOLVE integr; // Dummy EVOLVE struct so we don't have to dereference control a lot
    double dVarNow,dMinNow,dMin=dHUGE,dVarTotal,dTimeOut; // Intermediate storage variables

    integr = control->Evolve;


    // XXX Change Eqn to Proc?

    dMin = dHUGE;

    for (iBody=0;iBody<control->Evolve.iNumBodies;iBody++) {
      if (update[iBody].iNumVars > 0) {
        for (iVar=0;iVar<update[iBody].iNumVars;iVar++) {

          // The parameter does not require a derivative, but is calculated explicitly as a function of age.
          /*
          printf("%d %d\n",iBody,iVar);
          fflush(stdout);
          */
  	      if (update[iBody].iaType[iVar][0] == 0) {
  	        dVarNow = *update[iBody].pdVar[iVar];
  	        for (iEqn=0;iEqn<update[iBody].iNumEqns[iVar];iEqn++) {
  	          update[iBody].daDerivProc[iVar][iEqn] =
                fnUpdate[iBody][iVar][iEqn](body,system,update[iBody].iaBody[iVar][iEqn]);
  	        }
  	        if (control->Evolve.bFirstStep) {
  	          dMin = integr.dTimeStep;
  	          control->Evolve.bFirstStep = 0;
  	        } else {
  	          /* Sum over all equations giving new value of the variable */
  	          dVarTotal = 0.;
  	          for (iEqn=0;iEqn<update[iBody].iNumEqns[iVar];iEqn++) {
  	            dVarTotal += update[iBody].daDerivProc[iVar][iEqn];
  	          }
  	          // Prevent division by zero
  	          if (dVarNow != dVarTotal) {
  	            dMinNow = fabs(dVarNow/((dVarNow - dVarTotal)/integr.dTimeStep));
  	            if (dMinNow < dMin)
  		            dMin = dMinNow;
  	           }
  	         }
          /* Equations that are integrated in the matrix but are NOT allowed to dictate
             timestepping.  These are derived quantities, like lost energy, that must
             be integrated as primary variables to keep track of them properly, i.e.
             lost energy depends on changing radii, which are integrated.  But in this
             case, since they are derived quantities, they should NOT participate in
             timestep selection - dflemin3
           */
           } else if (update[iBody].iaType[iVar][0] == 5) {
             //continue;
             for (iEqn=0;iEqn<update[iBody].iNumEqns[iVar];iEqn++) {
               update[iBody].daDerivProc[iVar][iEqn] =
                 fnUpdate[iBody][iVar][iEqn](body,system,update[iBody].iaBody[iVar][iEqn]);
             }

           /* Integration for binary, where parameters can be computed via derivatives,
           or as an explicit function of age */
           } else if (update[iBody].iaType[iVar][0] == 10) {
           /* Equations not in matrix, computing things as explicit function of time,
             so we set dMin to time until next output
             Figure out time until next output */
             dMinNow = fdNextOutput(control->Evolve.dTime,control->Io.dOutputTime);
             if (dMinNow < dMin) {
               dMin = dMinNow;
             }

           /* The parameter does not require a derivative, but is calculated
  	          explicitly as a function of age and is a sinusoidal quantity
  	          (e.g. h,k,p,q in DistOrb) */
  	       } else if (update[iBody].iaType[iVar][0] == 3) {
  	          dVarNow = *update[iBody].pdVar[iVar];
  	          for (iEqn=0;iEqn<update[iBody].iNumEqns[iVar];iEqn++) {
  	             update[iBody].daDerivProc[iVar][iEqn] = fnUpdate[iBody][iVar][iEqn](body,system,update[iBody].iaBody[iVar][iEqn]);
  	          }
  	          if (control->Evolve.bFirstStep) {
  	             dMin = integr.dTimeStep;
  	             control->Evolve.bFirstStep = 0;
  	          } else {
  	            /* Sum over all equations giving new value of the variable */
  	            dVarTotal = 0.;
  	            for (iEqn=0;iEqn<update[iBody].iNumEqns[iVar];iEqn++) {
  	              dVarTotal += update[iBody].daDerivProc[iVar][iEqn];
  	            }
  	            // Prevent division by zero
  	            if (dVarNow != dVarTotal) {
  	              dMinNow = fabs(1.0/((dVarNow - dVarTotal)/integr.dTimeStep));
  	              if (dMinNow < dMin)
  		              dMin = dMinNow;
  	            }
  	          }

  	       /* The parameter is a "polar/sinusoidal quantity" and
  	          controlled by a time derivative */
  	       } else {
  	         for (iEqn=0;iEqn<update[iBody].iNumEqns[iVar];iEqn++) {
  	           if (update[iBody].iaType[iVar][iEqn] == 2) {
  	             update[iBody].daDerivProc[iVar][iEqn] = fnUpdate[iBody][iVar][iEqn](body,system,update[iBody].iaBody[iVar][iEqn]);
  	             //if (update[iBody].daDerivProc[iVar][iEqn] != 0 && *(update[iBody].pdVar[iVar]) != 0) {
  	             if (update[iBody].daDerivProc[iVar][iEqn] != 0) {
  		             /* ?Obl require special treatment because they can
  		                overconstrain obliquity and PrecA */
  		             if (iVar == update[iBody].iXobl || iVar == update[iBody].iYobl || iVar == update[iBody].iZobl) {
  		                if (body[iBody].dObliquity != 0) {
  		                  dMinNow = fabs(sin(body[iBody].dObliquity)/update[iBody].daDerivProc[iVar][iEqn]);
                      } else { // Obliquity is 0, so its evolution shouldn't impact the timestep
                        dMinNow = dHUGE;
                      }
  		             } else if (iVar == update[iBody].iHecc || iVar == update[iBody].iKecc) {
  		                if (body[iBody].dEcc != 0) {
  		                  dMinNow = fabs(body[iBody].dEcc/update[iBody].daDerivProc[iVar][iEqn]);
                      } else { // Eccentricity is 0, so its evolution shouldn't impact the timestep
                        dMinNow = dHUGE;
                      }
  		             } else {
  		                dMinNow = fabs(1.0/update[iBody].daDerivProc[iVar][iEqn]);
  		             }
  		             if (dMinNow < dMin) {
  		               dMin = dMinNow;
                   }
  	             }

  	           // enforce a minimum step size for ice sheets, otherwise dDt -> 0 real fast
  	           } else if (update[iBody].iaType[iVar][iEqn] == 9) {
  	              update[iBody].daDerivProc[iVar][iEqn] = fnUpdate[iBody][iVar][iEqn](body,system,update[iBody].iaBody[iVar][iEqn]);
  	              if (update[iBody].daDerivProc[iVar][iEqn] != 0 && *(update[iBody].pdVar[iVar]) != 0) {
  		               dMinNow = fabs((*(update[iBody].pdVar[iVar]))/update[iBody].daDerivProc[iVar][iEqn]);
  		               if (dMinNow < dMin) {
  		                 if (dMinNow < control->Halt[iBody].iMinIceDt*(2*PI/body[iBody].dMeanMotion)/control->Evolve.dEta) {
  		                    dMin = control->Halt[iBody].iMinIceDt*(2*PI/body[iBody].dMeanMotion)/control->Evolve.dEta;
  		                 } else {
  		                    dMin = dMinNow;
  		                 }
  		               }
  	              }

               // SpiNBody timestep: semi-temporary hack XXX
               // dt = r^2/v^2
               // r: Position vector
               // v: Velocity vector
               // Inefficient?
               } else if (update[iBody].iaType[iVar][iEqn] == 7) {
                 if ( (control->Evolve.bSpiNBodyDistOrb==0) || (control->Evolve.bUsingSpiNBody==1) ) {
                   update[iBody].daDerivProc[iVar][iEqn] = fnUpdate[iBody][iVar][iEqn](body,system,update[iBody].iaBody[iVar][iEqn]);
                   dMinNow = sqrt((body[iBody].dPositionX*body[iBody].dPositionX+body[iBody].dPositionY*body[iBody].dPositionY+body[iBody].dPositionZ*body[iBody].dPositionZ)
                      /(body[iBody].dVelX*body[iBody].dVelX+body[iBody].dVelY*body[iBody].dVelY+body[iBody].dVelZ*body[iBody].dVelZ));
                   if (dMinNow < dMin)
                     dMin = dMinNow;
                 }
               } else {
  	             // The parameter is controlled by a time derivative
  	             update[iBody].daDerivProc[iVar][iEqn] = fnUpdate[iBody][iVar][iEqn](body,system,update[iBody].iaBody[iVar][iEqn]);
                 if (!bFloatComparison(update[iBody].daDerivProc[iVar][iEqn],0.0) && !bFloatComparison(*(update[iBody].pdVar[iVar]),0.0)) {
  	      	        dMinNow = fabs((*(update[iBody].pdVar[iVar]))/update[iBody].daDerivProc[iVar][iEqn]);
  		              if (dMinNow < dMin)
  		                dMin = dMinNow;
  	             }
  	           }
  	         } // for loop
  	       } // else polar/sinusoidal
        } // for iNumVars
      } // if (update[iBody].iNumVars > 0)
    } // for loop iNumBodies

    return dMin;
}


void fdGetUpdateInfo(BODY *body,CONTROL *control,SYSTEM *system,UPDATE *update,fnUpdateVariable ***fnUpdate) {
/* Fills the Update arrays with the derivatives
 * or new values..
 */

  int iBody,iVar,iEqn,iNumBodies,iNumVars,iNumEqns; // Dummy counting variables
  EVOLVE integr; // Dummy EVOLVE struct so we don't have to dereference control a lot
  double dVarNow,dMinNow,dMin=dHUGE,dVarTotal; // Intermediate storage variables

  integr = control->Evolve;

  // XXX Change Eqn to Proc?
  iNumBodies = control->Evolve.iNumBodies;
  for (iBody=0;iBody<iNumBodies;iBody++) {
    if (update[iBody].iNumVars > 0) {
      iNumVars = update[iBody].iNumVars;
      for (iVar=0;iVar<iNumVars;iVar++) {
        iNumEqns = update[iBody].iNumEqns[iVar];
        for (iEqn=0;iEqn<iNumEqns;iEqn++) {
          update[iBody].daDerivProc[iVar][iEqn] = fnUpdate[iBody][iVar][iEqn](body,system,update[iBody].iaBody[iVar][iEqn]);
        }
      }
    }
  }
}


void EulerStep(BODY *body,CONTROL *control,SYSTEM *system,UPDATE *update,fnUpdateVariable ***fnUpdate,double *dDt,int iDir) {
/* Compute and apply an Euler update step to a given parameter (x = dx/dt * dt) */
  int iBody,iVar,iEqn;
  double dTimeOut,dFoo;

  /* Adjust dt? */
  if (control->Evolve.bVarDt) {
    dTimeOut = fdNextOutput(control->Evolve.dTime,control->Io.dOutputTime);
    /* This is minimum dynamical timescale */
    *dDt = fdGetTimeStep(body,control,system,update,fnUpdate);
    *dDt = AssignDt(*dDt,(dTimeOut - control->Evolve.dTime),control->Evolve.dEta);
  }

  for (iBody=0;iBody<control->Evolve.iNumBodies;iBody++) {
    for (iVar=0;iVar<update[iBody].iNumVars;iVar++) {
      for (iEqn=0;iEqn<update[iBody].iNumEqns[iVar];iEqn++) {
        if (update[iBody].iaType[iVar][iEqn] == 0)
          /* XXX This looks broken */
          *(update[iBody].pdVar[iVar]) = update[iBody].daDerivProc[iVar][iEqn];
        else {
          /* Update the parameter in the BODY struct! Be careful! */
          *(update[iBody].pdVar[iVar]) += iDir*update[iBody].daDerivProc[iVar][iEqn]*(*dDt);
        }
      }
    }
  }
}

void RungeKutta4Step(BODY *body,CONTROL *control,SYSTEM *system,UPDATE *update,fnUpdateVariable ***fnUpdate,double *dDt,int iDir) {
/* Compute and apply a 4th order Runge-Kutta update step a given parameter. */
  int iBody,iVar,iEqn,iSubStep,iNumBodies,iNumVars,iNumEqns;
  double dTimeOut,dFoo,dDelta;

  EVOLVE *evolve = &(control->Evolve); // Save Evolve as a variable for speed and legibility

  /* Create a copy of BODY array */
  BodyCopy(evolve->tmpBody,body,&control->Evolve);

  /* Verify that rotation angles behave correctly in an eqtide-only run
  if (evolve->tmpBody[1].dPrecA != 0)
    printf("PrecA = %e\n",evolve->tmpBody[1].dPrecA);
  XXX
  */

  /* Derivatives at start */
  //*dDt = fdGetTimeStep(body,control,system,evolve->tmpUpdate,fnUpdate);
  //UpdateCopy(update,control->Evolve.tmpUpdate,control->Evolve.iNumBodies);
  *dDt = fdGetTimeStep(body,control,system,control->Evolve.tmpUpdate,fnUpdate);
  //UpdateCopy(update,control->Evolve.tmpUpdate,control->Evolve.iNumBodies);
  //fdGetUpdateInfo(body,control,system,update,fnUpdate);

  /* Adjust dt? */
  if (evolve->bVarDt) {
     dTimeOut = fdNextOutput(evolve->dTime,control->Io.dOutputTime);
     /*  This is minimum dynamical timescale */
     *dDt = AssignDt(*dDt,(dTimeOut - evolve->dTime),evolve->dEta);
  } else {
    *dDt = evolve->dTimeStep;
  }

  evolve->dCurrentDt = *dDt;

  /* XXX Should each eqn be updated separately? Each parameter at a
     midpoint is moved by all the modules operating on it together.
     Does RK4 require the equations to be independent over the full step? */

  iNumBodies = evolve->iNumBodies;

  for (iBody=0;iBody<iNumBodies;iBody++) {
    iNumVars = update[iBody].iNumVars;
    for (iVar=0;iVar<iNumVars;iVar++) {
      evolve->daDeriv[0][iBody][iVar] = 0;
      iNumEqns = update[iBody].iNumEqns[iVar];
      for (iEqn=0;iEqn<iNumEqns;iEqn++) {
        // XXX Set update.dDxDtModule here?
        evolve->daDeriv[0][iBody][iVar] += iDir*evolve->tmpUpdate[iBody].daDerivProc[iVar][iEqn];
        //evolve->daTmpVal[0][iBody][iVar] += (*dDt)*iDir*evolve->tmpUpdate[iBody].daDeriv[iVar][iEqn];
      }

      if (update[iBody].iaType[iVar][0] == 0 || update[iBody].iaType[iVar][0] == 3 || update[iBody].iaType[iVar][0] == 10){
        // LUGER: Note that this is the VALUE of the variable getting passed, contrary to what the names suggest
        // These values are updated in the tmpUpdate struct so that equations which are dependent upon them will be
        // evaluated with higher accuracy
        *(evolve->tmpUpdate[iBody].pdVar[iVar]) = evolve->daDeriv[0][iBody][iVar];
      } else {
        /* While we're in this loop, move each parameter to the midpoint of the timestep */
        *(evolve->tmpUpdate[iBody].pdVar[iVar]) = *(update[iBody].pdVar[iVar]) + 0.5*(*dDt)*evolve->daDeriv[0][iBody][iVar];
      }
    }
  }

  /* First midpoint derivative.*/
  PropertiesAuxiliary(evolve->tmpBody,control,update);

  fdGetUpdateInfo(evolve->tmpBody,control,system,evolve->tmpUpdate,fnUpdate);

  for (iBody=0;iBody<iNumBodies;iBody++) {
    iNumVars = update[iBody].iNumVars;
    for (iVar=0;iVar<iNumVars;iVar++) {
      evolve->daDeriv[1][iBody][iVar] = 0;
      iNumEqns = update[iBody].iNumEqns[iVar];
      for (iEqn=0;iEqn<iNumEqns;iEqn++) {
        evolve->daDeriv[1][iBody][iVar] += iDir*evolve->tmpUpdate[iBody].daDerivProc[iVar][iEqn];
      }

      if (update[iBody].iaType[iVar][0] == 0 || update[iBody].iaType[iVar][0] == 3 || update[iBody].iaType[iVar][0] == 10){
        // LUGER: Note that this is the VALUE of the variable getting passed, contrary to what the names suggest
        // These values are updated in the tmpUpdate struct so that equations which are dependent upon them will be
        // evaluated with higher accuracy
        *(evolve->tmpUpdate[iBody].pdVar[iVar]) = evolve->daDeriv[1][iBody][iVar];
      } else {
        /* While we're in this loop, move each parameter to the midpoint
        of the timestep based on the midpoint derivative. */
        *(evolve->tmpUpdate[iBody].pdVar[iVar]) = *(update[iBody].pdVar[iVar]) + 0.5*(*dDt)*evolve->daDeriv[1][iBody][iVar];
      }
    }
  }

  /* Second midpoint derivative */
  PropertiesAuxiliary(evolve->tmpBody,control,update);

  fdGetUpdateInfo(evolve->tmpBody,control,system,evolve->tmpUpdate,fnUpdate);

  for (iBody=0;iBody<iNumBodies;iBody++) {
    iNumVars = update[iBody].iNumVars;
    for (iVar=0;iVar<iNumVars;iVar++) {
      evolve->daDeriv[2][iBody][iVar] = 0;
      iNumEqns = update[iBody].iNumEqns[iVar];
      for (iEqn=0;iEqn<iNumEqns;iEqn++) {
        evolve->daDeriv[2][iBody][iVar] += iDir*evolve->tmpUpdate[iBody].daDerivProc[iVar][iEqn];
      }

      if (update[iBody].iaType[iVar][0] == 0 || update[iBody].iaType[iVar][0] == 3 || update[iBody].iaType[iVar][0] == 10){
        // LUGER: Note that this is the VALUE of the variable getting passed, contrary to what the names suggest
        // These values are updated in the tmpUpdate struct so that equations which are dependent upon them will be
        // evaluated with higher accuracy
        *(evolve->tmpUpdate[iBody].pdVar[iVar]) = evolve->daDeriv[2][iBody][iVar];
      } else {
        /* While we're in this loop, move each parameter to the end of
        the timestep based on the second midpoint derivative. */
        *(evolve->tmpUpdate[iBody].pdVar[iVar]) = *(update[iBody].pdVar[iVar]) + *dDt*evolve->daDeriv[2][iBody][iVar];
      }
    }
  }

  /* Full step derivative */
  PropertiesAuxiliary(evolve->tmpBody,control,update);

  fdGetUpdateInfo(evolve->tmpBody,control,system,evolve->tmpUpdate,fnUpdate);

  for (iBody=0;iBody<iNumBodies;iBody++) {
    iNumVars = update[iBody].iNumVars;
    for (iVar=0;iVar<iNumVars;iVar++) {

      if (update[iBody].iaType[iVar][0] == 0 || update[iBody].iaType[iVar][0] == 3 || update[iBody].iaType[iVar][0] == 10){
        // NOTHING!
      } else {
        evolve->daDeriv[3][iBody][iVar] = 0;
        iNumEqns = update[iBody].iNumEqns[iVar];
        for (iEqn=0;iEqn<iNumEqns;iEqn++) {
          evolve->daDeriv[3][iBody][iVar] += iDir*evolve->tmpUpdate[iBody].daDerivProc[iVar][iEqn];
        }
      }
    }
  }
  /* Now do the update -- Note the pointer to the home of the actual variables!!! */
  for (iBody=0;iBody<iNumBodies;iBody++) {
    iNumVars = update[iBody].iNumVars;
    for (iVar=0;iVar<iNumVars;iVar++) {
      update[iBody].daDeriv[iVar] = 1./6*(evolve->daDeriv[0][iBody][iVar] + 2*evolve->daDeriv[1][iBody][iVar] +
      2*evolve->daDeriv[2][iBody][iVar] + evolve->daDeriv[3][iBody][iVar]);

      if (update[iBody].iaType[iVar][0] == 0 || update[iBody].iaType[iVar][0] == 3 || update[iBody].iaType[iVar][0] == 10){
        // LUGER: Note that this is the VALUE of the variable getting passed, contrary to what the names suggest
        *(update[iBody].pdVar[iVar]) = evolve->daDeriv[0][iBody][iVar];
      } else {
        *(update[iBody].pdVar[iVar]) += update[iBody].daDeriv[iVar]*(*dDt);
      }
    }
  }
}

/*
 * Evolution Subroutine
 */

void Evolve(BODY *body,CONTROL *control,FILES *files,MODULE *module,OUTPUT *output,SYSTEM *system,UPDATE *update,fnUpdateVariable ***fnUpdate,fnWriteOutput *fnWrite,fnIntegrate fnOneStep) {
/* Master evolution routine that controls the simulation integration. */
  int iDir,iBody,iModule,nSteps; // Dummy counting variables
  double dTimeOut; // When to output next
  double dDt,dFoo; // Next timestep, dummy variable
  double dEqSpinRate; // Store the equilibrium spin rate

  control->Evolve.nSteps=0;
  nSteps=0;

  if (control->Evolve.bDoForward)
    iDir=1;
  else
    iDir=-1;

  dTimeOut = fdNextOutput(control->Evolve.dTime,control->Io.dOutputTime);

  PropertiesAuxiliary(body,control,update);

  // Get derivatives at start, useful for logging
  dDt = fdGetTimeStep(body,control,system,update,fnUpdate);

  /* Adjust dt? */
  if (control->Evolve.bVarDt) {
    /* Get time to next output */
    dTimeOut = fdNextOutput(control->Evolve.dTime,control->Io.dOutputTime);
    /* Now choose the correct timestep */
    dDt = AssignDt(dDt,(dTimeOut - control->Evolve.dTime),control->Evolve.dEta);
  } else
      dDt = control->Evolve.dTimeStep;

  /* Write out initial conditions */

  WriteOutput(body,control,files,output,system,update,fnWrite,control->Evolve.dTime,dDt);

  /* If Runge-Kutta need to copy actual update to that in
     control->Evolve. This transfer all the meta-data about the
     struct. */
  UpdateCopy(control->Evolve.tmpUpdate,update,control->Evolve.iNumBodies);

  /*
   *
   * Main loop begins here
   *
   */

  while (control->Evolve.dTime < control->Evolve.dStopTime) {
    /* Take one step */
    fnOneStep(body,control,system,update,fnUpdate,&dDt,iDir);

    for (iBody=0;iBody<control->Evolve.iNumBodies;iBody++) {
      for (iModule=0;iModule<control->Evolve.iNumModules[iBody];iModule++)
        control->fnForceBehavior[iBody][iModule](body,module,&control->Evolve,&control->Io,system,update,fnUpdate,iBody,iModule);

      for (iModule=0;iModule<control->iNumMultiForce[iBody];iModule++)
        control->fnForceBehaviorMulti[iBody][iModule](body,module,&control->Evolve,&control->Io,system,update,fnUpdate,iBody,iModule);
    }

    fdGetUpdateInfo(body,control,system,update,fnUpdate);

    /* Halt? */
    if (fbCheckHalt(body,control,update)) {
      /* Use dummy variable as dDt is used for the integration.
       * Here we just want the instantaneous derivatives.
       * This should make the output self-consistent.
       */
      fdGetUpdateInfo(body,control,system,update,fnUpdate);
      WriteOutput(body,control,files,output,system,update,fnWrite,control->Evolve.dTime,control->Io.dOutputTime/control->Evolve.nSteps);
      return;
    }

    for (iBody=0;iBody<control->Evolve.iNumBodies;iBody++)
      body[iBody].dAge += iDir*dDt;

    control->Evolve.dTime += dDt;
    nSteps++;

    /* Time for Output? */
    if (control->Evolve.dTime >= dTimeOut) {
      //fdGetUpdateInfo(body,control,system,update,fnUpdate);
      WriteOutput(body,control,files,output,system,update,fnWrite,control->Evolve.dTime,control->Io.dOutputTime/control->Evolve.nSteps);
      dTimeOut = fdNextOutput(control->Evolve.dTime,control->Io.dOutputTime);
      control->Evolve.nSteps += nSteps;
      nSteps=0;
    }

    /* Get auxiliary properties for next step -- first call
       was prior to loop. */
    PropertiesAuxiliary(body,control,update);

    // If control->Evolve.bFirstStep hasn't been switched off by now, do so.
    if (control->Evolve.bFirstStep) {
      control->Evolve.bFirstStep = 0;
    }
  }

  if (control->Io.iVerbose >= VERBPROG)
    printf("Evolution completed.\n");
//     printf("%d\n",body[1].iBadImpulse);
}
