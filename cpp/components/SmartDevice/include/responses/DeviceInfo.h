#pragma once

#include "ISerializable.h"

#include <string>

namespace SmartDevice
{
    class DeviceInfo : public ISerializable
    {
    public:
        std::string type;
        std::string name;

        DeviceInfo(std::string type, std::string name) : type(type), name(name) {}

        std::unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
        {
            // Create the root cJSON object
            std::unique_ptr<cJSON, void (*)(cJSON *item)> root(cJSON_CreateObject(), cJSON_Delete);

            // Add the type
            cJSON_AddStringToObject(root.get(), "type", type.c_str());

            // Add the name
            cJSON_AddStringToObject(root.get(), "name", name.c_str());

            return root;
        }
    };
};