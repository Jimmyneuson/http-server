CC = g++
CFLAGS = -std=c++20
SRC_FILES = src/server.cpp src/HttpMessage.cpp src/HttpRequest.cpp src/HttpResponse.cpp
OUTPUT = server

.PHONY: clean test run

# Build server binary
$(OUTPUT): $(SRC_FILES)
	$(CC) $(CFLAGS) $(SRC_FILES) -o $(OUTPUT) -lz

# Clean up binary
clean:
	rm -f $(OUTPUT)

# Run server
run: $(OUTPUT)
	./$(OUTPUT)

test: $(OUTPUT)
	@echo "Starting server in the background"
	@./$(OUTPUT) --directory /tmp/ > /dev/null 2>&1 &
	@echo "Beginning tests"
	@sleep 1
	@printf "Running root directory test: "
	@curl -sI localhost:80/ | grep -q "200 OK" && echo "passed" || echo "failed"
	@printf "Running non-existant directory test: "
	@curl -sI localhost:80/none | grep -q "404 Not Found" && echo "passed" || echo "failed"
	@printf "Running echo directory test: "
	@curl -s -o - localhost:80/echo/abc | grep -q "abc" && echo "passed" || echo "failed"
	@printf "Running user-agent directory test: "
	@curl -s -o - localhost:80/user-agent | grep -q "curl/8.7.1" && echo "passed" || echo "failed"
	@printf "Running file write/read test: "
	@curl -s --data "12345" -H "Content-Type: application/octet-stream" localhost:80/files/foo 
	@curl -s -o - localhost:80/files/foo | grep -q "12345" && echo "passed" || echo "failed"
	@printf "Running non-existant file test: "
	@curl -s -w "%{http_code}" localhost:80/files/none | grep -q "404" && echo "passed" || echo "failed"
	@printf "Running gzip test: "
	@curl -s -H "Accept-Encoding: gzip" -o - localhost:80/echo/abc | gunzip | grep -q "abc" && echo "passed" || echo "failed"
	@pkill -f ./$(OUTPUT)
