#ifndef DSM_H
#define DSM_H
void initializeDSM(int ismaster, char * masterip, int mport, char *otherip, int oport, int numpagestoalloc);
void * getsharedregion();
#endif
