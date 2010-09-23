#include <stdlib.h>

#define _GNU_SOURCE
#include "pys.h"
#include <structmember.h>
#include <string.h>
#include <arpa/inet.h>
#include "zmalloc.h"

char *strdup( char *);

char *g_pyenstr = NULL;
int g_pyenstrlen = 0;

PyObject *g_pyo_enc_rule;

typedef struct _Cls_Def{
	char **atts;
	char *fakename;
	int  atts_len ;
} Cls_Def;

typedef struct _Cls_Ref_Info{
	int pos;
	int no;
} Cls_Ref_Info;
int g_enc_count ;
PyObject * g_enc_refs;
PyObject * g_dec_refs;

static char * pyenstrnew( ){
//	printf(">>> strnew\n" );	
	char* s = PyMem_Malloc( g_pyenstrlen + 10240 + 4 );
//	printf(">>> s %d  %d\n", s, g_pyenstr );
	memcpy( s+4, g_pyenstr, g_pyenstrlen );
	PyMem_Free( g_pyenstr-4 );
	g_pyenstr = s + 4;
	g_pyenstrlen += 10240;	
	return g_pyenstr;
}
static void pyo_init(){
	
	g_pyenstrlen = 102400;
	g_pyenstr = PyMem_Malloc( g_pyenstrlen + 4 );
	g_pyenstr = g_pyenstr + 4;

	g_enc_refs = PyDict_New();
	g_dec_refs = PyDict_New();

}

static PyObject* pyo_def_enc( PyObject* self, PyObject* args ){
	PY_N( self );

	PyObject* rule;
	PyObject* cls;
	PyObject* atts = NULL;
	char* fakename = NULL;
//	self;

	if( !PyArg_ParseTuple( args, "OO|Os", &rule, &cls, &atts, &fakename ) ){
		PyErr_BadArgument();
		return NULL;
	}
	
	if(! PyObject_HasAttrString( rule, "__rules__" )){
		PyObject_SetAttrString( rule, "__rules__", PyDict_New() );
	}

	if(! PyObject_HasAttrString( rule, "__names__")){
		PyObject_SetAttrString( rule, "__names__", PyDict_New() );
	}


	PyObject *dict = PyObject_GetAttrString( rule, "__rules__" );
	PyObject *dictNs = PyObject_GetAttrString( rule, "__names__" );

	PyObject *v = PyDict_GetItem( dict, cls );
	if( v ){
		PyErr_SetString(PyExc_RuntimeError, "all readey set this rule");
	  	return NULL;
	}
	
	Cls_Def * pdef = PyMem_Malloc( sizeof (Cls_Def)  );
	pdef->atts = NULL;
	pdef->atts_len = 0;
	pdef->fakename = NULL;

	if( fakename ){
		pdef->fakename = strdup( fakename );
	}
	else{
		pdef->fakename = strdup( PyString_AsString( PyObject_GetAttrString( cls, "__name__" ) ) );
	}

	PyDict_SetItemString( dictNs, pdef->fakename, cls );

	if( atts != NULL){
		Py_ssize_t ss= PyList_Size( atts );
		pdef->atts = PyMem_Malloc( (ss + 1) *sizeof( char* ));
		pdef->atts[ss] = NULL;
		pdef->atts_len = ss;
		for( int i=0; i < ss; i++ ){
			char *pname = PyString_AS_STRING(	PyList_GetItem( atts, i ) );
			if( pname ){
				pdef->atts[i] = strdup( pname );
			}
		}
	}
	else{
		// look up parent class rule
		PyObject * pcls = cls;
		while( 1 ){
			PyObject* bases = PyObject_GetAttrString( pcls, "__bases__" );
			if( bases == NULL ) return NULL;
			if( PyTuple_GET_SIZE(bases ) == 0 ) break;
			pcls = PyTuple_GET_ITEM( bases, 0 );
			if( pcls ){
				PyObject *pv = PyDict_GetItem( dict, pcls );
				if( pv ){
					Cls_Def * ppdef = PyCObject_AsVoidPtr( pv );
					pdef->atts = PyMem_Malloc( ( ppdef->atts_len+1 ) * sizeof( char*) );
					for( int i = 0; i < ppdef->atts_len + 1; i++ ){
						pdef->atts[i] = ppdef->atts[i];
					}
					pdef->atts[ppdef->atts_len] = NULL;
					break;
				}
			}
			else{
				break;
			}
		}
	}
	PyDict_SetItem( dict, cls, PyCObject_FromVoidPtr( pdef, NULL ));
	Py_RETURN_NONE;
}

#define SEP "\x10"
#define SEP2 '\x10'

int pyo_handle_enc_ref( PyObject* pyo, PyObject* ref,  int pos, int * offset ){
//	printf("pyo_handle_enc_ref %d, %d\n", pos, *offset );
	int l = 0;
	static char constr[1024];
	if( ref == NULL ){		
		Cls_Ref_Info* info = PyMem_Malloc( sizeof( Cls_Ref_Info ) );
		info->no = 0;
		info->pos = pos;
		ref = PyCObject_FromVoidPtr( info , NULL );
		PyDict_SetItem( g_enc_refs, PyInt_FromLong( (unsigned long)pyo ), ref );
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
			Py_ssize_t _pos = 0;
			while( PyDict_Next( g_enc_refs, &_pos, &k, &v ) ){
				Cls_Ref_Info *_info = PyCObject_AsVoidPtr( v );
				if( _info->pos > info->pos ) _info->pos += ml;
			}
//			printf("memmove %d, %d , %d\n", info->pos + ml, info->pos, *offset - info->pos );
//			printf("memmove str %s\n", g_pyenstr + info->pos );

			memmove( g_pyenstr + info->pos + ml, g_pyenstr+info->pos, *offset - info->pos );
			memcpy( g_pyenstr + info->pos, constr, ml );
		//	*offset += ml;
			l += ml;

		}
		l += sprintf( g_pyenstr + l + (*offset), "=" SEP "%d", info->no );	
		return l;
	}
	return l;
}


int pyo_inter_encode( PyObject* pyo, char *cpt, int offset, PyObject* rule  ){
	int l = 1;
	if( g_pyenstrlen - offset < 10240 ) pyenstrnew();
	cpt = g_pyenstr + offset;
	if( pyo == Py_None ) *cpt = ',';
	else if( Py_True  == pyo ) *cpt = '>';
	else if( Py_False == pyo ) *cpt = '<';
	else if( PyInt_Check( pyo ) ){
//		int v = PyInt_AS_LONG( pyo );
//		l = snprintf( cpt, 1000, "%ld", v );
		PyObject * o = PyObject_Str( pyo );
		char *s;
		s = PyString_AS_STRING( o );
		l = PyString_GET_SIZE( o );
		memcpy( g_pyenstr + offset , s, l );
		Py_DECREF( o );
	}
	else if( PyLong_CheckExact( pyo ) ){
		PY_LONG_LONG value = PyLong_AsLongLong( pyo );
		char buf[32], *p;
		unsigned long long v;
		v = ( value < 0 ) ? -value : value;
		p = buf + 31;
		do {
			*p-- = '0'+(v%10);
			v /= 10;
		} while(v);
		if (value < 0) *p-- = '-';
		p++;
		l = 32-(p-buf);
		memcpy( g_pyenstr + offset, p, l ); 
	}
	else if( PyFloat_CheckExact( pyo ) ){
		double db = PyFloat_AS_DOUBLE( pyo );
		if( db != db ) {
			l = 3;
			strcpy( g_pyenstr+ offset, "NaN" );
		}
		else {
//	PyFloat_AsString( g_pyenstr + offset, pyo );
			l = sprintf(g_pyenstr+ offset, "%f", db );
		}
	}
	else if( PyList_CheckExact( pyo ) ){
		l = 0;
		PyObject *lk = PyInt_FromLong( (long)pyo );
		PyObject *ref = PyDict_GetItem(g_enc_refs, lk );
		Py_CLEAR( lk );
		int ss = PyList_Size( pyo );
		if ( ref == NULL ){
			l = sprintf( g_pyenstr+offset,  "[" SEP "%d" SEP, ss );
		}
		int r = pyo_handle_enc_ref( pyo, ref, l + offset,  &offset );
		if( r ) return r;
		for( int i=0; i < ss; i++){
			PyObject* o = PyList_GET_ITEM( pyo, i );
			int l2 = pyo_inter_encode( o, g_pyenstr+ offset + l,offset + l , rule );
			if( l2 == 0 ) return 0;
			l = l + l2;
			*(g_pyenstr + offset +l) = SEP2;
			++l;
		}
		*(g_pyenstr + offset +l ) = ']';
		l++;

	}
	else if( PyDict_CheckExact( pyo )){
		l = 0;
		PyObject *lk = PyInt_FromLong( (long)pyo );
		PyObject *ref = PyDict_GetItem( g_enc_refs, lk );
		Py_DECREF( lk );
		if( ref == NULL ){
			strcpy( g_pyenstr+offset, "{" SEP SEP );
			l+=3;
		}
		int r = pyo_handle_enc_ref( pyo, ref, l + offset, &offset );
		if( r ) return r;
		PyObject *k, *v; Py_ssize_t pos = 0;
		while( PyDict_Next( pyo, &pos, &k, &v )){
			if(! PyString_CheckExact( k ) ){
				k = PyObject_Str( k );
				Py_DECREF(k);
			}

			char *buff; Py_ssize_t ss;
			buff = PyString_AS_STRING( k );
			ss = PyString_GET_SIZE( k );
			strncpy( g_pyenstr + offset + l, buff, ss );
			l += ss;
			(g_pyenstr + offset +l)[0] = SEP2;
			++l;
			int l2 = pyo_inter_encode( v, g_pyenstr + offset + l, offset+l, rule );
			if( l2 == 0 ) return 0;
			l = l+ l2;
			*(g_pyenstr+offset+l) = SEP2;
			++l;
			
		}
		strcpy( g_pyenstr+offset +l, SEP "}" );
		l+=2;
	}
	else if( PyString_CheckExact( pyo ) ){
		char *buff;
		l = 0;
		Py_ssize_t ss;
		buff = PyString_AS_STRING( pyo );
		ss = PyString_GET_SIZE( pyo );
		(g_pyenstr+offset+l)[0] = SEP2;
		++l;
		memcpy( g_pyenstr + offset + l, buff, ss );
		l += ss;
	}
	else{
		l = 0;
		PyObject *lk = PyInt_FromLong( (long) pyo );
		PyObject * ref = PyDict_GetItem( g_enc_refs, lk );
		Py_DECREF( lk );
		Cls_Def * def = NULL;
		if( ref == NULL ){
			PyObject *type = PyObject_GetAttrString( pyo, "__class__" );
			PyObject *rdict = NULL;
			
			rdict = PyObject_GetAttrString( rule ,"__rules__" );
			if( rdict ){
				PyObject* ds = PyDict_GetItem( rdict, type );
				if( ds ) {
					def = PyCObject_AsVoidPtr( ds );
				}
			}
			if(  def == NULL ) {
				PyErr_Format( PyExc_Exception, "cant find rule for '%s'", PyString_AsString( PyObject_GetAttrString( type, "__name__" ) ) );
				return 0;
			}
			
			memcpy( g_pyenstr + offset , "{" SEP , 2 ); l+=2;
			int fl = strlen( def->fakename );
			memcpy( g_pyenstr+ offset +l, def->fakename, fl ); l+=fl;
			*(g_pyenstr + offset + l) = SEP2; l +=1;
		}
		int r = pyo_handle_enc_ref( pyo, ref, l + offset, &offset );
		if( r ) return r;
		PyObject *odict = PyObject_GetAttrString( pyo , "__dict__");
		if( odict ){
			int i = 0;
			while( 1 ){
				char *name = def->atts[i];
				if( name == NULL ) break;
				PyObject * v = PyDict_GetItemString( odict, name );
				if( v ){
					strcpy( g_pyenstr+offset + l, name );
					l += strlen( name );
					(g_pyenstr + offset +l)[0] = SEP2;
					l ++;
					int l2 = pyo_inter_encode( v, g_pyenstr + offset + l, offset + l, rule );
					if( l2 == 0 ) return 0;
					l += l2;
					(g_pyenstr+ offset + l)[0] = SEP2;
					l++;
				}
				i ++;
			}
		}
		strcpy( g_pyenstr+ offset + l, SEP "}" );
		l += 2;	
	}

	return l;
}

int pyo_handle_dec_ref( char *buff, PyObject * o ){
	int l = 0;
	while (1){
		char ch = buff[l];

		if( ch >= '0' && ch <= '9' ){
				
		}else if( ch == SEP2 ){
			buff[l] = 0;
			PyDict_SetItem( g_dec_refs, PyString_FromString( buff ), o );
			return l;
		}
		else{
			return 0;
		}
		l ++;
	}
}

PyObject* pyo_inter_decode( char * buff, int * len, PyObject *rule ){
	if( buff[0] == SEP2 ){ // string
		int i = 1;
		while( buff[i] != 0 && buff[i] != SEP2 ) ++i;
		*len = i ;
		return PyString_FromStringAndSize( buff + 1, i - 1 );
	}	
	else if( buff[0] == '<' ){
		*len = 1;
		Py_RETURN_FALSE;
	}
	else if( buff[0] == '=' ){
		int l=2;
		char *start = buff + l;
		while( buff[l] > '0' && buff[l] < '9' ){
			l++;
		}
		if( buff[l] == SEP2 ){
			buff[l] = 0;
			PyObject * r = PyDict_GetItemString(g_dec_refs, start );
			Py_XINCREF( r );
			*len = l;
			return r;
		}
		else{
			return 0;
		}
	}
	else if( buff[0] == '>' ){
		*len = 1;
		Py_RETURN_TRUE;
	}
	else if( buff[0] == ',' ){
		*len = 1;
		Py_RETURN_NONE;
	}
	else if( buff[0] == '[' ){
		int l = 2;
		char *start = buff + 2;
		while(1){
			
			if( buff[l] >= '0' && buff[l] <= '9' ){
				l++;
				continue;
			}
			else if( buff[l] == SEP2 ){
				buff[l] = 0;
				break;
			}
			else{
				PyErr_SetString( PyExc_Exception, "list expect list size , but error" );
				return NULL;
			}
		}
		// get list size
		PyObject *r;
		l ++;
		int ss = atoi( start );
		r = PyList_New( ss );

		if( buff[l] == ':' ){
			l+=2;
			int l2 = pyo_handle_dec_ref( buff+l, r );
			if( l2 == 0 ) return 0;
			l += l2 +1;
		}

		for( int i =0; i < ss; i++){

			int outlen = 0;
			PyObject * o = pyo_inter_decode( buff + l, &outlen, rule );
			if( o == NULL ) return NULL;
			PyList_SET_ITEM( r, i, o );
			l += outlen + 1 ;
		}
		if( buff[l] == ']' ){
			*len = l + 1;
			return r;
		}else{
			return 0;
		}
	}
	else if( buff[0] == '{' && buff[1] == SEP2 && buff[2] == SEP2 ){
		int l = 3;
		PyObject * r = PyDict_New();
		while( 1 ){
			char c = buff[l];
			if( c == 0 ) return 0;
			else if( c == SEP2 ){
				if( buff[l+1] == '}' ){
					*len = l + 2;
					return r;
				}
			}
			else if( c == ':' ){
				l+=2;
				int l2 = pyo_handle_dec_ref( buff+l, r );
				if( l2 == 0 ) return 0;
				l += l2 + 1;
			}
			else{
				char *start = buff + l;
				while ( 1 ){
					if( buff[l] == SEP2 || buff[l] == 0 ){
						buff[l] = 0;
						break;
					}
					l++;	
				}
				l ++;
				int outlen = 0;
				PyObject * v = pyo_inter_decode(  buff + l, &outlen, rule );
				if( v == NULL ) return NULL;
				PyDict_SetItemString( r, start, v );
				Py_DECREF( v );
				l += outlen;
				l++;
			}

		}

	}
	else if( buff[0] == '{' && buff[1] == SEP2 && buff[2] != SEP2 ){
		int l = 2;
		char *cname = buff + 2;
		while( 1 ){
			char ch = buff[l];
			if( ch == 0 ) return 0;
			else if( ch == SEP2 ){
				buff[l] = 0;
				break;
			}
			++ l;
		}
		l++;

		PyObject *dictNs = PyObject_GetAttrString( rule, "__names__" );
		if( dictNs == NULL ){
			PyErr_SetString(PyExc_RuntimeError, "no rule found");
			return 0;
		}	
		
		PyObject *cls = PyDict_GetItemString( dictNs, cname );
		if( cls == NULL ){
			PyErr_Format( PyExc_RuntimeError, "cant find name: %s", cname );
			return 0;
		}
		
		PyObject *d = PyDict_New();
		PyObject *r = PyInstance_NewRaw( cls, d );
		Py_XDECREF( d );
		while( 1 ){
			char ch = buff[l];
			if ( ch == 0 ) return 0;
			else if( ch == ':' ){
				l+=2;
				int l2 = pyo_handle_dec_ref( buff+l, r );
				if( l2 == 0 ) return 0;
				l += l2 + 1;
			}
			else if( ch == SEP2 && buff[l+1] == '}' ){
				*len = l +2;
				return r;
			}
			else{
				if ( ch == SEP2 ){
					PyErr_SetString( PyExc_RuntimeError, "found \\0x10 ");
					return 0;
				}
				char * start = buff + l;
				while( 1 ){
					if( buff[l] == SEP2 ){
						buff[l] = 0;
						break;
					}
					else if( buff[l] == 0 ){
						return 0;
					}
					++l;
				}
				++ l;
				int outlen = 0;
				PyObject * v = pyo_inter_decode(  buff + l, &outlen, rule );
				if( v == NULL ) return NULL;
				PyDict_SetItemString( d, start, v );
				Py_DECREF( v );
				l += outlen;
				l++;
			}
		}

	}
	else{ // number
		char *start = buff;
		int i = 0;
		while( 1 ){
			if ( buff[i] == SEP2 || buff[i] == 0 ){
				buff[i] = 0;
				*len = i;
				if( strchr( start, '.') ){
					return PyFloat_FromString( PyString_FromString(start), NULL );
				}
				else if( i <=8 ){
					return PyInt_FromString(start, NULL, 10 );
				}
				else{
					return PyLong_FromString( start, NULL, 10 );
				}
				break;
			}
			i++;
		}
	}
}

PyObject* _pyo_decode( char * buff, PyObject *rule ){
	int len = 0;
	PyObject* r= pyo_inter_decode( buff, &len, rule );
	PyObject *k, *v;
	Py_ssize_t pos = 0;
	while( PyDict_Next( g_dec_refs, &pos, &k, &v ) ){
		Py_XDECREF(k);
	};
	PyDict_Clear( g_dec_refs );
	return r;
}

PyObject* pyo_decode( PyObject* self, PyObject * args ){
	PY_N( self );
	PyObject *buf = PyTuple_GET_ITEM( args, 0 );
	PyObject *rule = PyTuple_GET_ITEM( args, 1 );
	return _pyo_decode( PyString_AsString( buf ), rule );
}

char * _pyo_encode( PyObject* o, PyObject * rule, int *len ){
	g_enc_count = 0 ;
	int l = pyo_inter_encode( o, g_pyenstr, 0, rule );
	// 清除 g_enc_refs 的内容
	PyObject *k, *v;
	Py_ssize_t pos = 0;
	while( PyDict_Next( g_enc_refs, &pos, &k, &v )){
		Cls_Ref_Info * _info = PyCObject_AsVoidPtr( v );
		Py_DECREF( k );
		Py_DECREF( v );
		PyMem_Free( _info );
	};
	
	PyDict_Clear( g_enc_refs);
	*len = l;
	return g_pyenstr;
}

PyObject* pyo_encode(PyObject* self, PyObject * args ){
	PY_N( self );
	
	PyObject * rule = NULL;
	PyObject * o = PyTuple_GET_ITEM( args, 0 );
	rule = PyTuple_GET_ITEM( args, 1 );
	int l = 0;
	_pyo_encode( o, rule, &l );
	if( l == 0 ) return NULL;
	return PyString_FromStringAndSize( g_pyenstr, l );
}

PyObject * pyo_set_global_rule ( PyObject* self, PyObject* args ){
	PY_N(self);
	PyObject *o = PyTuple_GET_ITEM( args, 0 );
	g_pyo_enc_rule = o;
	Py_RETURN_NONE;
}

PyObject *pyo_set_object2( PyObject *self, PyObject *args ){
	return pyo_set_object( self, args );
}

PyObject* pyo_save(PyObject *self, PyObject *args ){
	PY_N(self);
	PY_N(args);
	PY_N( self );
	
	if( pysave() ){
		Py_RETURN_NONE;
	}
	PyErr_SetString( PyExc_RuntimeError, "save error");
	return 0;
}

PyObject* pyo_get( PyObject *self, PyObject *args){
	PY_N( self );
	
	PyObject * k  = PyTuple_GET_ITEM( args, 0 );
	char *key; Py_ssize_t len;
	PyString_AsStringAndSize( k, &key, &len );
	return pyget( key, len );
}

PyObject * pyo_calllater( PyObject *self, PyObject *args, PyObject *kw ){
	PY_N(self);
	double t = PyFloat_AsDouble( PyTuple_GET_ITEM( args, 0 ) );
	PyObject * fun = PyTuple_GET_ITEM( args, 1 );
	PyObject * as = PyTuple_New( PyTuple_Size( args ) -2 );
	for( int i = 2; i < PyTuple_Size( args ); i++){
		PyObject * o = PyTuple_GET_ITEM( args, i );
		Py_XINCREF( o );
		PyTuple_SET_ITEM( as, i -2, o );
	}
	pycallLater( t, fun, as, kw );
	Py_RETURN_NONE;
}

PyObject * pyo_server( PyObject *self, PyObject *args ){
	PY_N( self );
	int port = PyInt_AsLong( PyTuple_GET_ITEM( args , 0 ) );
	PyObject *proto = PyTuple_GET_ITEM( args, 1 );
	Py_XINCREF( proto );
	int r = pycreateServer( port, proto );
	if( r < 1 ){
		PyErr_SetString( PyExc_RuntimeError, "create server error" );
		return 0;
	}
	Py_RETURN_NONE;
}


static void pyconn_dealloc( PyObjectConn * self ){
	if( self->buffer ) zfree( self->buffer );
	close(self->fd);
	PyObject_DEL( self );
}

static PyObject* pyconn_send( PyObjectConn *self, PyObject * args ){
	PyObject *o = PyTuple_GET_ITEM( args, 0 );
	int len ;
	char * buf = _pyo_encode( o, g_pyo_enc_rule, &len );
	if( len == 0) {
		if( !PyErr_Occurred() ){
			PyErr_SetString(PyExc_RuntimeError, "_pyo_encode error");
		}
		return 0;
	}
	long *pl = (long*) (buf-4);
	*pl = htonl(len);
	int r = pysend( self->fd, buf-4, len+4 );
	if( r > 0 ) Py_RETURN_NONE; 
	PyErr_SetFromErrno( PyExc_SystemError );
	return 0;
}

static PyMethodDef pyconn_methods[] = {
	{"send",(PyCFunction)pyconn_send,	METH_VARARGS, NULL },
	{NULL, NULL, 0, NULL} ,
};

static PyMemberDef pyconn_memberlist[] = {
       {"fd", T_INT, offsetof(PyObjectConn, fd), READONLY, "the socket fd"},
       {NULL, 0, 0, 0, NULL},
};

static PyTypeObject PyConn_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)	/* Must fill in type value late */
	"redis.conn",			/* tp_name */
	sizeof(PyObjectConn),		/* tp_basicsize */
	0,					/* tp_itemsize */
	(destructor)pyconn_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	PyObject_GenericSetAttr,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	0,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	pyconn_methods,				/* tp_methods */
	pyconn_memberlist,			/* tp_members */
	0
};

PyObject * PyObjectConn_New( int fd ){
	PyObjectConn * self;
	self = PyObject_NEW( PyObjectConn, &PyConn_Type);
	if( self == NULL ) return NULL;
	self->fd = fd;
	self->server = 0;
	self->protocol = 0;
	self->buffer = 0;
	self->bufferlen = 0;
	return (PyObject*)self;
}

static PyMethodDef pyMds[] = {
	{ "def_enc", pyo_def_enc, METH_VARARGS, "define encode rule"},
	{ "enc", pyo_encode, METH_VARARGS, "encode object"},
	{ "dec", pyo_decode, METH_VARARGS, "decode string to python object"},
	{ "rule", pyo_set_global_rule, METH_VARARGS, "set global rule"},
	{ "set", pyo_set_object2, METH_VARARGS, "set key"},
	{ "save", pyo_save, METH_VARARGS, "save"},
	{ "get",pyo_get, METH_VARARGS, "get"},
	{ "callLater", pyo_calllater, METH_KEYWORDS, "callLater" },
	{ "server", pyo_server, METH_KEYWORDS, "create server" },
	{ NULL, NULL, NULL, NULL }
};
int initPyVM(){
	Py_Initialize();
	pyo_init();
	PyObject *m = Py_InitModule("pyjot", pyMds);
	assert( m!= NULL );

	Py_TYPE(&PyConn_Type) = &PyType_Type;

	FILE * F = fopen( "py/main.py","r" );
	if( F == NULL ){
		printf("cant open py/main.py\n");
		return 1;
	}
	PyRun_AnyFile( F, "main.py");
	fclose(F);
	return 0;
}



