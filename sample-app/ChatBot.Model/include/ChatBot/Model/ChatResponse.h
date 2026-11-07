#pragma once

#include <cstddef>
#include <string>

namespace ChatBot::Model
{
    struct TokenUsage
    {
        std::size_t promptTokens = 0;
        std::size_t completionTokens = 0;
        std::size_t totalTokens = 0;
    };

    struct ChatResponse
    {
        std::string content;
        TokenUsage usage;
    };
}
