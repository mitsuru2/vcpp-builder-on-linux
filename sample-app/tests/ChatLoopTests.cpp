#include "../ChatBot.Console/src/ChatLoop.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace
{
    class FakeChatService final : public ChatBot::Core::IChatService
    {
    public:
        ChatBot::Model::ChatResult Send(const ChatBot::Model::ChatRequest& request) const override
        {
            if (request.input == "first" && request.history.empty())
            {
                return ChatBot::Model::ChatResponse{ "I\xe2\x80\x99m \xe6\x97\xa5\xe6\x9c\xac\xf0\x9f\x98\x80", {} };
            }
            if (request.input == "second" && request.history.size() == 2)
            {
                return ChatBot::Model::ChatResponse{ "Second response", {} };
            }
            throw std::runtime_error("Unexpected request or conversation history.");
        }
    };
}

int main()
{
    try
    {
        ChatBot::ConsoleApp::ConfigureOutputStreams();
        std::wistringstream input(L"first\nsecond\nexit\n");
        auto* const originalBuffer = std::wcin.rdbuf(input.rdbuf());
        const int result = ChatBot::ConsoleApp::RunChatLoop(FakeChatService{});
        std::wcin.rdbuf(originalBuffer);
        return result;
    }
    catch (const std::exception& error)
    {
        std::wcerr << L"Test failed: " << error.what() << L'\n';
        return 1;
    }
}
