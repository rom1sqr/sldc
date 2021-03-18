#ifdef SWIGPYTHON
%module sldc

typedef unsigned int guint32;

%typemap(in) (guchar* buffer, gsize len, gsize* outlen) (unsigned long tmp) {
    $1 = (guchar*) PyString_AsString($input);
    $2 = PyString_Size($input);
    $3 = &tmp;
    /*
    tmp = 0;
    fprintf(stderr, "pyby= %p\n", swig_obj[0]);
    fprintf(stderr, "$1= %p\n", $1);
    fprintf(stderr, "$2= %ld\n", $2);
    fprintf(stderr, "$3= %ld\n", *$3);
    */
}
%typemap(out) guchar* {
    /*
    fprintf(stderr, "pyby= %p\t(", result);
    for (int i = 0; i < *arg3; i++) fprintf(stderr, "%02x", result[i]);
    fprintf(stderr, ")\n");
    fprintf(stderr, "arg3= %d\n", *arg3);
    */
    $result = PyBytes_FromStringAndSize((const char*) $1, *arg3);
}
#endif
%rename("%(strip:[sldc_])s") "";
%{
#include "sldc.h"
%}
%include "sldc.h"
