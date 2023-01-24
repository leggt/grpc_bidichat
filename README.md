# bidichat
The goal was to test using grpc c++ api to manage a bidirectional stream with an asyncronous response protocol. The example is a chat app. A client will connect to the server and send a stream of messages to the server. In turn the server will receive messages from all clients and send all received messages to all connected clients.

# Build
```
make all
```

# Test
```
bidichat$ make test 
./bidichat-server& sleep 4; ./bidichat-client jim& ./bidichat-client bob& sleep 5; pgrep bidichat-server | xargs kill -9; pgrep bidichat-client | xargs kill -9
Server listening on 0.0.0.0:50051
(on client bob) message from bob: sup
(on client bob) message from bob: yo
(on client jim) message from jim: sup
(on client bob) message from bob: what is grpc?
(on client jim) message from jim: yo
(on client bob) message from jim: sup
(on client jim) message from jim: what is grpc?
(on client bob) message from jim: yo
(on client bob) message from jim: what is grpc?
```