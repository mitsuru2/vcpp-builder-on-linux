#pragma once

#include "ChatMessage.h"

#include <string>
#include <vector>

namespace ChatBot::Model
{
    struct ChatRequest
    {
        std::string input;
        std::vector<ChatMessage> history;
    };
}
