#include "WindowsCredentialStore.h"
#include "ChatLoop.h"

#include <ChatBot/Core/AzureOpenAIChatService.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

namespace
{
    std::wstring ReadEnvironmentVariable(const wchar_t* name)
    {
        const wchar_t* value = _wgetenv(name);
        return value == nullptr ? std::wstring{} : std::wstring(value);
    }
}

int wmain()
{
    const auto apiKey = ChatBot::ConsoleApp::WindowsCredentialStore::ReadAzureOpenAIApiKey();
    if (!apiKey.has_value())
    {
        std::wcerr << L"Error: Azure OpenAI API key was not found in Windows Credential Manager.\n"
            << L"Create a Generic Credential with target \"SmallChatBot/AzureOpenAI\" and store the API key as its password.\n";
        return 1;
    }

    ChatBot::Core::AzureOpenAIConfig config;
    config.endpoint = ReadEnvironmentVariable(L"SMALLCHATBOT_AZURE_ENDPOINT");
    config.deploymentName = ReadEnvironmentVariable(L"SMALLCHATBOT_AZURE_DEPLOYMENT");
    const std::wstring apiVersion = ReadEnvironmentVariable(L"SMALLCHATBOT_AZURE_API_VERSION");
    if (!apiVersion.empty())
    {
        config.apiVersion = apiVersion;
    }

    if (config.endpoint.empty() || config.deploymentName.empty())
    {
        std::wcerr << L"Error: Set SMALLCHATBOT_AZURE_ENDPOINT and SMALLCHATBOT_AZURE_DEPLOYMENT before starting.\n";
        return 1;
    }

    ChatBot::Core::AzureOpenAIChatService service(std::move(config), *apiKey);
    return ChatBot::ConsoleApp::RunChatLoop(service);
}
