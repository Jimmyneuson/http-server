#pragma once

#include "HttpMessage.h"
#include <string>
#include <map>

class HttpRequest : public HttpMessage
{
public:
    std::string method;
    std::string target;

    HttpRequest(std::string method, std::string target = "/", std::unordered_map<std::string, std::string> headers = {}, std::string body = "");
    static HttpRequest fromRequest(std::string request);
    std::string toString() const;
    bool has_encoding(std::string encoding);
    operator std::string() const;
    friend std::ostream& operator<<(std::ostream& os, const HttpRequest& request);
private:
    static std::string extract_directory(std::string request);
    static std::string extract_method(std::string request);
    static std::unordered_map<std::string, std::string> extract_headers(std::string request);
    static std::string extract_body(std::string request);
};
