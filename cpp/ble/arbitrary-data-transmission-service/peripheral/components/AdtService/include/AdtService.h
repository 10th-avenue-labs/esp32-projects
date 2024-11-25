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

enum AdtServiceEventType {
    TRANSMISSION_START = 0,
    TRANSMISSION_DATA = 1,
    TRANSMISSION_CANCEL = 2,
};

class MessageInformation {
public:
    uint8_t totalChunks;
    std::vector<std::byte> data;
};

class AdtService
{
public:
    std::function<void(std::vector<std::byte>)> onMessageReceived;

    AdtService(
        std::string serviceUuid,
        std::string mtuCharacteristicUuid,
        std::string transmissionCharacteristicUuid
    );
    ~AdtService();

    BleService *getBleService(void);
private:
    BleService *bleService;
    BleCharacteristic *mtuCharacteristic;
    BleCharacteristic *transmissionCharacteristic;
    std::map<uint16_t, MessageInformation> messageInfos;
};

#endif // ADT_SERVICE_H