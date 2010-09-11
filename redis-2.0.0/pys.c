
#include "pys.h"

char *g_pyenstr = NULL;
int g_pyenstrlen = 0;

typedef struct _Cls_Def{
	char **atts;
	char *fakename;
} Cls_Def;

PyObject **g_pyo_dicts;

static char * pyenstrnew( ){
	
	
	char* s = PyMem_Malloc( g_pyenstrlen + 1024 );
	memcpy( s, g_pyenstr, g_pyenstrlen );
	PyMem_Free( g_pyenstr );
	g_pyenstr = s;
	g_pyenstrlen += 1024;	
	return g_pyenstr;
}

static void pyo_init(){
	int l = 10;
	g_pyo_dicts = PyMem_Malloc( l * sizeof (PyObject *) );
	for( int i=0; i < l; i++){
		g_pyo_dicts[i] = PyDict_New();
	}
	
	g_pyenstrlen = 1024;
	g_pyenstr = PyMem_Malloc( g_pyenstrlen );

}

static PyObject* pyo_def_enc( PyObject* self, PyObject* args ){
	long lid;
	PyObject* cls;
	PyObject* atts = NULL;
	char* fakename = NULL;
//	self;

	if( !PyArg_ParseTuple( args, "IO|Os", &lid, &cls, &atts, &fakename ) ){
		return NULL;
	}
	
	if( lid >= 10 ) return NULL;
	PyObject *dict = g_pyo_dicts[lid];
	PyObject *v = PyDict_GetItem(dict, cls );
	if( v )  return NULL;
	
	Cls_Def * pdef = PyMem_Malloc( sizeof (Cls_Def)  );
	pdef->atts = NULL;
	pdef->fakename = NULL;

	if( atts != NULL){
		Py_ssize_t ss= PyList_Size( atts );
		pdef->atts = PyMem_Malloc( ss * (sizeof( char* ) +1 ));
		pdef->atts[ss] = NULL;
		for( int i=0; i < ss; i++ ){
			char *pname = PyString_AS_STRING(	PyList_GetItem( atts, i ) );
			if( pname ){
				pdef->atts[i] = strdup( pname );
			}
		}

	}

	PyDict_SetItem( dict, cls, PyCObject_FromVoidPtr( pdef, NULL ));
	
}

#define SEP "\x10"

int pyo_inter_encode( PyObject* pyo, int offset ){
	int l = 1;
	if( pyo == Py_None ) g_pyenstr[offset] = ',';
	else if( Py_True  == pyo ) g_pyenstr[offset] = '>';
	else if( Py_False == pyo ) g_pyenstr[offset] = '<';
	else if( PyNumber_Check( pyo ) ){
		
	}

	return l;
}

int pyo_encode( PyObject * pyo ){
	

}


void initPyVM(){
	Py_Initialize();
	pyo_init();
}



