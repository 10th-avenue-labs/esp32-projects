#pragma once

#include "IDeserializable.h"
#include "Result.h"

#include "esp_log.h"
#include <string>
#include <unordered_map>

namespace SmartDevice
{
    class Request : public IDeserializable
    {
    public:
        std::string type;
        std::unique_ptr<IDeserializable> data;

        /**
         * @brief Construct a new Request object
         *
         * @param type The type of the request
         * @param data The deserializable data for the request
         */
        Request(std::string type, std::unique_ptr<IDeserializable> data) : type(type), data(std::move(data)) {}

        /**
         * @brief Register a request type
         *
         * @param typeName The name of the request type
         * @param typeIndex The type index of the data type
         */
        static void registerRequestType(std::string typeName, std::type_index typeIndex)
        {
            dataTypesByTypeName.emplace(typeName, typeIndex);
        }

        /**
         * @brief Deserialize a request
         *
         * @param root The cJSON root object
         * @return Result<std::unique_ptr<IDeserializable>> A result containing the deserialized request
         */
        static Result<std::unique_ptr<IDeserializable>> deserialize(const cJSON *root)
        {
            // Get the fields
            cJSON *typeItem = cJSON_GetObjectItem(root, "type");
            cJSON *dataItem = cJSON_GetObjectItem(root, "data");

            // Verify the fields
            if (!cJSON_IsString(typeItem))
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing 'type' field");
            }

            // Get the type
            std::string type = typeItem->valuestring;

            // Get the deserializer for the data type
            auto it = dataTypesByTypeName.find(type);
            if (it == dataTypesByTypeName.end())
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Failed to get data type for request type '" + type + "'");
            }
            auto dataDeserializer = it->second == typeid(void) ? voidReturner : IDeserializable::getDeserializer(it->second);
            if (!dataDeserializer)
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Failed to get data deserializer");
            }

            // Deserialize the data
            auto deserializedDataResult = dataDeserializer(dataItem);
            if (!deserializedDataResult.isSuccess())
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Failed to deserialize data: " + deserializedDataResult.getError());
            }

            // Transfer ownership from the result
            auto dataValue = deserializedDataResult.getValue();

            // Return the deserialized request
            return Result<std::unique_ptr<IDeserializable>>::createSuccess(std::make_unique<Request>(type, std::move(dataValue)));
        }

    private:
        static std::unordered_map<std::string, std::type_index> dataTypesByTypeName;
        static std::function<Result<std::unique_ptr<IDeserializable>>(const cJSON *root)> voidReturner;
    };
};