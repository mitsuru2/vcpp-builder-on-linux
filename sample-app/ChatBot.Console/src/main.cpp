#include "WindowsCredentialStore.h"
#include "ChatLoop.h"

#include <ChatBot/Core/AzureOpenAIChatService.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

namespace
{
    std::wstring ReadEnvironmentVariable(const wchar_t* name, const wchar_t* defaultValue)
    {
        const wchar_t* value = _wgetenv(name);
        return value == nullptr || *value == L'\0' ? std::wstring(defaultValue) : std::wstring(value);
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
    config.endpoint = ReadEnvironmentVariable(
        L"SMALLCHATBOT_AZURE_ENDPOINT", L"https://cemkl5kd55yuk-openai.openai.azure.com/");
    config.deploymentName = ReadEnvironmentVariable(
        L"SMALLCHATBOT_AZURE_DEPLOYMENT", L"gpt-5.4-nano");
    const std::wstring apiVersion = ReadEnvironmentVariable(
        L"SMALLCHATBOT_AZURE_API_VERSION", config.apiVersion.c_str());
    if (!apiVersion.empty())
    {
        config.apiVersion = apiVersion;
    }

    ChatBot::Core::AzureOpenAIChatService service(std::move(config), *apiKey);
    return ChatBot::ConsoleApp::RunChatLoop(service);
}
