#pragma once

#include "HttpMessage.h"
#include <string>
#include <map>

class HttpResponse : public HttpMessage
{
public:
    HttpResponse(std::string status = OK, std::unordered_map<std::string, std::string> headers = {}, std::string body = "");
    std::string toString() const;
    void send_to(int client_fd) const;
    operator std::string() const;
    friend std::ostream& operator<<(std::ostream& os, const HttpResponse& response);
private:
    std::string status;
};
