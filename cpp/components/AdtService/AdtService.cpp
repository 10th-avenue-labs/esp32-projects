#include "AdtService.h"

static const char* TAG = "ADT_SERVICE";

const uint8_t MTU_RESERVED_BYTES = 3;

AdtService::AdtService(
    std::string serviceUuid,
    std::string mtuCharacteristicUuid,
    std::string transmissionCharacteristicUuid,
    std::string receiveCharacteristicUuid,
    std::function<void(uint16_t messageId, std::vector<std::byte>, shared_ptr<BleDevice> device)> onMessageReceived
):
    onMessageReceived(onMessageReceived)
{
    // Create the receive characteristic
    this->receiveCharacteristic = make_shared<BleCharacteristic>(
        receiveCharacteristicUuid,
        nullptr,    // No write callback
        nullptr,    // No read callback
        nullptr,    // No subscribe callback
        false       // No write acknowledgment
    );

    // Create the BLE service
    this->bleService = make_shared<BleService>(BleService(serviceUuid, vector<shared_ptr<BleCharacteristic>> {
        make_shared<BleCharacteristic>(
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
            nullptr,    // No subscribe callback
            false  // No write acknowledgment
        ),
        make_shared<BleCharacteristic>(
            transmissionCharacteristicUuid,
            [this](std::vector<std::byte> data, shared_ptr<BleDevice> device) -> int {
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
                ESP_LOGI(TAG, "event type: %d, chunk arg: %d, message ID: %d", eventType, chunkByte, messageId);

                switch (eventType) {
                    case TRANSMISSION_START: {
                        // Create a vector to store the data
                        std::vector<std::byte> messageData;
                        messageData.reserve(data.size() - 4);

                        // Move the data to the message
                        std::move(data.begin() + 4, data.end(), std::back_inserter(messageData));

                        // Check if this is the only chunk in the message
                        if (chunkByte == 1) {
                            // Call the message received delegate
                            if (this->onMessageReceived != nullptr) {
                                this->onMessageReceived(messageId, messageData, device);
                            }
                            return 0;
                        }

                        // Store the data until all chunks are received
                        this->messageInfos[messageId] = {chunkByte, messageData};

                        break;
                    }
                    case TRANSMISSION_DATA: {
                        // Add the data to the message
                        this->messageInfos[messageId].data.insert(this->messageInfos[messageId].data.end(), data.begin() + 4, data.end());

                        // TODO: We could add logic somewhere in here to check for dropped or skipped chunks

                        // Check if there is more data to receive
                        if (chunkByte < this->messageInfos[messageId].totalChunks - 1) {
                            return 0;
                        }

                        // Call the message received delegate
                        if (this->onMessageReceived != nullptr) {
                            this->onMessageReceived(messageId, this->messageInfos[messageId].data, device);
                        }

                        // Remove the message from the map
                        this->messageInfos.erase(messageId);

                        break;
                    }
                    case TRANSMISSION_CANCEL: {
                        // Log the cancel event
                        ESP_LOGI(TAG, "message ID %d cancelled", messageId);

                        // Clear the message
                        this->messageInfos.erase(messageId);
                        break;
                    }
                    default: {
                        // Log the unknown event type
                        ESP_LOGE(TAG, "Unknown event type: %d", eventType);

                        // Clear the message
                        this->messageInfos.erase(messageId);
                        return -1;
                    }
                }

                // Return success
                return 0;
            },
            nullptr,    // No read callback
            nullptr,    // No subscribe callback
            false       // No write acknowledgment
        ),
        this->receiveCharacteristic
    }));
}

esp_err_t AdtService::sendMessage(vector<shared_ptr<BleDevice>> devices, vector<byte> data) {
    /*
    Chunk structure:
        bytes: 1,1: Event type
        bytes: 2,2: Total chunks (for event type START) or chunk number (for event type DATA)
        bytes: 3,4: Message ID
        bytes: 5,n: Message
    */

    // Get the MTU
    uint16_t mtu = BleAdvertiser::getMtu();

    // Calculate the header size
    uint8_t transmissionHeaderSize = 4;

    // Calculate the maximum data size
    uint8_t dataSize = mtu - transmissionHeaderSize - MTU_RESERVED_BYTES; // 249
    ESP_LOGI(TAG, "data size: %d", dataSize);

    // Get the total bytes that must be transmitted
    size_t totalBytes = data.size();
    ESP_LOGI(TAG, "total bytes: %d", totalBytes);

    // Calculate the number of chunks
    int totalChunks = custom_ceil(((double) totalBytes) / dataSize);
    if (totalChunks > 255) {
        ESP_LOGE(TAG, "total chunks is greater than 255");

        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "total chunks: %d", totalChunks);

    // Create a message ID
    uint16_t messageId = (uint16_t)rand() ^ (((uint16_t)rand()) << 8);
    ESP_LOGI(TAG, "message ID: %d", messageId);

    // Create the start event header
    vector<byte> startEventHeader = vector<byte>(transmissionHeaderSize);
    startEventHeader[0] = (byte)0; // Event type
    startEventHeader[1] = (byte)totalChunks; // Total chunks
    startEventHeader[2] = (byte)(messageId & 0xFF);
    startEventHeader[3] = (byte)((messageId >> 8) & 0xFF);

    // Get the first chunk of data
    vector<byte> chunk = getNextChunk(data, dataSize);

    // Create the start event message
    vector<byte> payload = vector<byte>(startEventHeader.size() + chunk.size());
    move(startEventHeader.begin(), startEventHeader.end(), payload.begin());
    move(chunk.begin(), chunk.end(), payload.begin() + startEventHeader.size());

    // Send the start event
    ESP_ERROR_CHECK(this->receiveCharacteristic->notify(devices, payload));

    // Loop through the data and send the chunks
    int chunkNumber = 1;
    while(data.size() > 0) {
        // Create the data event header
        vector<byte> dataEventHeader = vector<byte>(transmissionHeaderSize);
        dataEventHeader[0] = (byte)1; // Event type
        dataEventHeader[1] = (byte)chunkNumber; // Chunk number
        dataEventHeader[2] = (byte)(messageId & 0xFF);
        dataEventHeader[3] = (byte)((messageId >> 8) & 0xFF);

        // Get the first chunk of data
        vector<byte> chunk = getNextChunk(data, dataSize);

        // Create the data event message
        vector<byte> dataEventPayload = vector<byte>(dataEventHeader.size() + chunk.size());
        move(dataEventHeader.begin(), dataEventHeader.end(), dataEventPayload.begin());
        move(chunk.begin(), chunk.end(), dataEventPayload.begin() + dataEventHeader.size());

        // Send the data event
        ESP_ERROR_CHECK(this->receiveCharacteristic->notify(devices, dataEventPayload));

        // Increment the chunk number
        chunkNumber++;
    }

    return ESP_OK;
}

shared_ptr<BleService> AdtService::getBleService(void) {
    return this->bleService;
}

vector<byte> AdtService::getNextChunk(vector<byte>& data, int chunkSize) {
    // If chunk size is greater than or equal to the size of the vector, return the whole vector and clear it
    if (chunkSize >= data.size()) {
        vector<byte> result = move(data);
        data.clear();
        return result;
    }

    // Create a new vector with the first chunk of bytes
    std::vector<byte> result(data.begin(), data.begin() + chunkSize);

    // Remove the first n bytes from the original vector
    data.erase(data.begin(), data.begin() + chunkSize);

    return result;
}

double AdtService::custom_ceil(double num) {
    // TODO: This function is only included due to conflicts between the imported math libraries for c and cpp. Ideally, we would fix the root issue and remove this function.
    int int_part = (int)num;  // Get the integer part of the number
    if (num > int_part) {
        return int_part + 1;  // Add 1 if there's a fractional part
    }
    return int_part;          // Otherwise, return the integer part
}
