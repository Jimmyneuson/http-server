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
#include <fstream>
#include <thread>

const int PORT = 80;
const int BACKLOG = 32;
const std::string RESPONSE_200 = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n";
const std::string RESPONSE_404 = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n";

std::string working_directory;

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
    response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
    response += "\r\n";
    response += body;
    return response;
}

std::string file_response(std::string filename)
{
    std::string file_path = working_directory.substr(1) + filename;
    std::ifstream file(file_path);
    if (!file) {
        std::cout << "test" << std::endl;
        return RESPONSE_404;
    }

    std::string file_content((std::istreambuf_iterator<char>(file) ),
                       ( std::istreambuf_iterator<char>()     ));

    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: application/octet-stream\r\n";
    response += "Content-Length: " + std::to_string(sizeof(file_content)) + "\r\n";
    response += "\r\n";
    response += file_content;

    file.close();

    return response;
}

std::string extract_header(std::string request, std::string header)
{
    int start = request.find(header);
    int end = request.find("\r\n", start);
    int len = header.length() + 2;
    std::string value = request.substr(start + len, end - (start + len));
    return value;
}

std::string get_method(std::string request) 
{
    if (request.starts_with("GET")) 
    {
        return "GET";
    } else if (request.starts_with("POST"))
    {
        return "POST";
    }
    return "NONE";
}

void write_file(std::string filename, std::string content)
{
    std::string file_path = working_directory.substr(1) + filename;
    std::ofstream file(file_path);
    if (!file)
    {
        std::cerr << "Could not open file";
    }
    file << content;
    file.close();
}

void handle_request(int client_fd) 
{
    const int buffer_size = 1024;
    char buffer[buffer_size] = {0};

    int bytes = recv(client_fd, buffer, buffer_size, 0);
    if (bytes > 0) 
    {
        std::cout << "Request from client " << client_fd << ":\n" << buffer << std::endl;
    } else 
    {
        std::cout << "Connection closed from client " << client_fd << std::endl;
        close(client_fd);
    }

    std::string request(buffer, bytes);
    std::string method = get_method(request);
    std::string dir = parse_directory(request);

    if (dir == "/")
    {
        if (send(client_fd, RESPONSE_200.c_str(), RESPONSE_200.size(), 0) == -1)
        {
            std::cerr << "Could not send response";
        }
    }
    else if (dir.starts_with("/echo/"))
    {
        std::string body = dir.substr(6);
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
    } else if (dir.starts_with("/files/"))
    {
        std::string filename = dir.substr(7);
        if (method == "GET")
        {
            std::string response = file_response(filename);
            if (send(client_fd, response.c_str(), response.size(), 0) < 0) 
            {
                std::cerr << "Could not send response";
            }
        } else if (method == "POST")
        {
            int start = request.rfind("\r\n");
            std::string body = request.substr(start+2);
            std::string response = "HTTP/1.1 201 Created\r\n\r\n";
            write_file(filename, body);
            if (send(client_fd, response.c_str(), response.size(), 0) < 0)
            {
                std::cerr << "Could not send response";
            }
        }
    }
    else
    {
        if (send(client_fd, RESPONSE_404.c_str(), RESPONSE_404.size(), 0) == -1)
        {
            std::cerr << "Could not send response";
        }
    }
}

int get_server_socket() 
{
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
    if (argc > 2 && std::string(argv[1]) == "--directory")
    {
        working_directory = argv[2];
    } else if (argc > 2)
    {
        std::cerr << "Usage: ./server --directory <path_to_directory>\n";
        return 1;
    }

    int server_fd = get_server_socket();
    std::cout << "Server is listening on port " << PORT << std::endl;

    while (true) 
    {
        int client_fd = accept(server_fd, nullptr, nullptr); 
        if (client_fd < 0) 
        {
            std::cerr << "accept failed";
            return 1;
        } else 
        {
            std::cout << "New client connected: " << client_fd << std::endl;
        }

        std::thread thread_obj(handle_request, client_fd);
        thread_obj.detach();
    }

    close(server_fd);
    return 0;
}
