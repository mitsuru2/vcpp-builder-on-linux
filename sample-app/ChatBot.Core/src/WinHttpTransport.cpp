#include "WinHttpTransport.h"

#include <windows.h>
#include <winhttp.h>

#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
    class WinHttpHandle
    {
    public:
        explicit WinHttpHandle(HINTERNET handle = nullptr) : handle_(handle) {}
        ~WinHttpHandle() { if (handle_ != nullptr) WinHttpCloseHandle(handle_); }
        WinHttpHandle(const WinHttpHandle&) = delete;
        WinHttpHandle& operator=(const WinHttpHandle&) = delete;
        HINTERNET get() const { return handle_; }

    private:
        HINTERNET handle_;
    };

    std::wstring Utf8ToWide(const std::string& value)
    {
        if (value.empty())
        {
            return {};
        }

        const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), nullptr, 0);
        if (length <= 0)
        {
            throw std::runtime_error("Unable to decode UTF-8 request data.");
        }

        std::wstring result(static_cast<std::size_t>(length), L'\0');
        if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), length) != length)
        {
            throw std::runtime_error("Unable to decode UTF-8 request data.");
        }
        return result;
    }

    void Check(bool success, const char* message)
    {
        if (!success)
        {
            throw std::runtime_error(message);
        }
    }
}

namespace ChatBot::Core::Internal
{
    HttpResponse PostJson(const std::wstring& url, const std::string& apiKey,
        const std::string& body, unsigned int timeoutSeconds)
    {
        if (apiKey.find_first_of("\r\n") != std::string::npos)
        {
            throw std::runtime_error("Azure OpenAI API key contains invalid header characters.");
        }
        if (body.size() > MAXDWORD)
        {
            throw std::runtime_error("Azure OpenAI request is too large.");
        }

        URL_COMPONENTS parts{};
        parts.dwStructSize = sizeof(parts);
        parts.dwSchemeLength = static_cast<DWORD>(-1);
        parts.dwHostNameLength = static_cast<DWORD>(-1);
        parts.dwUrlPathLength = static_cast<DWORD>(-1);
        parts.dwExtraInfoLength = static_cast<DWORD>(-1);
        Check(WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &parts) != FALSE,
            "Unable to parse Azure OpenAI endpoint.");
        Check(parts.nScheme == INTERNET_SCHEME_HTTPS, "Azure OpenAI endpoint must use HTTPS.");

        const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
        std::wstring path(parts.lpszUrlPath, parts.dwUrlPathLength);
        if (parts.dwExtraInfoLength > 0)
        {
            path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
        }

        WinHttpHandle session(WinHttpOpen(L"SmallChatBot/1.0",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0));
        Check(session.get() != nullptr, "Unable to initialize Windows HTTP.");

        const DWORD timeoutMs = timeoutSeconds > MAXDWORD / 1000 ? MAXDWORD : timeoutSeconds * 1000;
        Check(WinHttpSetTimeouts(session.get(), timeoutMs, timeoutMs, timeoutMs, timeoutMs) != FALSE,
            "Unable to configure HTTP timeout.");

        WinHttpHandle connection(WinHttpConnect(session.get(), host.c_str(), parts.nPort, 0));
        Check(connection.get() != nullptr, "Unable to connect to Azure OpenAI.");

        WinHttpHandle request(WinHttpOpenRequest(connection.get(), L"POST", path.c_str(),
            nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE));
        Check(request.get() != nullptr, "Unable to create Azure OpenAI request.");

        const std::wstring wideKey = Utf8ToWide(apiKey);
        const std::wstring headers = L"Content-Type: application/json\r\napi-key: " + wideKey + L"\r\n";
        Check(WinHttpSendRequest(request.get(), headers.c_str(),
            static_cast<DWORD>(headers.size()), const_cast<char*>(body.data()),
            static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0) != FALSE,
            "Azure OpenAI request could not be sent.");
        Check(WinHttpReceiveResponse(request.get(), nullptr) != FALSE,
            "Azure OpenAI response could not be received.");

        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        Check(WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX) != FALSE,
            "Unable to read Azure OpenAI response status.");

        std::string responseBody;
        for (;;)
        {
            DWORD available = 0;
            if (!WinHttpQueryDataAvailable(request.get(), &available))
            {
                throw std::runtime_error("Unable to read Azure OpenAI response.");
            }
            if (available == 0)
            {
                break;
            }

            std::vector<char> buffer(available);
            DWORD read = 0;
            if (!WinHttpReadData(request.get(), buffer.data(), available, &read))
            {
                throw std::runtime_error("Unable to read Azure OpenAI response.");
            }
            responseBody.append(buffer.data(), read);
        }

        return { static_cast<unsigned int>(statusCode), std::move(responseBody) };
    }
}
