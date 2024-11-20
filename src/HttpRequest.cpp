#include "HttpRequest.h"

HttpRequest::HttpRequest(std::string method, std::string target, std::unordered_map<std::string, std::string> headers, std::string body)
{
	this->method = method;
    this->target = target;
    this->headers = headers;
    this->body = body;
}

HttpRequest HttpRequest::fromRequest(std::string request)
{
	return HttpRequest(extract_method(request), extract_directory(request), extract_headers(request), extract_body(request));
}

std::string HttpRequest::toString() const
{
	std::string request = method;
	request += target + " " + PROTOCOL + "\r\n";
	for (const auto& header : headers)
	{
		request += header.first + ": " + header.second + "\r\n";
	}
	request += "\r\n";
	request += body;

	return request;
}

bool HttpRequest::has_encoding(std::string encoding)
{
	std::string str = headers["Accept-Encoding"];
	if (str.find(",") == std::string::npos)
	{
		return str == encoding;
	}

	int start = 0;

	while (true)
	{
		if (str.substr(start, str.find(",", start) - start) == encoding)
		{
			return true;
		} 
		else
		{
			if (str.find(",", start) == std::string::npos)
			{
				return false;
			}
			start = str.find(",", start) + 2;
		}
	}

	return false;
}

HttpRequest::operator std::string() const
{
	return HttpRequest::toString();
}

std::ostream& operator<<(std::ostream& os, const HttpRequest& request)
{
	os << request.toString();
	return os;
}

std::string HttpRequest::extract_directory(std::string request)
{
	int start = request.find(" ") + 1;
	int end = request.find(" ", start);
	return request.substr(start, end - start);
}

std::string HttpRequest::extract_method(std::string request)
{
	if (request.starts_with(GET)) 
	{
		return GET;
	} 
	else if (request.starts_with(POST))
	{
		return POST;
	}

	return "Invalid Method";
}

std::unordered_map<std::string, std::string> HttpRequest::extract_headers(std::string request)
{
	std::unordered_map<std::string, std::string> headers;
	int start = request.find("\r\n") + 2;
	int end = request.find("\r\n\r\n");

	while (start < end)
	{
		int lineEnd = request.find("\r\n", start);
		if (lineEnd == std::string::npos || lineEnd > end) break;
		int colon = request.find(":", start);
		if (colon == std::string::npos || colon > lineEnd) break;

		std::string header = request.substr(start, colon - start);
		std::string value = request.substr(colon + 2, lineEnd - (colon + 2));

		headers[header] = value;

		start = lineEnd + 2;
	}

	return headers;
}

std::string HttpRequest::extract_body(std::string request)
{
	int start = request.rfind("\r\n");
	std::string body = request.substr(start+2);
	return body;
}
