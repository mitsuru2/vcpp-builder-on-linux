#pragma once

#include <string>

namespace ChatBot::Core
{
    struct AzureOpenAIConfig
    {
        std::wstring endpoint;
        std::wstring deploymentName;
        std::wstring apiVersion = L"2021-04-30";
        unsigned int timeoutSeconds = 60;
    };
}
