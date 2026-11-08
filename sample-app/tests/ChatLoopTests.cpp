#include "../ChatBot.Console/src/ChatLoop.h"

#include <iostream>
#include <stdexcept>

namespace
{
    class FakeChatService final : public ChatBot::Core::IChatService
    {
    public:
        ChatBot::Model::ChatResult Send(const ChatBot::Model::ChatRequest& request) const override
        {
            if (request.input == "\xe3\x81\x93\xe3\x82\x93\xe3\x81\xab\xe3\x81\xa1\xe3\x81\xaf \xf0\x9f\x98\x80" &&
                request.history.empty())
            {
                ++count_;
                return ChatBot::Model::ChatResponse{ "I\xe2\x80\x99m \xe6\x97\xa5\xe6\x9c\xac\xf0\x9f\x98\x80", {} };
            }
            if (request.input == "second" && request.history.size() == 2 &&
                request.history[0].content == "\xe3\x81\x93\xe3\x82\x93\xe3\x81\xab\xe3\x81\xa1\xe3\x81\xaf \xf0\x9f\x98\x80")
            {
                ++count_;
                return ChatBot::Model::ChatResponse{ "Second response", {} };
            }
            throw std::runtime_error("Unexpected request or conversation history.");
        }

        int Count() const { return count_; }

    private:
        mutable int count_ = 0;
    };
}

int main()
{
    try
    {
        ChatBot::ConsoleApp::ConfigureConsoleStreams();
        const FakeChatService service;
        const int result = ChatBot::ConsoleApp::RunChatLoop(service);
        if (service.Count() != 2)
        {
            throw std::runtime_error("Expected two requests.");
        }
        return result;
    }
    catch (const std::exception& error)
    {
        std::wcerr << L"Test failed: " << error.what() << L'\n';
        return 1;
    }
}
