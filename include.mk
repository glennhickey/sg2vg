#copied from sonLib...

# we do specific stuff for specific host for now.
HOSTNAME = $(shell hostname)
MACH = $(shell uname -m)
SYS =  $(shell uname -s)

# We need bash for pushd and popd
SHELL=/bin/bash

cxx = cc -std=c99
cpp = c++ 

platformCompileFlags=
platformLinkFlags=

#C compiler
ifeq ($(SYS),Darwin)
	platformCompileFlags = -I/opt/local/include
	platformLinkFlags g =  -L/opt/local/lib
else
endif

# Compiler flags.
# DO NOT put static library -l options here. Those must be specified *after*
# linker input files. See <http://stackoverflow.com/a/8266512/402891>.

#Release compiler flags
cflags_opt = -O3 -g -Wall --pedantic -funroll-loops -DNDEBUG 
#-fopenmp
cppflags_opt = -O3 -g -Wall -funroll-loops -DNDEBUG -Wno-sign-compare

#Debug flags (slow)
cflags_dbg = -Wall -Werror --pedantic -g -fno-inline
cppflags_dbg = -Wall -g -O0 -fno-inline 

#Ultra Debug flags (really slow)
cflags_ultraDbg = -Wall -Werror --pedantic -g -fno-inline 

#Profile flags
cflags_prof = -Wall -Werror --pedantic -pg -O3 

#for cpp code: don't use pedantic, or Werror
cppflags = ${cppflags_opt} 

#Flags to use
cflags = ${cflags_opt} 

binPath=${rootPath}
libPath=${rootPath}

sgExportPath=${rootPath}/sgExport
rapidJsonPath=${rootPath}/rapidjson

cflags +=  -I ${sgExportPath} ${platformCompileFlags}
cppflags +=  -I ${sgExportPath} -I ${rapidJsonPath}/include ${platformCompileFlags}
basicLibs = ${sgExportPath}/sgExport.a -lz ${platformLinkFlags} -lcurl
basicLibsDependencies = ${sgExportPath}/sgExport.a


