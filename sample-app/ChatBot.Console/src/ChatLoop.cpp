#include "ChatLoop.h"

#include <ChatBot/Model/ChatMessage.h>
#include <ChatBot/Model/ChatRequest.h>

#include <iostream>
#include <string>
#include <vector>
#include <cwctype>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <stdexcept>
#include <windows.h>

namespace
{
    std::string ToUtf8(const std::wstring& value)
    {
        if (value.empty())
        {
            return {};
        }
        const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (length <= 0)
        {
            throw std::runtime_error("Unable to encode console input as UTF-8.");
        }
        std::string result(static_cast<std::size_t>(length), '\0');
        if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
            result.data(), length, nullptr, nullptr) != length)
        {
            throw std::runtime_error("Unable to encode console input as UTF-8.");
        }
        return result;
    }

    std::wstring FromUtf8(const std::string& value)
    {
        if (value.empty())
        {
            return {};
        }
        const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), nullptr, 0);
        if (length <= 0)
        {
            throw std::runtime_error("The response was not valid UTF-8.");
        }
        std::wstring result(static_cast<std::size_t>(length), L'\0');
        if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), length) != length)
        {
            throw std::runtime_error("The response was not valid UTF-8.");
        }
        return result;
    }

    bool IsExitCommand(const std::wstring& input)
    {
        constexpr wchar_t exitText[] = L"exit";
        if (input.size() != 4)
        {
            return false;
        }
        for (std::size_t index = 0; index < input.size(); ++index)
        {
            if (std::towlower(input[index]) != exitText[index])
            {
                return false;
            }
        }
        return true;
    }
}

namespace ChatBot::ConsoleApp
{
    void ConfigureConsoleStreams()
    {
        const auto configure = [](FILE* stream)
        {
            const int descriptor = _fileno(stream);
            const std::intptr_t handle = _get_osfhandle(descriptor);
            DWORD mode = 0;
            const bool isConsole = handle != -1 &&
                GetConsoleMode(reinterpret_cast<HANDLE>(handle), &mode) != FALSE;
            if (_setmode(descriptor, isConsole ? _O_U16TEXT : _O_U8TEXT) == -1)
            {
                throw std::runtime_error("Unable to configure Unicode console streams.");
            }
        };
        configure(stderr);
        configure(stdout);
        configure(stdin);
    }

    int RunChatLoop(const Core::IChatService& chatService)
    {
        std::vector<Model::ChatMessage> history;
        std::wstring input;

        for (;;)
        {
            std::wcout << L"INPUT: " << std::flush;
            if (!std::wcout)
            {
                throw std::runtime_error("Unable to write console output.");
            }
            if (!std::getline(std::wcin, input))
            {
                if (std::wcin.bad() || !std::wcin.eof())
                {
                    throw std::runtime_error("Unable to read console input.");
                }
                std::wcout << L"\n";
                return 0;
            }
            if (IsExitCommand(input))
            {
                return 0;
            }
            if (input.empty())
            {
                continue;
            }

            const std::string utf8Input = ToUtf8(input);
            const Model::ChatRequest request{ utf8Input, history };
            const Model::ChatResult result = chatService.Send(request);
            if (const auto* response = std::get_if<Model::ChatResponse>(&result))
            {
                std::wcout << L"> " << FromUtf8(response->content) << L"\n\n";
                history.push_back({ Model::ChatRole::User, utf8Input });
                history.push_back({ Model::ChatRole::Assistant, response->content });
            }
            else
            {
                const auto& error = std::get<Model::ChatError>(result);
                std::wcerr << L"Error: " << FromUtf8(error.message) << L"\n\n";
            }
            if (!std::wcout || !std::wcerr)
            {
                throw std::runtime_error("Unable to write console output.");
            }
        }
    }
}
