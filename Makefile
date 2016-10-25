rootPath = ./
include ${rootPath}/include.mk

all : sg2vg

clean : 
	rm -f  sg2vg sg2vg.o sgclient.o download.o json2sg.o side2seq.o sg2vgjson.o libsg2vg.a 
	cd sgExport && make clean
	cd tests && make clean

unitTests : sg2vg
	cd tests && make

${sgExportPath}/sgExport.a : ${sgExportPath}/*.cpp ${sgExportPath}/*.h
	cd ${sgExportPath} && make

sg2vg.o : sg2vg.cpp sgclient.h download.h side2seq.h json2sg.h sg2vgjson.h ${basicLibsDependencies}
	${cpp} ${cppflags} -I . sg2vg.cpp -c

sgclient.o: sgclient.cpp sgclient.h download.h json2sg.h ${sgExportPath}/*.h
	${cpp} ${cppflags} -I. sgclient.cpp -c

download.o: download.cpp download.h 
	${cpp} ${cppflags} -I. download.cpp -c

json2sg.o: json2sg.cpp json2sg.h  ${sgExportPath}/*.h
	${cpp} ${cppflags} -I. json2sg.cpp -c

sg2vgjson.o: sg2vgjson.cpp sg2vgjson.h  ${sgExportPath}/*.h
	${cpp} ${cppflags} -I. sg2vgjson.cpp -c

side2seq.o: side2seq.cpp side2seq.h  ${sgExportPath}/*.h
	${cpp} ${cppflags} -I. side2seq.cpp -c

libsg2vg.a : sgclient.o download.o json2sg.o sg2vgjson.o side2seq.o
	ar rc libsg2vg.a sgclient.o download.o json2sg.o sg2vgjson.o side2seq.o

sg2vg : sg2vg.o libsg2vg.a ${basicLibsDependencies}
	${cpp} ${cppflags} sg2vg.o libsg2vg.a ${basicLibs} -o sg2vg 

test : unitTests
	pushd .  && cd ${sgExportPath} && make test && popd && tests/unitTests


