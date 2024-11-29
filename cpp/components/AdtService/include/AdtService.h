#ifndef ADT_SERVICE_H
#define ADT_SERVICE_H

#include <string>
#include <BleService.h>
#include <BleAdvertiser.h>
#include <vector>
#include <functional>

extern "C" {
    #include <esp_log.h>
}

/**
 * @brief Enum for the different types of ADT service events
 * 
 */
enum AdtServiceEventType {
    TRANSMISSION_START = 0,
    TRANSMISSION_DATA = 1,
    TRANSMISSION_CANCEL = 2,
};

/**
 * @brief Helper class to contain message information
 * 
 */
class MessageInformation {
public:
    uint8_t totalChunks;
    std::vector<std::byte> data;
};

class AdtService
{
public:
    /**
     * @brief Construct a new ADT (Arbitrary Data Transfer) Service object
     * 
     * @param serviceUuid The UUID of the service
     * @param mtuCharacteristicUuid The UUID of the MTU characteristic
     * @param transmissionCharacteristicUuid The UUID of the transmission characteristic
     * @param receiveCharacteristicUuid The UUID of the receive characteristic
     * @param onMessageReceived The callback function for when a message is received
     */
    AdtService(
        std::string serviceUuid,
        std::string mtuCharacteristicUuid,
        std::string transmissionCharacteristicUuid,
        std::string receiveCharacteristicUuid,
        std::function<void(std::vector<std::byte>)> onMessageReceived
    );

    /**
     * @brief Destroy the Adt (Arbitrary Data Transfer) Service object
     * 
     */
    ~AdtService();

    /**
     * @brief Get the BLE service describing the ADT service.
     * 
     * @return BleService* A pointer to the BLE service
     */
    BleService *getBleService(void);
private:
    BleService *bleService;
    std::map<uint16_t, MessageInformation> messageInfos;
    std::function<void(std::vector<std::byte>)> onMessageReceived;
};

#endif // ADT_SERVICE_H