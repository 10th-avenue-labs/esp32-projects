#ifndef ISERIALIZABLE_H
#define ISERIALIZABLE_H

#include <string>
#include <memory>
#include "cJSON.h"

using namespace std;

class ISerializable
{
public:
    /**
     * @brief Serialize the object to a cJSON object
     *
     * @return unique_ptr<cJSON, void (*)(cJSON *item)> The serialized cJSON object
     */
    virtual unique_ptr<cJSON, void (*)(cJSON *item)> serialize() = 0;

    /**
     * @brief Serialize the object to a string
     *
     * @param pretty Whether to pretty-print the JSON
     * @return string The serialized JSON string
     */
    string serializeToString(bool pretty = false)
    {
        unique_ptr<cJSON, void (*)(cJSON *item)> json(serialize().release(), cJSON_Delete);
        char *jsonStr = pretty ? cJSON_Print(json.get()) : cJSON_PrintUnformatted(json.get());
        string result(jsonStr);
        free(jsonStr);
        return result;
    }
};

#endif // ISERIALIZABLE_H