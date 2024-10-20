#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

const int PORT = 80;
const int BACKLOG = 5;

std::string parse_directory(std::string request)
{
    int start = request.find(" ") + 1;
    int end = request.find(" ", start);
    return request.substr(start, end - start);
}

int main(int argc, char **argv)
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cout << "Logs from your program will appear here!\n";

    // INET is the transport protocol (TCP server)
    // server_fd is the file descriptor for the socket (just an int)
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "Failed to create server socket\n";
        return 1;
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        std::cerr << "setsockopt failed\n";
        return 1;
    }

    // INET address
    // use `htons` to keep byte interpretations consistent
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Assign name to socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
    {
        std::cerr << "Failed to bind to port " << PORT << std::endl;
        return 1;
    }

    if (listen(server_fd, BACKLOG) != 0)
    {
        std::cerr << "Listen failed\n";
        return 1;
    }

    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    std::cout << "Waiting for a client to connect...\n";

    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    std::cout << "Client connected\n";

    // Send a HTTP request
    /*
    Status line
    HTTP/1.1  // HTTP version
    200       // Status code
    OK        // Optional reason phrase
    \r\n      // CRLF that marks the end of the status line

    Headers (empty)
    \r\n      // CRLF that marks the end of the headers

    request body (empty)
    */
    std::string response_200 = "HTTP/1.1 200 OK\r\n\r\n";
    std::string response_404 = "HTTP/1.1 404 Not Found\r\n\r\n";

    size_t buffer_size = 1024;
    char *buffer = new char[buffer_size];
    if (recv(client_fd, buffer, buffer_size, 0) < 0)
    {
        std::cerr << "Could not receive request";
    }

    std::cout << "Request:\n"
              << buffer << std::endl;
    std::string request(buffer, buffer_size);
    std::string dir = parse_directory(request);
    std::cout << dir;
    if (dir == "/")
    {
        if (send(client_fd, response_200.c_str(), response_200.size(), 0) == -1)
        {
            std::cerr << "Could not send response";
        }
    }
    else
    {
        if (send(client_fd, response_404.c_str(), response_404.size(), 0) == -1)
        {
            std::cerr << "Could not send response";
        }
    }

    close(server_fd);
    return 0;
}