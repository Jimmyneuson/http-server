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

std::string plain_text_response(std::string body)
{
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: " + std::to_string(body.length()) + "\r\n\r\n";
    response += body;
    return response;
}

std::string extract_header(std::string request, std::string header)
{
    int start = request.find(header);
    int end = request.find("\r\n", start);
    int len = header.length() + 1;
    std::string value = request.substr(start + len, end - start + len);
    return value;
}

int main(int argc, char **argv)
{
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    const std::string response_200 = "HTTP/1.1 200 OK\r\n\r\n";
    const std::string response_404 = "HTTP/1.1 404 Not Found\r\n\r\n";

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "Failed to create server socket\n";
        return 1;
    }

    // Quickly restart server without binding issue
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
    {
        std::cerr << "setsockopt failed";
        return 1;
    }

    // INET address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

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

    size_t buffer_size = 1024;
    char *buffer = new char[buffer_size];
    if (recv(client_fd, buffer, buffer_size, 0) < 0)
    {
        std::cerr << "Could not receive request";
        return 1;
    }

    std::cout << "Request:\n"
              << buffer << std::endl;
    std::string request(buffer, buffer_size);

    std::string dir = parse_directory(request);
    if (dir == "/")
    {
        if (send(client_fd, response_200.c_str(), response_200.size(), 0) == -1)
        {
            std::cerr << "Could not send response";
        }
    }
    else if (dir.starts_with("/echo/"))
    {
        std::string body = dir.substr(6, dir.length() - 5);
        std::string response = plain_text_response(body);
        if (send(client_fd, response.c_str(), response.size(), 0) == -1)
        {
            std::cerr << "Could not send response";
            return 1;
        }
    }
    else if (dir.starts_with("/user-agent"))
    {
        std::string body = extract_header(request, "User-Agent");
        std::string response = plain_text_response(body);
        if (send(client_fd, response.c_str(), response.size(), 0) == -1)
        {
            std::cerr << "Could not send response";
            return 1;
        }
    }
    else
    {
        if (send(client_fd, response_404.c_str(), response_404.size(), 0) == -1)
        {
            std::cerr << "Could not send response";
            return 1;
        }
    }

    close(server_fd);
    return 0;
}