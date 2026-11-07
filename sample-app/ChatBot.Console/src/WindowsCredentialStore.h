#pragma once

#include <optional>
#include <string>

namespace ChatBot::ConsoleApp
{
    class WindowsCredentialStore
    {
    public:
        static std::optional<std::string> ReadAzureOpenAIApiKey();
    };
}
