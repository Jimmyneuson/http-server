# Installation
```
git clone https://github.com/jimmyneuson/http-server.git
cd http-server
make
```
# Usage
1. Run the server
```
make run
```
- Default port: 80
2. Test with a client
- Browser: http://localhost:80
- cURL: `curl -i http://localhost:80`
# How it Works
- Uses UNIX sockets and TCP to communicate
- Handles concurrent client connections
- Automated testing when compiling using Makefile
# Acknowledgement
Project specifications provided by [Codecrafters](https://app.codecrafters.io/courses/http-server/)
