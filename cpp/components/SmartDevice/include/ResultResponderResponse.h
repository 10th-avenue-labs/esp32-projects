#pragma once

#include "Result.h"
#include "ISerializable.h"

namespace SmartDevice
{
    class ResultResponderResponse : public ISerializable
    {
    public:
        uint16_t messageId;
        Result<std::shared_ptr<ISerializable>> result;

        /**
         * @brief Construct a new Result Responder Response object. These are responses to requests that expect a result in response
         *
         * @param messageId The message ID of the request
         * @param result The result of the request
         */
        ResultResponderResponse(uint16_t messageId, Result<std::shared_ptr<ISerializable>> result)
            : messageId(messageId), result(result) {}

        /**
         * @brief Serialize the object to a cJSON object
         *
         * @return std::unique_ptr<cJSON, void (*)(cJSON *item)> The serialized cJSON object
         */
        std::unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
        {
            // Create a new cJSON object
            std::unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_CreateObject(), cJSON_Delete);

            // Serialize the object
            cJSON_AddItemToObject(root.get(), "messageId", cJSON_CreateNumber(messageId));
            cJSON_AddItemToObject(root.get(), "result", result.serialize().release());

            return root;
        };
    };
}