#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <vector>

const int PORT = 80;
const int BACKLOG = 5;
const char* RESPONSE_200 = "HTTP/1.1 200 OK\r\n\r\n";
const char* RESPONSE_404 = "HTTP/1.1 404 Not Found\r\n\r\n";

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

void handle_request(int client_fd, std::string request) {
    std::string dir = parse_directory(request);

    if (dir == "/")
    {
        if (send(client_fd, RESPONSE_200, strlen(RESPONSE_200), 0) == -1)
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
        }
    }
    else if (dir.starts_with("/user-agent"))
    {
        std::string body = extract_header(request, "User-Agent");
        std::string response = plain_text_response(body);
        if (send(client_fd, response.c_str(), response.size(), 0) == -1)
        {
            std::cerr << "Could not send response";
        }
    }
    else
    {
        if (send(client_fd, RESPONSE_404, strlen(RESPONSE_404), 0) == -1)
        {
            std::cerr << "Could not send response";
        }
    }
}

int get_server_socket() {
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

    // Bind to port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
    {
        std::cerr << "Failed to bind to port " << PORT << std::endl;
        return 1;
    }

    // Listen to incoming connections
    if (listen(server_fd, BACKLOG) != 0)
    {
        std::cerr << "Listen failed\n";
        return 1;
    }

    return server_fd;
}

int main(int argc, char **argv)
{
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    int server_fd = get_server_socket();

    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    std::cout << "Server is listening on port " << PORT << std::endl;

    //int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    //std::cout << "Client connected\n";

    //int buffer_size = 1024;
    //char *buffer = new char[buffer_size];
    //if (recv(client_fd, buffer, buffer_size, 0) < 0)
    //{
    //    std::cerr << "Could not receive request";
    //    return 1;
    //}

    //std::cout << "Request:\n" << buffer << std::endl;
    //std::string request(buffer, buffer_size);
    //handle_request(client_fd, request);

    std::vector<pollfd> fds;
    fds.push_back({server_fd, POLLIN, 0});

    while (true) {
        if (poll(fds.data(), fds.size(), -1) < 0) {
            std::cerr << "poll error";
            return 1;
        }

        for (int i = 0; i < fds.size(); i++) {
            // Check for incoming connection on server
            if (fds[i].fd == server_fd && (fds[i].revents & POLLIN)) {
                int client_fd = accept(server_fd, nullptr, nullptr); 
                if (client_fd < 0) {
                    std::cerr << "accept failed";
                    return 1;
                } else {
                    std::cout << "New client connected: " << client_fd << std::endl;
                    fds.push_back({client_fd, POLLIN, 0});
                }
            } else if (fds[i].revents & POLLIN) {
                const int buffer_size = 1024;
                char buffer[buffer_size] = {0};
                int bytes = recv(fds[i].fd, buffer, buffer_size, 0);
                if (bytes > 0) {
                    std::cout << "Request from client " << fds[i].fd << ": " << buffer << std::endl;
                    std::string request(buffer, buffer_size);
                    handle_request(fds[i].fd, request);
                } else {
                    std::cout << "Connection closed from client " << fds[i].fd << std::endl;
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i);
                    i--;
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
