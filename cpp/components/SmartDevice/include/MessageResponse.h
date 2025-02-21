#pragma once

#include "ISerializable.h"
#include "Result.h"

namespace SmartDevice
{
    template <typename T>
    class MessageResponse : public ISerializable
    {
    public:
        uint16_t messageId;
        Result<T> result;

        MessageResponse(uint16_t messageId, Result<T> result)
            : messageId(messageId), result(result) {}

        std::unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
        {
            // Create the root cJSON object
            std::unique_ptr<cJSON, void (*)(cJSON *item)> root(cJSON_CreateObject(), cJSON_Delete);

            // Add the message ID
            cJSON_AddNumberToObject(root.get(), "messageId", messageId);

            // Add the result
            cJSON_AddItemToObject(root.get(), "result", result.serialize().release());

            return root;
        }
    };
}