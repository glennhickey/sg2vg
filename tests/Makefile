rootPath = ..
include ${rootPath}/include.mk

ga2vgObjectsAll = $(wildcard ${rootPath}/*.o)
ga2vgObjects=$(subst ../ga2vg.o,,${ga2vgObjectsAll})

all : unitTests

clean :
	rm -f *.o unitTests

unitTests : CuTest.o unitTests.o sgclientTests.o 

CuTest.o : ${sgExportPath}/tests/CuTest.h ${sgExportPath}/tests/CuTest.c
	${cxx} ${cflags} -I ${sgExportPath}/tests -c ${sgExportPath}/tests/CuTest.c	

unitTests.o : unitTests.h unitTests.cpp
	${cpp} ${cppflags} -I ${sgExportPath}/tests -I${rootPath}/ unitTests.cpp -c

sgclientTests.o : sgclientTests.cpp ${rootPath}/*.h
	${cpp} ${cppflags} -I ${sgExportPath}/tests -I${rootPath}/ sgclientTests.cpp -c

unitTests : CuTest.o unitTests.o sgclientTests.o ${sgExportPath}/sgExport.a ${ga2vgObjects} ${basicLibsDependencies}
	${cpp} -I ${sgExportPath}/tests -I${rootPath}/ ${cppflags}  CuTest.o unitTests.o sgclientTests.o ${ga2vgObjects} ${basicLibs} ${sgExportPath}/sgExport.a -o unitTests

