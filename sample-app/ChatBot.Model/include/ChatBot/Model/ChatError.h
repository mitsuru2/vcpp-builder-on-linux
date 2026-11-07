#pragma once

#include <string>

namespace ChatBot::Model
{
    enum class ChatErrorCode
    {
        Configuration,
        Authentication,
        RateLimited,
        Http,
        Network,
        InvalidResponse
    };

    struct ChatError
    {
        ChatErrorCode code;
        std::string message;
        unsigned int httpStatus = 0;
    };
}
