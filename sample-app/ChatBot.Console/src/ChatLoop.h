#pragma once

#include <ChatBot/Core/IChatService.h>

namespace ChatBot::ConsoleApp
{
    void ConfigureConsoleStreams();
    int RunChatLoop(const Core::IChatService& chatService);
}
