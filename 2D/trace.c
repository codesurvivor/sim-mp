#include <stdio.h>

main()
{
  FILE *fp;
  long i, numelts, rnoise, bnoise, cnoise, unst, st, calib, refc, refr, refb;
  long intb, intc, intr, j;
  long interval;
  long cyc[200002],ref[200002],br[200002];

  fprintf(stderr,"Starting\n");
  fp=fopen("tr1","r");
  fscanf(fp,"%ld",&numelts);
  for (i=0;i<numelts;i++) {
    fscanf(fp,"%d %d %d",&cyc[i],&ref[i],&br[i]);
  }
  fprintf(stderr,"Read values\n");
  for (interval=1; interval < numelts; interval=interval*2) {
    rnoise = interval*500;
    bnoise = interval*200;
    cnoise = 15;
    unst=0;
    st=0;
    calib=1;
    refc=0;
    refr=0;
    refb=0;
    for (j=0;j<interval;j++) {
      refc += cyc[j];
      refr += ref[j];
      refb += br[j];
    }
    i=j;
    while ((i+2*interval)<numelts) {
      intc=0;
      intr=0;
      intb=0;
      for (j=0;j<interval;j++,i++) {
	intc += cyc[i];
	intr += ref[i];
	intb += br[i];
      }
      if ((labs(intb-refb) > bnoise) || (labs(intr-refr) > rnoise) /* || (((labs(intc-refc)*100)/refc) > cnoise)*/  ) {
	unst++;
	refc=0;
	refr=0;
	refb=0;
	calib++;
        for (j=0;j<interval;j++,i++) {
          refc += cyc[i];
          refr += ref[i];
          refb += br[i];
        }
      }
      else {
	st++;
      }
    }
    fprintf(stderr,"Interval size: %ldK, ",interval*10);
    fprintf(stderr,"Calib %d Unstable %d Stable %d, Stable perc %d\n",calib, unst, st, (st*100)/(st+unst+calib));
  }
}
