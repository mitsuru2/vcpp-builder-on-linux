#include <ChatBot/Model/ChatMessage.h>

#include <stdexcept>

namespace ChatBot::Model
{
    std::string ToString(ChatRole role)
    {
        switch (role)
        {
        case ChatRole::System: return "system";
        case ChatRole::User: return "user";
        case ChatRole::Assistant: return "assistant";
        default: throw std::invalid_argument("Unsupported chat message role.");
        }
    }
}
