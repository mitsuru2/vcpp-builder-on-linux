#pragma once

#include <string>
#include <windows.h>
#include <winhttp.h>

namespace ChatBot::Core::Internal
{
    struct HttpResponse
    {
        DWORD statusCode = 0;
        std::string body;
    };

    HttpResponse PostJson(
        const std::wstring& url,
        const std::string& apiKey,
        const std::string& body,
        unsigned int timeoutSeconds);
}
