protobufPath=./protobuf

all : vg.pb2.py


clean : 
	rm -f vg.pb2.py

${protobufPath}/libprotobuf.a: ${protobufPath}/src/google/protobuf/*cc  ${protobufPath}/src/google/protobuf/*h
	cd ${protobufPath} && mkdir -p build && ./autogen.sh && ./configure --prefix=`pwd`/build/ && make && make i nstall
	cp ${protobufPath}/build/lib/libprotobuf.a ${protobufPath}/

vg.pb2.py: vg.proto ${protobufPath}/libprotobuf.a
	${protobufPath}/build/bin/protoc vg.proto --python_out=.
