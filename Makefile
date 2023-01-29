CXX = g++ -g
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
           -pthread\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl \
		   -lcppunit
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = protos

TEST_EXEC := test_runner

all: $(TEST_EXEC)

%.grpc.pb.cc:
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $*.proto

%.pb.cc:
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $*.proto

%.o : %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(TEST_EXEC): $(TEST_EXEC).o $(OBJS) bidichat.pb.o bidichat.grpc.pb.o
	$(CXX) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o *.pb.cc *.pb.h $(TEST_EXEC)