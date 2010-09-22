
#ifndef _PYS_H_
#define _PYS_H_

#include<Python.h>

#define REDIS_PYOBJ  5

typedef struct TimerSt{
	PyObject *fun;
	PyObject *args;
	PyObject *kw;
} TimerSt;

typedef struct PyObjectConn{
	PyObject_HEAD;
	int fd;
	int server;
	PyObject * protocol;
	PyObject * proto_lost;
	PyObject * proto_recv;
	char * buffer;
	unsigned int bufferlen;
}PyObjectConn;

static char *g_pybuff;
static int g_pybufflen;

extern PyObject * g_pyo_enc_rule;
int initPyVM();
char * _pyo_encode( PyObject*, PyObject*, int * );
PyObject *_pyo_decode( char *, PyObject * );
PyObject *pyo_set_object( PyObject *, PyObject * );
PyObject *PyObjectConn_New( int fd );
int pysave();
int pysend( int fd,  char * buf, int len );
PyObject *pyget( char *, int );
#endif


