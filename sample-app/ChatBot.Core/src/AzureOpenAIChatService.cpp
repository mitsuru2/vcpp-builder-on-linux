#include <ChatBot/Core/AzureOpenAIChatService.h>

#include "WinHttpTransport.h"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <utility>

namespace
{
    using Json = nlohmann::json;

    std::wstring EncodePathSegment(const std::wstring& value)
    {
        std::wstring encoded;
        for (const wchar_t character : value)
        {
            if ((character >= L'a' && character <= L'z') ||
                (character >= L'A' && character <= L'Z') ||
                (character >= L'0' && character <= L'9') ||
                character == L'-' || character == L'_' || character == L'.' || character == L'~')
            {
                encoded.push_back(character);
            }
            else
            {
                constexpr wchar_t hex[] = L"0123456789ABCDEF";
                encoded.push_back(L'%');
                encoded.push_back(hex[(character >> 4) & 0xF]);
                encoded.push_back(hex[character & 0xF]);
            }
        }
        return encoded;
    }

    std::wstring BuildEndpoint(const ChatBot::Core::AzureOpenAIConfig& config)
    {
        std::wstring endpoint = config.endpoint;
        while (!endpoint.empty() && endpoint.back() == L'/')
        {
            endpoint.pop_back();
        }
        return endpoint + L"/openai/deployments/" + EncodePathSegment(config.deploymentName) +
            L"/chat/completions?api-version=" + EncodePathSegment(config.apiVersion);
    }

    ChatBot::Model::ChatError MakeError(ChatBot::Model::ChatErrorCode code,
        std::string message, unsigned int status = 0)
    {
        return { code, std::move(message), status };
    }

    std::size_t ReadSize(const Json& value, const char* key)
    {
        const auto iterator = value.find(key);
        if (iterator == value.end() ||
            (!iterator->is_number_unsigned() && !iterator->is_number_integer()))
        {
            return 0;
        }
        const auto parsed = iterator->get<long long>();
        return parsed < 0 ? 0 : static_cast<std::size_t>(parsed);
    }
}

namespace ChatBot::Core
{
    AzureOpenAIChatService::AzureOpenAIChatService(AzureOpenAIConfig config, std::string apiKey)
        : config_(std::move(config)), apiKey_(std::move(apiKey))
    {
    }

    Model::ChatResult AzureOpenAIChatService::Send(const Model::ChatRequest& request) const
    {
        if (config_.endpoint.empty() || config_.deploymentName.empty() || apiKey_.empty())
        {
            return MakeError(Model::ChatErrorCode::Configuration,
                "Azure OpenAI endpoint, deployment, and API key must be configured.");
        }
        if (request.input.empty())
        {
            return MakeError(Model::ChatErrorCode::Configuration, "Chat input cannot be empty.");
        }

        try
        {
            Json messages = Json::array();
            messages.push_back({ {"role", "system"}, {"content", "You are a helpful assistant. Reply naturally and concisely."} });
            for (const auto& message : request.history)
            {
                messages.push_back({ {"role", Model::ToString(message.role)}, {"content", message.content} });
            }
            messages.push_back({ {"role", "user"}, {"content", request.input} });

            const std::string body = Json{ {"messages", std::move(messages)} }.dump();
            const std::wstring url = BuildEndpoint(config_);
            const auto response = Internal::PostJson(url, apiKey_, body, config_.timeoutSeconds);

            if (response.statusCode < 200 || response.statusCode >= 300)
            {
                if (response.statusCode == 401 || response.statusCode == 403)
                {
                    return MakeError(Model::ChatErrorCode::Authentication,
                        "Azure OpenAI rejected the API key or access.", response.statusCode);
                }
                if (response.statusCode == 429)
                {
                    return MakeError(Model::ChatErrorCode::RateLimited,
                        "Azure OpenAI rate limit reached. Try again shortly.", response.statusCode);
                }
                return MakeError(Model::ChatErrorCode::Http,
                    "Azure OpenAI returned HTTP status " + std::to_string(response.statusCode) + ".",
                    response.statusCode);
            }

            const Json payload = Json::parse(response.body);
            if (!payload.contains("choices") || !payload["choices"].is_array() ||
                payload["choices"].empty() || !payload["choices"][0].contains("message") ||
                !payload["choices"][0]["message"].contains("content") ||
                !payload["choices"][0]["message"]["content"].is_string())
            {
                return MakeError(Model::ChatErrorCode::InvalidResponse,
                    "Azure OpenAI response did not contain a message.");
            }

            Model::ChatResponse result;
            result.content = payload["choices"][0]["message"]["content"].get<std::string>();
            if (payload.contains("usage") && payload["usage"].is_object())
            {
                result.usage.promptTokens = ReadSize(payload["usage"], "prompt_tokens");
                result.usage.completionTokens = ReadSize(payload["usage"], "completion_tokens");
                result.usage.totalTokens = ReadSize(payload["usage"], "total_tokens");
            }
            return result;
        }
        catch (const nlohmann::json::parse_error&)
        {
            return MakeError(Model::ChatErrorCode::InvalidResponse, "Azure OpenAI returned invalid JSON.");
        }
        catch (const nlohmann::json::exception&)
        {
            return MakeError(Model::ChatErrorCode::InvalidResponse, "Azure OpenAI returned an invalid response.");
        }
        catch (const std::invalid_argument& error)
        {
            return MakeError(Model::ChatErrorCode::Configuration, error.what());
        }
        catch (const std::runtime_error&)
        {
            return MakeError(Model::ChatErrorCode::Network,
                "Unable to communicate with Azure OpenAI. Check network connectivity and configuration.");
        }
    }
}
