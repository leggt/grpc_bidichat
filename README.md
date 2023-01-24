# bidichat
The goal was to test using grpc c++ api to manage a bidirectional stream with an asyncronous response protocol. The example is a chat app. A client will connect to the server and send a stream of messages to the server. In turn the server will receive messages from all clients and send all received messages to all connected clients.

# Build
```
make all
```

# Test
```
make test
```