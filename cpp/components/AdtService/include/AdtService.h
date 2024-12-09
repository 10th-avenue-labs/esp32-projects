#ifndef ADT_SERVICE_H
#define ADT_SERVICE_H

#include <string>
#include <memory>
#include <BleService.h>
#include <BleAdvertiser.h>
#include <vector>
#include <functional>

#include <esp_log.h>

using namespace std;

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
    vector<byte> data;
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
     * @param onMessageReceived The callback function for when a message is received. Takes in a message id, the received data, and the device that sent it
     */
    AdtService(
        string serviceUuid,
        string mtuCharacteristicUuid,
        string transmissionCharacteristicUuid,
        string receiveCharacteristicUuid,
        function<void(uint16_t messageId, vector<byte>, shared_ptr<BleDevice> device)> onMessageReceived
    );

    /**
     * @brief Send a message to a list of devices
     * 
     * @param devices The devices to send the message to
     * @param data The data to send
     * @return esp_err_t ESP_OK if successful, ESP_FAIL otherwise
     */
    esp_err_t sendMessage(vector<shared_ptr<BleDevice>> devices, vector<byte> data);

    /**
     * @brief Get the BLE service describing the ADT service.
     * 
     * @return BleService* A pointer to the BLE service
     */
    shared_ptr<BleService> getBleService(void);
private:
    shared_ptr<BleService> bleService;
    map<uint16_t, MessageInformation> messageInfos;
    function<void(uint16_t messageId, vector<byte>, shared_ptr<BleDevice> device)> onMessageReceived;
    shared_ptr<BleCharacteristic> receiveCharacteristic;

    /**
     * @brief Get the next chunk of data
     * 
     * @param data The data to get the chunk from
     * @param chunkSize The size of the chunk
     * @return vector<byte> The next chunk of data
     */
    static vector<byte> getNextChunk(vector<byte>& data, int chunkSize);

    /**
     * @brief Custom ceil function
     * 
     * @param num The number to ceil
     * @return double The ceiled number
     */
    static double custom_ceil(double num);
};

#endif // ADT_SERVICE_H