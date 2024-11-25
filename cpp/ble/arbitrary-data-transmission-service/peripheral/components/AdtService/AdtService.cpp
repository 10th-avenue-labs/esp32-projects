#include "AdtService.h"

const char* TAG = "ADT_SERVICE";

AdtService::AdtService(
    std::string serviceUuid,
    std::string mtuCharacteristicUuid,
    std::string transmissionCharacteristicUuid
) {
    // Create the BLE service
    this->bleService = new BleService(serviceUuid, {
        BleCharacteristic(
            mtuCharacteristicUuid,
            nullptr, // No write callback
            []() -> std::vector<std::byte> {
                // Log the read event
                ESP_LOGI(TAG, "mtu characteristic read");

                // Get the MTU
                uint16_t mtu = BleAdvertiser::getMtu();

                // Convert the MTU to a vector of bytes
                std::vector<std::byte> mtuBytes;
                for (int i = 0; i < sizeof(mtu); i++) {
                    mtuBytes.push_back(static_cast<std::byte>((mtu >> (i * 8)) & 0xFF));
                }

                return mtuBytes;
            },
            true,  // Can read
            false, // Cannot write
            false  // No write acknowledgment
        ),
        BleCharacteristic(
            transmissionCharacteristicUuid,
            [this](std::vector<std::byte> data) -> int {
                // Log the write event
                ESP_LOGI(TAG, "transmission characteristic write");

                // Get the event type
                AdtServiceEventType eventType = (AdtServiceEventType) std::to_integer<int>(data[0]);

                // Get the chunkByte. This is the total number of chunks for start events and the chunk number for data events
                uint8_t chunkByte = std::to_integer<uint8_t>(data[1]);

                // Get the message ID
                uint16_t messageId = (static_cast<uint16_t>(std::to_integer<uint8_t>(data[3])) << 8) |
                                    static_cast<uint16_t>(std::to_integer<uint8_t>(data[2]));

                // Log the header information
                ESP_LOGI(TAG, "event type: %d, chunk number: %d, message ID: %d", eventType, chunkByte, messageId);

                switch (eventType) {
                    case TRANSMISSION_START: {
                        // Create a vector to store the data
                        std::vector<std::byte> messageData;
                        messageData.reserve(data.size() - 4);

                        // Move the data to the message
                        std::move(data.begin() + 4, data.end(), std::back_inserter(messageData));

                        // Check if this is the only chunk in the message
                        if (chunkByte > 1) {
                            // Call the message received delegate
                            this->onMessageReceived(messageData);
                            return;
                        }

                        // Store the data until all chunks are received
                        this->messageInfos[messageId] = {chunkByte, messageData};

                        break;
                    }
                    case TRANSMISSION_DATA: {
                        // Add the data to the message
                        this->messageInfos[messageId].data.insert(this->messageInfos[messageId].data.end(), data.begin(), data.end());

                        // Check if there is more data to receive
                        if (chunkByte < this->messageInfos[messageId].totalChunks) {
                            return;
                        }

                        // Call the message received delegate
                        this->onMessageReceived(this->messageInfos[messageId].data);

                        // Remove the message from the map
                        this->messageInfos.erase(messageId);

                        break;
                    }
                    default: {
                        // Log the unknown event type
                        ESP_LOGE(TAG, "Unknown event type: %d", eventType);
                        return -1;
                    }
                }

                // Return success
                return 0;
            },
            nullptr,    // No read callback
            false,      // Cannot read
            true,       // Can write
            false       // No write acknowledgment
        )
    });
}

AdtService::~AdtService() {
    // Delete the BLE service
    delete this->bleService;
}

BleService *AdtService::getBleService(void) {
    return this->bleService;
}