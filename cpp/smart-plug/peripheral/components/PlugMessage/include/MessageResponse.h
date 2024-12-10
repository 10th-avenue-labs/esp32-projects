#ifndef MESSAGE_RESPONSE_H
#define MESSAGE_RESPONSE_H

#include <Result.h>
#include <IPlugMessageData.h>

template <typename T>
class MessageResponse: public IPlugMessageData {
    public:
        uint16_t messageId;
        Result<T> result;

        MessageResponse(uint16_t messageId, Result<T> result)
            : messageId(messageId), result(result) {}

        string serialize() {
            cJSON* root = cJSON_CreateObject();
            cJSON_AddNumberToObject(root, "messageId", messageId);
            cJSON_AddItemToObject(root, "result", cJSON_Parse(result.serialize().c_str()));
            return cJSON_PrintUnformatted(root);
        }
};

#endif // MESSAGE_RESPONSE_H