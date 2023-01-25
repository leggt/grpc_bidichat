# bidichat
The goal was to test using grpc c++ async and callback apis to manage a bidirectional stream with an asyncronous response protocol. The example is a chat app. A client will connect to the server and send a stream of messages to the server. In turn the server will receive messages from all clients and send all received messages to all connected clients.

There are two implementations of the server. `bidichat-callback-server.cc` implements the server using grpc's callback api. `bidichat-async-server.cc` implements the server using grpc's async api.

# Build
```
make all
```

# Test
```
bidichat$ make test
./bidichat-async-server& sleep 4; ./bidichat-callback-client jim& ./bidichat-callback-client bob& sleep 5; pgrep -f bidichat-async-server | xargs kill -9
Server listening on 0.0.0.0:50051
(on client jim) message from jim: sup
(on client bob) message from jim: sup
(on client jim) message from jim: yo
(on client bob) message from jim: yo
(on client jim) message from jim: sup
(on client bob) message from bob: sup
(on client jim) message from jim: what is grpc?
(on client bob) message from jim: what is grpc?
(on client jim) message from jim: yo
(on client jim) message from jim: what is grpc?
(on client bob) message from bob: yo
(on client bob) message from bob: what is grpc?
```