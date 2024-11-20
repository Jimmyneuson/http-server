#include "HttpResponse.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/socket.h>
#include <iostream>

HttpResponse::HttpResponse(std::string status, std::unordered_map<std::string, std::string> headers, std::string body)
{
	this->status = status;
    this->headers = headers;
    this->body = body;
}

std::string HttpResponse::toString() const
{
	std::string response = PROTOCOL + " ";
	response += status;
	response += "\r\n";
	for (const auto& header : headers)
	{
		response += header.first + ": " + header.second + "\r\n";
	}
	response += "\r\n";
	response += body;

	return response;
}

void HttpResponse::send_to(int client_fd) const
{
	if (send(client_fd, toString().c_str(), toString().size(), 0) == -1)
    {
    	std::cerr << "Could not send response";
    }
}

HttpResponse::operator std::string() const {
	return HttpResponse::toString();
}

std::ostream& operator<<(std::ostream& os, const HttpResponse& response)
{
	os << response.toString();
	return os;
}
