HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CXX = g++ -g
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11
ifeq ($(SYSTEM),Darwin)
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
           -pthread\
           -lgrpc++_reflection\
           -ldl
else
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
           -pthread\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl
endif
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = protos

vpath %.proto $(PROTOS_PATH)

all: system-check bidichat-callback-server bidichat-callback-client bidichat-async-server bidichat-async-client

bidichat-callback-client: bidichat.pb.o bidichat.grpc.pb.o bidichat-callback-client.o
	$(CXX) $^ $(LDFLAGS) -o $@

bidichat-callback-server: bidichat.pb.o bidichat.grpc.pb.o bidichat-callback-server.o
	$(CXX) $^ $(LDFLAGS) -o $@

bidichat-async-client: bidichat.pb.o bidichat.grpc.pb.o bidichat-async-client.o
	$(CXX) $^ $(LDFLAGS) -o $@

bidichat-async-server: bidichat.pb.o bidichat.grpc.pb.o bidichat-async-server.o
	$(CXX) $^ $(LDFLAGS) -o $@

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

clean:
	rm -f *.o *.pb.cc *.pb.h bidichat-callback-server bidichat-callback-client bidichat-async-server bidichat-async-client


# The following is to test your system and ensure a smoother experience.
# They are by no means necessary to actually compile a grpc-enabled software.

PROTOC_CMD = which $(PROTOC)
PROTOC_CHECK_CMD = $(PROTOC) --version | grep -q libprotoc.3
PLUGIN_CHECK_CMD = which $(GRPC_CPP_PLUGIN)
HAS_PROTOC = $(shell $(PROTOC_CMD) > /dev/null && echo true || echo false)
ifeq ($(HAS_PROTOC),true)
HAS_VALID_PROTOC = $(shell $(PROTOC_CHECK_CMD) 2> /dev/null && echo true || echo false)
endif
HAS_PLUGIN = $(shell $(PLUGIN_CHECK_CMD) > /dev/null && echo true || echo false)

SYSTEM_OK = false
ifeq ($(HAS_VALID_PROTOC),true)
ifeq ($(HAS_PLUGIN),true)
SYSTEM_OK = true
endif
endif

system-check:
ifneq ($(HAS_VALID_PROTOC),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have protoc 3.0.0 installed in your path."
	@echo "Please install Google protocol buffers 3.0.0 and its compiler."
	@echo "You can find it here:"
	@echo
	@echo "   https://github.com/protocolbuffers/protobuf/releases/tag/v3.0.0"
	@echo
	@echo "Here is what I get when trying to evaluate your version of protoc:"
	@echo
	-$(PROTOC) --version
	@echo
	@echo
endif
ifneq ($(HAS_PLUGIN),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have the grpc c++ protobuf plugin installed in your path."
	@echo "Please install grpc. You can find it here:"
	@echo
	@echo "   https://github.com/grpc/grpc"
	@echo
	@echo "Here is what I get when trying to detect if you have the plugin:"
	@echo
	-which $(GRPC_CPP_PLUGIN)
	@echo
	@echo
endif
ifneq ($(SYSTEM_OK),true)
	@false
endif

GRPC_TRACE:=all
GRPC_VERBOSITY:=DEBUG

debug-client:
	GRPC_TRACE=$(GRPC_TRACE) GRPC_VERBOSITY=$(GRPC_VERBOSITY) ./bidichat-client > client.log 2>&1

debug-server:
	GRPC_TRACE=$(GRPC_TRACE) GRPC_VERBOSITY=$(GRPC_VERBOSITY) ./bidichat-server > server.log 2>&1

test:
	./bidichat-async-server& sleep 4; ./bidichat-callback-client jim& ./bidichat-callback-client bob& sleep 5; pgrep -f bidichat-async-server | xargs kill -9
