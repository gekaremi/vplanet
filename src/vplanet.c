/**
  @file vplanet.c
  @brief The main entry point for the code. All the magic starts here.
  @author Rory Barnes ([RoryBarnes](https://github.com/RoryBarnes/))
  @date May 7 2014
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "vplanet.h"
#include <sys/time.h>

/*
#ifdef DEBUG
#define _GNU_SOURCE
#include <fenv.h>
#endif
*/

#include <xmmintrin.h>

/* Do not change these values */
const double dHUGE = DBL_MAX; // This is the largest possible double value according to <float.h>
const double dTINY = 1./DBL_MAX; // This is the smallest possibled double value according to <float.h>
/* Do not change these values */

/*! \brief Main function. All the magic happens here!
 */
int main(int argc,char *argv[]) {
#ifdef DEBUG
  //  feenableexcept(FE_INVALID | FE_OVERFLOW);
  _MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() & ~_MM_MASK_INVALID);
#endif

  struct timeval start, end;

  gettimeofday(&start, NULL);

//  time_t dStartTime;
  //dStartTime = time(NULL);

  int iOption,iVerbose,iQuiet,iOverwrite;
  OPTIONS *options;
  OUTPUT *output;
  CONTROL control;
  UPDATE *update;
  BODY *body;
  MODULE module;
  FILES files;
  SYSTEM system;
  char infile[NAMELEN];
  fnReadOption fnRead[MODULEOPTEND]; // XXX Pointers?
  fnWriteOutput fnWrite[MODULEOUTEND];
  fnUpdateVariable ***fnUpdate;
  fnIntegrate fnOneStep;

  #ifdef GITVERSION
  strcpy(control.sGitVersion,GITVERSION);
  #else
  FILE *fp;
  char version[64];

  /* Open the command for reading. */
  fp = popen("git describe --tags --abbrev=40 --always", "r");
  if (fp == NULL) {
  	printf("Failed to run git command\n" );
  	exit(1);
  }

  /* Read the output a line at a time - output it. */
  fgets(version, sizeof(version)-1, fp);
  strcpy(control.sGitVersion,version);

  pclose(fp);

  //exit(1);
  #endif

  /** Must initialize all options and outputs for all modules
     independent of what is selected. This allows a complete
     help screen as well as checks during ReadOptions. This
     also requires the only modifications outside of module.c:
     Add explicit references to all modules at the end of
     options.c, output.c and util.c.
  */
  options = malloc(MODULEOPTEND*sizeof(OPTIONS));
  InitializeOptions(options,fnRead);
  output = malloc(MODULEOUTEND*sizeof(OUTPUT));
  InitializeOutput(output,fnWrite);

  /* Set to IntegrationMethod to 0, so default can be
     assigned if necessary */
  control.Evolve.iOneStep = 0;

  /* Copy executable file name to the files struct. */
  strcpy(files.cExe,argv[0]);

  if (argc == 1) {
    fprintf(stderr,"ERROR: Incorrect number of arguments. Usage: %s [-verbose] [-help] file [file].\n",argv[0]);
    exit(EXIT_EXE);
  }

  iVerbose=-1;
  iQuiet=-1;
  iOverwrite=-1;
  control.Io.iVerbose=-1;
  control.Io.bOverwrite=-1;

  /* Check for flags */
  for (iOption=1;iOption<argc;iOption++) {
    if (memcmp(argv[iOption],"-v",2) == 0) {
      control.Io.iVerbose = 5;
      iVerbose=iOption;
    }
    if (memcmp(argv[iOption],"-q",2) == 0) {
      control.Io.iVerbose = 0;
      iQuiet=iOption;
    }
    if (memcmp(argv[iOption],"-f",2) == 0) {
      control.Io.bOverwrite = 1;
      iOverwrite=iOption;
    }
    if (memcmp(argv[iOption],"-h",2) == 0)
      Help(options,output,files.cExe);
    if (memcmp(argv[iOption],"-H",2) == 0)
      LongHelp(options,output,files.cExe);
  }

  if (iQuiet != -1 && iVerbose != -1) {
    fprintf(stderr,"ERROR: -v and -q cannot be set simultaneously.\n");
    exit(EXIT_EXE);
  }

  /* Now identify input file, usually vpl.in */
  for (iOption=1;iOption<argc;iOption++) {
    if (iOption != iVerbose && iOption != iQuiet && iOption != iOverwrite)
      strcpy(infile,argv[iOption]);
  }

  /* Read input files */
  ReadOptions(&body,&control,&files,&module,options,output,&system,&update,fnRead,infile);

  if (control.Io.iVerbose >= VERBINPUT)
    printf("Input files read.\n");

  /* Check that user options are mutually compatible */
  VerifyOptions(body,&control,&files,&module,options,output,&system,update,&fnOneStep,&fnUpdate);

  if (control.Io.iVerbose >= VERBINPUT)
    printf("Input files verified.\n");

  control.Evolve.dTime=0;
  control.Evolve.bFirstStep=1;

  if (control.Io.bLog) {
    WriteLog(body,&control,&files,&module,options,output,&system,update,fnUpdate,fnWrite,0);
    if (control.Io.iVerbose >= VERBPROG)
      printf("Log file written.\n");
  }

  /* Perform evolution */

  if (control.Evolve.bDoForward || control.Evolve.bDoBackward) {
    Evolve(body,&control,&files,&module,output,&system,update,fnUpdate,fnWrite,fnOneStep);

    /* If evolution performed, log final system parameters */
    if (control.Io.bLog) {
      WriteLog(body,&control,&files,&module,options,output,&system,update,fnUpdate,fnWrite,1);
      if (control.Io.iVerbose >= VERBPROG)
	      printf("Log file updated.\n");
    }
  }

  gettimeofday(&end, NULL);

  if (control.Io.iVerbose >= VERBPROG) {
    printf("Simulation completed.\n");
    //printf("Total time: %.4e [sec]\n", difftime(end.tv_usec,start.tv_usec)/1e6);
  }
  exit(0);

}
