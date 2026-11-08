#pragma once

#include <string>

namespace ChatBot::Core
{
    struct AzureOpenAIConfig
    {
        std::wstring endpoint;
        std::wstring deploymentName;
        std::wstring apiVersion = L"2024-10-21";
        unsigned int timeoutSeconds = 60;
    };
}
