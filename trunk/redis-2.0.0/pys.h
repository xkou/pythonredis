
#ifndef _PYS_H_
#define _PYS_H_

#include<Python.h>

#define REDIS_PYOBJ  5

typedef struct TimerSt{
	PyObject *fun;
	PyObject *args;
	PyObject *kw;
} TimerSt;

extern PyObject * g_pyo_enc_rule;
int initPyVM();
char * _pyo_encode( PyObject*, PyObject*, int * );
PyObject *_pyo_decode( char *, PyObject * );
 PyObject *pyo_set_object( PyObject *, PyObject * );
 int pysave();
 PyObject *pyget( char *, int );
#endif


