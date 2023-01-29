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

SOURCE = $(wildcard *.cc)
PROTOS = $(wildcard $(PROTOS_PATH)/*proto)
OBJECTS = $(SOURCE:.cc=.o) $(PROTOS:.proto=.pb.o) $(PROTOS:.proto=.grpc.pb.o)

# Seems necessary to override the built-in target for .o 
.SUFFIXES:

.PRECIOUS: $(PROTOS_PATH)/%.grpc.pb.cc
$(PROTOS_PATH)/%.grpc.pb.cc: $(PROTOS)
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=$(PROTOS_PATH) --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $*.proto

.PRECIOUS: $(PROTOS_PATH)/%.pb.cc
$(PROTOS_PATH)/%.pb.cc: $(PROTOS)
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=$(PROTOS_PATH) $*.proto

$(PROTOS_PATH)/%.pb.o: $($@:.o=.cc)
$(PROTOS_PATH)/%.grpc.pb.o: $($@:.o=.cc)

%.o : %.cc $(PROTOS:.proto=.grpc.pb.cc) $(PROTOS:.proto=.pb.cc)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(TEST_EXEC): $(OBJECTS) 
	$(CXX) $^ -o $@ $(LDFLAGS)

test: $(TEST_EXEC)
	./$(TEST_EXEC)

clean:
	rm -f *.o $(PROTOS_PATH)/*.pb.cc $(PROTOS_PATH)/*.pb.h $(PROTOS_PATH)/*.o $(TEST_EXEC)