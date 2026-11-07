#pragma once

#include <string>

namespace ChatBot::Model
{
    enum class ChatRole
    {
        System,
        User,
        Assistant
    };

    struct ChatMessage
    {
        ChatRole role;
        std::string content;
    };

    std::string ToString(ChatRole role);
}
