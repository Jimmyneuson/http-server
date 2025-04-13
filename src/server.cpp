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
#include <fstream>
#include <thread>
#include <zlib.h>
#include <map>

#include "HttpMessage.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

const int PORT = 80;
const int BACKLOG = 32;

const HttpResponse RESPONSE_200(OK, {
    {"Content-Type", "text/plain"},
    {"Content-Length", "0"},
});
const HttpResponse RESPONSE_404(NOT_FOUND, {
    {"Content-Type", "text/plain"},
    {"Content-Length", "0"},
});

HttpResponse plain_text_response(std::string body)
{
    HttpResponse response(OK, {
            {"Content-Type", "text/plain"},
            {"Content-Length", std::to_string(body.length())},
            {"Connection", "keep-alive"}
    }, body);

    return response;
}

HttpResponse file_response(std::string filename, std::string working_directory)
{
    std::string file_path = working_directory + filename;
    std::ifstream file(file_path);
    if (!file) {
        return RESPONSE_404;
    }

    std::string file_content((std::istreambuf_iterator<char>(file)),
                             ( std::istreambuf_iterator<char>()));

    HttpResponse response(OK, {
        {"Content-Type", "application/octet-stream"},
        {"Content-Length", std::to_string(file_content.length())}
    }, file_content);

    file.close();

    return response;
}

void write_file(std::string filename, std::string content, std::string working_directory)
{
    std::string file_path = working_directory + filename;

    std::ofstream file(file_path);
    if (!file)
    {
        std::cerr << "Could not open file";
    }
    file << content;
    file.close();
}


std::string gzip(std::string str)
{
    // track compression state
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    // initialize DEFLATE data with gzip headers
    if (deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY) != Z_OK)
    {
        std::cerr << "could not compress";
    }

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();
    int ret;
    char outbuffer[32768];
    std::string outstring;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);
        ret = deflate(&zs, Z_FINISH);
        if (outstring.size() < zs.total_out) {
            outstring.append(outbuffer, zs.total_out - outstring.size());
        }

    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        std::cerr << "could not compress";
    }

    return outstring;
}

void handle_request(int client_fd, std::string working_directory)
{
    const int buffer_size = 1024;
    char buffer[buffer_size] = {0};

    int bytes = recv(client_fd, buffer, buffer_size, 0);
    while (bytes > 0) 
    {
        std::cout << "Request from client " << client_fd << ":\n" << buffer << std::endl;
        std::string request_data(buffer, bytes);
        HttpRequest request = HttpRequest::fromRequest(request_data);

        if (request.headers["Connection"] == "close") break;

        if (request.target == "/")
        {
            RESPONSE_200.send_to(client_fd);
        }
        else if (request.target.starts_with("/echo/"))
        {
            HttpResponse response;
            std::string body = request.target.substr(6);

            if (request.has_encoding("gzip")) {
                std::string compressed_data = gzip(body);
                response.headers = {
                    {"Content-Encoding", "gzip"},
                    {"Content-Type", "text/plain"},
                    {"Content-Length", std::to_string(compressed_data.size())}
                };
                response.body = compressed_data;
            } 
            else 
            {
                response = plain_text_response(body);
            }

            response.send_to(client_fd);
        }
        else if (request.target.starts_with("/user-agent"))
        {
            std::string body = request.headers["User-Agent"];
            HttpResponse response = plain_text_response(body);
            response.send_to(client_fd);
        } 
        else if (request.target.starts_with("/files/"))
        {
            std::string filename = request.target.substr(7);
            HttpResponse response;

            if (request.method == GET)
            {
                response = file_response(filename, working_directory);
            } 
            else if (request.method == POST)
            {
                response = HttpResponse(CREATED, {
                    {"Content-Type", "text/plain"},
                    {"Content-Length", "0"}
                });
                write_file(filename, request.body, working_directory);
            }

            response.send_to(client_fd);
        }
        else
        {
            RESPONSE_404.send_to(client_fd);
        }

        bytes = recv(client_fd, buffer, buffer_size, 0);
    } 

    HttpResponse close_response(OK, {
        {"Content-Type", "text/plain"},
        {"Content-Length", "0"},
        {"Connection", "close"} 
    });
    close_response.send_to(client_fd);
    std::cout << "Connection closed from client " << client_fd << std::endl;
    close(client_fd);
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
    std::string working_directory;
    if (argc > 2 && std::string(argv[1]) == "--directory")
    {
        working_directory = argv[2];
    } 
    else if (argc > 2)
    {
        std::cerr << "Usage: ./server --directory <path_to_directory>\n";
        return 1;
    }

    int server_fd = get_server_socket();
    if (server_fd == 1)
    {
        return server_fd;
    }

    std::cout << "Server is listening on port " << PORT << std::endl;

    while (true) 
    {
        int client_fd = accept(server_fd, nullptr, nullptr); 
        if (client_fd < 0)
        {
            std::cerr << "accept failed";
            return 1;
        }
        else 
        {
            std::cout << "New client connected: " << client_fd << std::endl;
        }

        std::thread thread_obj(handle_request, client_fd, working_directory);
        thread_obj.detach();
    }

    close(server_fd);
    return 0;
}
