#pragma once

#include "IDeserializable.h"
#include "ISerializable.h"

class AcDimmerConfig : public IDeserializable, public ISerializable
{
public:
    int zcPin;
    int psmPin;
    int debounceUs;
    int offsetLeading;
    int offsetFalling;
    int brightness;

    /**
     * @brief Construct a new Ac Dimmer Config object
     *
     * @param zcPin The zero crossing pin
     * @param psmPin The phase shift modulation pin
     * @param debounceUs The debounce time in microseconds
     * @param offsetLeading The offset for the leading edge
     * @param offsetFalling The offset for the falling edge
     * @param brightness The brightness (duty cycle) of the dimmer
     */
    AcDimmerConfig(int zcPin, int psmPin, int debounceUs, int offsetLeading, int offsetFalling, int brightness)
        : zcPin(zcPin), psmPin(psmPin), debounceUs(debounceUs), offsetLeading(offsetLeading), offsetFalling(offsetFalling), brightness(brightness) {};

    /**
     * @brief Serialize the object to a cJSON object
     *
     * @return std::unique_ptr<cJSON, void (*)(cJSON *item)> The serialized cJSON object
     */
    std::unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
    {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "zcPin", zcPin);
        cJSON_AddNumberToObject(root, "psmPin", psmPin);
        cJSON_AddNumberToObject(root, "debounceUs", debounceUs);
        cJSON_AddNumberToObject(root, "offsetLeading", offsetLeading);
        cJSON_AddNumberToObject(root, "offsetFalling", offsetFalling);
        cJSON_AddNumberToObject(root, "brightness", brightness);
        return std::unique_ptr<cJSON, void (*)(cJSON *item)>(root, cJSON_Delete);
    }

    /**
     * @brief Deserialize the object from a cJSON object
     *
     * @param root The cJSON object
     * @return Result<std::unique_ptr<IDeserializable>> The deserialized object
     */
    static Result<std::unique_ptr<IDeserializable>> deserialize(const cJSON *root)
    {
        cJSON *zcPinItem = cJSON_GetObjectItem(root, "zcPin");
        if (!cJSON_IsNumber(zcPinItem))
        {
            return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing zcPin field in JSON");
        }
        int zcPin = zcPinItem->valueint;

        cJSON *psmPinItem = cJSON_GetObjectItem(root, "psmPin");
        if (!cJSON_IsNumber(psmPinItem))
        {
            return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing psmPin field in JSON");
        }
        int psmPin = psmPinItem->valueint;

        cJSON *debounceUsItem = cJSON_GetObjectItem(root, "debounceUs");
        if (!cJSON_IsNumber(debounceUsItem))
        {
            return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing debounceUs field in JSON");
        }
        int debounceUs = debounceUsItem->valueint;

        cJSON *offsetLeadingItem = cJSON_GetObjectItem(root, "offsetLeading");
        if (!cJSON_IsNumber(offsetLeadingItem))
        {
            return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing offsetLeading field in JSON");
        }
        int offsetLeading = offsetLeadingItem->valueint;

        cJSON *offsetFallingItem = cJSON_GetObjectItem(root, "offsetFalling");
        if (!cJSON_IsNumber(offsetFallingItem))
        {
            return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing offsetFalling field in JSON");
        }
        int offsetFalling = offsetFallingItem->valueint;

        cJSON *brightnessItem = cJSON_GetObjectItem(root, "brightness");
        if (!cJSON_IsNumber(brightnessItem))
        {
            return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing brightness field in JSON");
        }
        int brightness = brightnessItem->valueint;

        return Result<std::unique_ptr<IDeserializable>>::createSuccess(std::make_unique<AcDimmerConfig>(zcPin, psmPin, debounceUs, offsetLeading, offsetFalling, brightness));
    };
};