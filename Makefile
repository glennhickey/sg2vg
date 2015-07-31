rootPath = ./
include ${rootPath}/include.mk

all : ga2vg

clean : 
	rm -f  ga2vg sgclient.o 
	cd sgExport && make clean
	cd tests && make clean
	rm -f vg.pb.h vg.pb.cc vg.pb.o vg.pb2.py

unitTests : ga2vg
	cd tests && make

ga2vg.o : ga2vg.cpp sgclient.h download.h ${basicLibsDependencies}
	${cpp} ${cppflags} -I . ga2vg.cpp -c

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

sgclient.o: sgclient.cpp sgclient.h download.h ${sgExportPath}/*.h
	${cpp} ${cppflags} -I. sgclient.cpp -c

download.o: download.cpp download.h 
	${cpp} ${cppflags} -I. download.cpp -c

ga2vg :  ga2vg.o sgclient.o download.o ${basicLibsDependencies}
	${cpp} ${cppflags}  ga2vg.o sgclient.o  ${basicLibs} -o ga2vg 

test : unitTests
	pushd .  && cd ${sgExportPath} && make test && popd && tests/unitTests


