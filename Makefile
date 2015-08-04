rootPath = ./
include ${rootPath}/include.mk

all : ga2vg

clean : 
	rm -f  ga2vg ga2vg.o sgclient.o download.o json2sg.o side2seq.o
	cd sgExport && make clean
	cd tests && make clean
	rm -f vg.pb.h vg.pb.cc vg.pb.o vg.pb2.py

unitTests : ga2vg
	cd tests && make

${sgExportPath}/sgExport.a : ${sgExportPath}/*.cpp ${sgExportPath}/*.h
	cd ${sgExportPath} && make

${protobufPath}/libprotobuf.a: ${protobufPath}/src/google/protobuf/*cc  ${protobufPath}/src/google/protobuf/*h
	cd ${protobufPath} && mkdir -p build && ./autogen.sh && ./configure --prefix=`pwd`/build/ && make && make install
	cp ${protobufPath}/build/lib/libprotobuf.a ${protobufPath}/

vg.pb2.py: vg.proto ${protobufPath}/libprotobuf.a
	${protobufPath}/build/bin/protoc vg.proto --python_out=.

vg.pb.cc: vg.pb.h
vg.pb.h: vg.proto ${protobufPath}/libprotobuf.a
	${protobufPath}/build/bin/protoc vg.proto --cpp_out=.

vg.pb.o: vg.pb.h vg.pb.cc
	${cpp} ${cppflags} -I . vg.pb.cc -c 

ga2vg.o : ga2vg.cpp sgclient.h download.h side2seq.h json2sg.h sg2vgjson.h ${basicLibsDependencies}
	${cpp} ${cppflags} -I . ga2vg.cpp -c

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

ga2vg : ga2vg.o sgclient.o download.o json2sg.o sg2vgjson.o side2seq.o ${basicLibsDependencies}
	${cpp} ${cppflags}  ga2vg.o sgclient.o download.o json2sg.o sg2vgjson.o side2seq.o ${basicLibs} -o ga2vg 

test : unitTests
	pushd .  && cd ${sgExportPath} && make test && popd && tests/unitTests


