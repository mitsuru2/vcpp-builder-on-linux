#pragma once

#include <ChatBot/Model/ChatRequest.h>
#include <ChatBot/Model/ChatResult.h>

namespace ChatBot::Core
{
    class IChatService
    {
    public:
        virtual ~IChatService() = default;
        virtual Model::ChatResult Send(const Model::ChatRequest& request) const = 0;
    };
}
