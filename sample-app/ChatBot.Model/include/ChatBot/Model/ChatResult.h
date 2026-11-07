#pragma once

#include "ChatError.h"
#include "ChatResponse.h"

#include <variant>

namespace ChatBot::Model
{
    using ChatResult = std::variant<ChatResponse, ChatError>;
}
