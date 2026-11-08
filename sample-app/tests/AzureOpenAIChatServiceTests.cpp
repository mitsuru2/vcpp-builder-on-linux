#include <ChatBot/Core/AzureOpenAIChatService.h>

#include "../ChatBot.Core/src/WinHttpTransport.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>

namespace
{
    ChatBot::Core::Internal::HttpResponse nextResponse;

    void Check(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void CheckError(const ChatBot::Core::AzureOpenAIChatService& service,
        unsigned int status, const std::string& body, ChatBot::Model::ChatErrorCode code,
        const std::string& expectedDetails)
    {
        nextResponse = { status, body };
        const auto result = service.Send({ "hello", {} });
        const auto* error = std::get_if<ChatBot::Model::ChatError>(&result);
        Check(error != nullptr, "Expected an error");
        Check(error->code == code && error->httpStatus == status, "Incorrect error classification");
        Check(error->message.find("HTTP status " + std::to_string(status)) != std::string::npos,
            "Missing HTTP status");
        Check(error->message.find(expectedDetails) != std::string::npos, "Missing error details");
        Check(error->message.find("secret-key") == std::string::npos, "API key leaked");
    }
}

namespace ChatBot::Core::Internal
{
    HttpResponse PostJson(const std::wstring&, const std::string&, const std::string&, unsigned int)
    {
        return nextResponse;
    }
}

int main()
{
    try
    {
        ChatBot::Core::AzureOpenAIConfig config;
        config.endpoint = L"https://example.openai.azure.com";
        config.deploymentName = L"deployment";
        const ChatBot::Core::AzureOpenAIChatService service(config, "secret-key");

        CheckError(service, 404, R"({"error":{"code":"DeploymentNotFound","message":"No deployment"}})",
            ChatBot::Model::ChatErrorCode::Http, "DeploymentNotFound: No deployment");
        CheckError(service, 404, R"({"error":{"message":"\u65e5\u672c\u8a9e"}})",
            ChatBot::Model::ChatErrorCode::Http, "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e");
        CheckError(service, 401, R"({"error":{"message":"Rejected secret-key"}})",
            ChatBot::Model::ChatErrorCode::Authentication, "Rejected [redacted]");
        CheckError(service, 403, "", ChatBot::Model::ChatErrorCode::Authentication, "rejected");
        CheckError(service, 429, R"({"error":{"code":"RateLimitReached","message":"Try again"}})",
            ChatBot::Model::ChatErrorCode::RateLimited, "RateLimitReached: Try again");
        CheckError(service, 502, "Gateway error: secret-key",
            ChatBot::Model::ChatErrorCode::Http, "Gateway error: [redacted]");
        CheckError(service, 500, std::string("Invalid UTF-8: ") + char(0xff),
            ChatBot::Model::ChatErrorCode::Http, "Invalid UTF-8: \xef\xbf\xbd");
        CheckError(service, 400, R"({"unexpected":"response"})",
            ChatBot::Model::ChatErrorCode::Http, R"({"unexpected":"response"})");

        nextResponse = { 200, R"({"choices":[{"message":{"content":"Hello"}}]})" };
        const auto result = service.Send({ "hello", {} });
        const auto* response = std::get_if<ChatBot::Model::ChatResponse>(&result);
        Check(response != nullptr && response->content == "Hello", "Successful response changed");
        std::cout << "AzureOpenAIChatService tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
