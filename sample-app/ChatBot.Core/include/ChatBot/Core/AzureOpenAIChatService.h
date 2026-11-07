#pragma once

#include "AzureOpenAIConfig.h"
#include "IChatService.h"

#include <memory>
#include <string>

namespace ChatBot::Core
{
    class AzureOpenAIChatService final : public IChatService
    {
    public:
        AzureOpenAIChatService(AzureOpenAIConfig config, std::string apiKey);
        Model::ChatResult Send(const Model::ChatRequest& request) const override;

    private:
        AzureOpenAIConfig config_;
        std::string apiKey_;
    };
}
