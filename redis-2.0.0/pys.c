#include <stdlib.h>
#include "pys.h"

char *g_pyenstr = NULL;
int g_pyenstrlen = 0;

typedef struct _Cls_Def{
	char **atts;
	char *fakename;
} Cls_Def;

typedef struct _Cls_Ref_Info{
	int pos;
	int no;
} Cls_Ref_Info;
int g_enc_count ;
PyObject * g_enc_refs;

PyObject **g_pyo_dicts;

static char * pyenstrnew( ){
	printf("#### renewstr" );	
	
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
	
	g_pyenstrlen = 10240;
	g_pyenstr = PyMem_Malloc( g_pyenstrlen );

	g_enc_refs = PyDict_New();

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
#define SEP2 '\x10'

int pyo_inter_encode( PyObject* pyo, char *cpt, int offset ){
	int l = 1;
	static char constr[1024];
	if( pyo == Py_None ) *cpt = ',';
	else if( Py_True  == pyo ) *cpt = '>';
	else if( Py_False == pyo ) *cpt = '<';
	else if( PyInt_Check( pyo ) ){
		if( g_pyenstrlen - offset < 1024 ) pyenstrnew();
//		int v = PyInt_AS_LONG( pyo );
//		l = snprintf( cpt, 1000, "%ld", v );
		PyObject * o = PyObject_Str( pyo );
		char *s; int ssize;
		PyString_AsStringAndSize( o, &s, &ssize );
		l = ssize;
		memcpy( cpt, s, l );
		Py_XDECREF( o );
	}
	else if( PyLong_CheckExact( pyo ) ){
		if( g_pyenstrlen - offset < 1024 ) pyenstrnew();
		l = sprintf( cpt, "%lld", PyLong_AsLongLong( pyo ) );
	}
	else if( PyFloat_CheckExact( pyo ) ){
		if( g_pyenstrlen - offset < 1024 ) pyenstrnew();
		double db = PyFloat_AS_DOUBLE( pyo );
		if( db != db ) {
			l = 3;
			strcpy( cpt, "NaN" );
		}
		else {
			l = sprintf( g_pyenstr + offset,"%f", db );
		}
	}
	else if( PyList_Check( pyo ) ){
		PyObject *ref = PyDict_GetItem(g_enc_refs, PyInt_FromLong(pyo) );
		int ss = PyList_Size( pyo );
		if( offset + 1024 > g_pyenstrlen ) pyenstrnew();
		l = sprintf( cpt,  "[" SEP "%d" SEP, ss );
		if( ref == NULL ){
			
			Cls_Ref_Info* info = PyMem_Malloc( sizeof( Cls_Ref_Info ) );
			info->no = 0;
			info->pos = offset + l;

			ref = PyCObject_FromVoidPtr( info , NULL );
			PyDict_SetItem( g_enc_refs, PyInt_FromLong( pyo ), ref );
		}
		else{
			 // printf("handle enc ref\n");
			Cls_Ref_Info * info = ( Cls_Ref_Info*)PyCObject_AsVoidPtr( ref );
			if( info->no == 0 ){
				g_enc_count++;
				info->no = g_enc_count;
				int ml = sprintf(constr, ":" SEP "%d" SEP, g_enc_count );
				// 所有的对象都后移  ml
				PyObject *k, *v;
				Py_ssize_t pos = 0;
				while( PyDict_Next( g_enc_refs, &pos, &k, &v ) ){
					Cls_Ref_Info *_info = PyCObject_AsVoidPtr( v );
					if( _info->pos > info->pos ) _info->pos += ml;
				}
				if( offset + l + ml > g_pyenstrlen ) pyenstrnew();
				memmove( g_pyenstr + info->pos + ml, g_pyenstr+info->pos, offset + l - pos );
				memcpy( g_pyenstr + info->pos, constr, ml );

				offset += ml;
			}
			l = sprintf( cpt, "=" SEP "%d", info->no );	
			return l;
		}

		for( int i=0; i < ss; i++){
			PyObject* o = PyList_GET_ITEM( pyo, i );
			int l2 = pyo_inter_encode( o, cpt+l,offset + l );
			if( l2 == NULL ) return NULL;
			l = l + l2;
			*(cpt+l) = SEP2;
			++l;
		}

		*(cpt +l ) = ']';
		l++;

	}
	else{
		printf("!!!!!Error\n");
		PyErr_SetObject(PyExc_TypeError , pyo );
		return NULL;
	}

	return l;
}

PyObject* pyo_encode(PyObject* self, PyObject * o ){
	
	PyDict_Clear( g_enc_refs);
	g_enc_count = 0;
	int l = pyo_inter_encode( o, g_pyenstr, 0 );
	// 清除 g_enc_refs 的内容
	PyObject *k, *v;
	Py_ssize_t pos = 0;
	while( PyDict_Next( g_enc_refs, &pos, &k, &v )){
		Cls_Ref_Info * _info = PyCObject_AsVoidPtr( v );
		Py_XINCREF( k );
		Py_XINCREF( v );
		PyMem_Free( _info );
	};


//	int l = 1;
	if( l == 0 ) return NULL;
//	Py_RETURN_NONE;
	return PyString_FromStringAndSize( g_pyenstr, l );
}

static PyMethodDef pyMds[] = {
	{ "def_enc", pyo_def_enc, METH_VARARGS, "define encode rule"},
	{ "enc", pyo_encode, METH_O, "encode object"},
	{ NULL, NULL, NULL, NULL }
};
int initPyVM(){
	Py_Initialize();
	pyo_init();
	PyObject *m = Py_InitModule("pyjot", pyMds);
	assert( m!= NULL );

	FILE * F = fopen( "py/main.py","r" );
	if( F == NULL ){
		printf("cant open py/main.py\n");
		return 1;
	}
	PyRun_AnyFile( F, "main.py");
	fclose(F);
	return 0;
}



