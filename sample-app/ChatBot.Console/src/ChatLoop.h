#pragma once

#include <ChatBot/Core/IChatService.h>

namespace ChatBot::ConsoleApp
{
    void ConfigureOutputStreams();
    int RunChatLoop(const Core::IChatService& chatService);
}
