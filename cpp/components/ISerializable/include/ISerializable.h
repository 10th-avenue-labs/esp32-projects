#pragma once

#include <string>
#include <memory>
#include "cJSON.h"

class ISerializable
{
public:
    /**
     * @brief Serialize the object to a cJSON object
     *
     * @return std::unique_ptr<cJSON, void (*)(cJSON *item)> The serialized cJSON object
     */
    virtual std::unique_ptr<cJSON, void (*)(cJSON *item)> serialize() = 0;

    /**
     * @brief Serialize the object to a string
     *
     * @param pretty Whether to pretty-print the JSON
     * @return string The serialized JSON string
     */
    std::string serializeToString(bool pretty = false)
    {
        std::unique_ptr<cJSON, void (*)(cJSON *item)> json(serialize().release(), cJSON_Delete);
        char *jsonStr = pretty ? cJSON_Print(json.get()) : cJSON_PrintUnformatted(json.get());
        std::string result(jsonStr);
        free(jsonStr);
        return result;
    }
};