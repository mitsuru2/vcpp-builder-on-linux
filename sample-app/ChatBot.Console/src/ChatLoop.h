#pragma once

#include <ChatBot/Core/IChatService.h>

namespace ChatBot::ConsoleApp
{
    int RunChatLoop(const Core::IChatService& chatService);
}
