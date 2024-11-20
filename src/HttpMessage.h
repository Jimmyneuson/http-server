#pragma once

#include <string>
#include <map>

extern const std::string PROTOCOL;
extern const std::string OK;
extern const std::string NOT_FOUND;
extern const std::string CREATED;
extern const std::string GET;
extern const std::string POST;

class HttpMessage
{
public:
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};
