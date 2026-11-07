#include "WindowsCredentialStore.h"

#include <windows.h>
#include <wincred.h>

#include <string_view>

namespace
{
    constexpr wchar_t CredentialTarget[] = L"SmallChatBot/AzureOpenAI";

    std::optional<std::string> DecodeCredential(const CREDENTIALW& credential)
    {
        if (credential.CredentialBlob == nullptr || credential.CredentialBlobSize == 0)
        {
            return std::nullopt;
        }

        const auto* bytes = reinterpret_cast<const unsigned char*>(credential.CredentialBlob);
        const std::size_t size = credential.CredentialBlobSize;
        std::string result;

        const bool looksUtf16 = size >= 2 && size % 2 == 0 &&
            bytes[1] == 0 && (size < 4 || bytes[3] == 0);
        if (looksUtf16)
        {
            for (std::size_t index = 0; index + 1 < size; index += 2)
            {
                if (bytes[index] == 0 && bytes[index + 1] == 0)
                {
                    break;
                }
                if (bytes[index + 1] != 0)
                {
                    return std::nullopt;
                }
                result.push_back(static_cast<char>(bytes[index]));
            }
        }
        else
        {
            result.assign(reinterpret_cast<const char*>(bytes), size);
            while (!result.empty() && result.back() == '\0')
            {
                result.pop_back();
            }
        }

        if (result.empty())
        {
            return std::nullopt;
        }
        return result;
    }
}

namespace ChatBot::ConsoleApp
{
    std::optional<std::string> WindowsCredentialStore::ReadAzureOpenAIApiKey()
    {
        PCREDENTIALW credential = nullptr;
        if (!CredReadW(CredentialTarget, CRED_TYPE_GENERIC, 0, &credential))
        {
            return std::nullopt;
        }

        const auto result = DecodeCredential(*credential);
        CredFree(credential);
        return result;
    }
}
