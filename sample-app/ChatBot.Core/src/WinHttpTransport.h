#pragma once

#include <string>

namespace ChatBot::Core::Internal
{
    struct HttpResponse
    {
        unsigned int statusCode = 0;
        std::string body;
    };

    HttpResponse PostJson(
        const std::wstring& url,
        const std::string& apiKey,
        const std::string& body,
        unsigned int timeoutSeconds);
}
