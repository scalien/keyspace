%module keyspace_client
%include "stl.i"
%include "inttypes.i"
%include "stdint.i"

%{
/* Includes the header in the wrapper code */
#define SWIG_FILE_WITH_INIT
#include "../KeyspaceClientWrap.h"
%}
 
/* Parse the header file to generate wrappers */
%include "../KeyspaceClientWrap.h"
