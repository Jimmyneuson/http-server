CC = g++
CFLAGS = -std=c++20
SRC_FILES = src/server.cpp src/HttpMessage.cpp src/HttpRequest.cpp src/HttpResponse.cpp
OUTPUT = server

$(OUTPUT): $(SRC_FILES)
	$(CC) $(CFLAGS) $(SRC_FILES) -o $(OUTPUT) -lz

clean:
	rm -f $(OUTPUT)
