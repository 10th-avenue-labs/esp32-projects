using HashtagChris.DotNetBlueZ;
using HashtagChris.DotNetBlueZ.Extensions;

class MessageInformation {
    public UInt16 TotalChunks { get; set; }
    public List<byte> Data { get; set; } = [];
}

class AdtService(
    string serviceUuid,
    string mtuCharacteristicUuid,
    string transmissionCharacteristicUuid,
    string receiveCharacteristicUuid,
    Func<List<byte>, Task> onMessageReceived
) {
    private static readonly int MTU_RESERVED_BYTES = 3;
    private static readonly int TRANSMISSION_HEADER_SIZE = 4;

    public string ServiceUuid { get; } = serviceUuid;

    string MtuCharacteristicUuid { get; } = mtuCharacteristicUuid;
    string TransmissionCharacteristicUuid { get; } = transmissionCharacteristicUuid;
    string ReceiveCharacteristicUuid { get; } = receiveCharacteristicUuid;
    Func<List<byte>, Task> OnMessageReceived { get; } = onMessageReceived;
    UInt16 Mtu { get; set; }
    GattCharacteristic? transmitCharacteristic = null;
    Dictionary<UInt16, MessageInformation> messageInfos = [];

    public async Task<bool> Init(Device? device)
    {
        // Check if the device is null
        if (device == null) {
            Console.WriteLine("Device not found.");
            return false;
        }

        // Get the ADT service
        Console.WriteLine($"Searching for ADT service.");
        var adtService = await device.GetServiceAsync(ServiceUuid);
        if (adtService == null) {
            var services = await device.GetServicesAsync();
            Console.WriteLine($"Could not find ADT service with ID '{ServiceUuid}'. Instead found the following:");
            foreach(var found in services) {
                await PrintServiceInfo(found);
            }
            return false;
        }
        await PrintServiceInfo(adtService);

        // Get the MTU characteristic
        Console.WriteLine($"Searching for MTU characteristic.");
        var mtuCharacteristic = await adtService.GetCharacteristicAsync(MtuCharacteristicUuid);
        if (mtuCharacteristic == null) {
            var characteristics = await adtService.GetCharacteristicsAsync();
            Console.WriteLine($"Could not find MTU characteristic with ID '{MtuCharacteristicUuid}'. Instead found the following:");
            foreach(var found in characteristics) {
                await PrintCharacteristicInfo(found);
            }
            return false;
        }
        await PrintCharacteristicInfo(mtuCharacteristic);

        // Get the Transmit characteristic
        Console.WriteLine($"Searching for Transmit characteristic.");
        transmitCharacteristic = await adtService.GetCharacteristicAsync(TransmissionCharacteristicUuid);
        if (transmitCharacteristic == null) {
            var characteristics = await adtService.GetCharacteristicsAsync();
            Console.WriteLine($"Could not find Transmit characteristic with ID '{TransmissionCharacteristicUuid}'. Instead found the following:");
            foreach(var found in characteristics) {
                await PrintCharacteristicInfo(found);
            }
            return false;
        }
        await PrintCharacteristicInfo(transmitCharacteristic);

        // Get the Receive characteristic
        Console.WriteLine($"Searching for Receive characteristic.");
        var receiveCharacteristic = await adtService.GetCharacteristicAsync(ReceiveCharacteristicUuid);
        if (receiveCharacteristic == null) {
            var characteristics = await adtService.GetCharacteristicsAsync();
            Console.WriteLine($"Could not find Receive characteristic with ID '{ReceiveCharacteristicUuid}'. Instead found the following:");
            foreach(var found in characteristics) {
                await PrintCharacteristicInfo(found);
            }
            return false;
        }
        await PrintCharacteristicInfo(receiveCharacteristic);

        // Read the MTU characteristic
        var mtuBytes = await mtuCharacteristic.ReadValueAsync(new Dictionary<string, object>());
        try {
            Mtu = BitConverter.ToUInt16(mtuBytes, 0);
        } catch (Exception e) {
            Console.WriteLine($"Failed to read MTU characteristic: {e.Message}");
            return false;
        }
        Console.WriteLine($"MTU: {Mtu}");

        // Subscribe to the Receive characteristic
        await receiveCharacteristic.StartNotifyAsync();
        receiveCharacteristic.Value += ChunkReceivedHandler;

        return true;
    }

    public async Task<bool> Transmit(List<byte> data) {
        /*
        Chunk structure:
            bytes: 1,1: Event type
            bytes: 2,2: Total chunks (for event type START) or chunk number (for event type DATA)
            bytes: 3,4: Message ID
            bytes: 5,n: Message
        */
        // Check if the transmit characteristic is null
        if (transmitCharacteristic == null) {
            Console.WriteLine("Transmit characteristic is null.");
            return false;
        }

        // Check if the data is empty
        if (data.Count == 0) {
            Console.WriteLine("Data is empty.");
            return false;
        }

        // Check if the Mtu is set
        if (Mtu == 0) {
            Console.WriteLine("MTU is not set.");
            return false;
        }

        // Calculate the chunk size
        var chunkSize = Mtu - MTU_RESERVED_BYTES - TRANSMISSION_HEADER_SIZE; // 249 usually
        Console.WriteLine($"Chunk size: {chunkSize}");

        // Get the total bytes that must be transmitted
        var totalBytes = data.Count();
        Console.WriteLine($"Total bytes: {totalBytes}");

        // Calculate the number of chunks
        int totalChunks = (int)Math.Ceiling(((double) totalBytes) / chunkSize);
        if (totalChunks > 255) {
            Console.WriteLine($"Total chunks is greater than 255: {totalChunks}");
            return false;
        }
        Console.WriteLine($"Total chunks: {totalChunks}");

        // Create a message ID
        Random random = new();
        UInt16 randomValue = (UInt16)random.Next(0, ushort.MaxValue + 1);
        Console.WriteLine($"Message ID: {randomValue}");
        // TODO: Could probably return this value to the caller

        // Create the start event header
        var startEventHeader = new byte[TRANSMISSION_HEADER_SIZE];
        startEventHeader[0] = 0; // Event type
        startEventHeader[1] = (byte)totalChunks; // Total chunks
        byte[] messageBytes = BitConverter.GetBytes(randomValue);
        Console.WriteLine($"Message ID bytes size: {messageBytes.Length}");
        messageBytes.CopyTo(startEventHeader, 2); // Message ID

        // Create the start event message
        byte[] payload = [..startEventHeader, ..GetNextChunk(data, chunkSize)];

        // Write the start event message
        await transmitCharacteristic!.WriteValueAsync(payload, new Dictionary<string, object>());

        // Loop through the data and send the chunks
        int chunkNumber = 1;
        while(data.Count > 0) {
            // Create the data event header
            var dataEventHeader = new byte[TRANSMISSION_HEADER_SIZE];
            dataEventHeader[0] = 1; // Event type
            dataEventHeader[1] = (byte)chunkNumber; // Chunk number
            messageBytes.CopyTo(dataEventHeader, 2); // Message ID

            // Create the data event message
            payload = [..dataEventHeader, ..GetNextChunk(data, chunkSize)];

            // Write the data event message
            await transmitCharacteristic.WriteValueAsync(payload, new Dictionary<string, object>());

            // Increment the chunk number
            chunkNumber++;
        }

        return true;
    }

    private static byte[] GetNextChunk(List<byte> data, int chunkSize) {
        // Check if the data is smaller than the chunk size
        if (data.Count <= chunkSize) {
            // Get the last data
            byte[] lastData = [.. data];

            // Clear the data
            data.Clear();

            return lastData;
        }

        // Extract the first `n` elements
        List<byte> removedSubset = data.GetRange(0, chunkSize);

        // Remove the first `n` elements from the original list
        data.RemoveRange(0, chunkSize);

        return [.. removedSubset];
    }

    private async Task ChunkReceivedHandler(GattCharacteristic sender, GattCharacteristicValueEventArgs eventArgs) {
        // Get the chunk
        var chunk = eventArgs.Value;

        // Get the event type as an integer
        var eventType = (int) chunk[0];

        // Get the chunkByte. This is the total number of chunks for start events and the chunk number for data events
        var chunkByte = chunk[1];

        // Get the message ID
        var messageId = BitConverter.ToUInt16(chunk[2..4]);

        // Log the header information
        Console.WriteLine($"Event type: {eventType}, Chunk byte: {chunkByte}, Message ID: {messageId}");

        switch(eventType) {
            case 0: {
                // Create a list to store the data
                var data = new List<byte>();

                // Move the data to the message
                data.AddRange(chunk[4..]);

                // Check if this is the only chunk in the message
                if (chunkByte == 1) {
                    // Call the on message received delegate
                    await OnMessageReceived(data);
                    break;
                }

                // Store the data until all chunks are received
                messageInfos[messageId] = new MessageInformation {
                    TotalChunks = chunkByte,
                    Data = data
                };

                break;
            }
            case 1: {
                // Add the data to the message
                messageInfos[messageId].Data.AddRange(chunk[4..]);

                // Check if there is more data to receive
                if (chunkByte < messageInfos[messageId].TotalChunks - 1) {
                    return;
                }

                // Call the on message received delegate
                await OnMessageReceived(messageInfos[messageId].Data);

                // Remove the message from the dictionary
                messageInfos.Remove(messageId);

                break;
            }
            default:
                throw new Exception($"Unknown event type: {eventType}");
        }
    }

    static async Task PrintServiceInfo(IGattService1 service)
    {
        // Retrieve all details about the service
        var serviceInfo = await service.GetAllAsync();

        // Print Service details
        Console.WriteLine($"  Service: {serviceInfo.UUID}");
        Console.WriteLine($"    Includes: {string.Join(", ", serviceInfo.Includes.Select(include => include.ToString()))}");
        Console.WriteLine($"    Primary: {serviceInfo.Primary}");
    }

    static async Task PrintCharacteristicInfo(IGattCharacteristic1 characteristic)
    {
        // Retrieve all details about the characteristic
        var characteristicInfo = await characteristic.GetAllAsync();

        // Print Characteristic details
        Console.WriteLine($"    Characteristic: {characteristicInfo.UUID}");
        Console.WriteLine($"      Notifying: {characteristicInfo.Notifying}");
        Console.WriteLine($"      WriteAcquired: {characteristicInfo.WriteAcquired}");
        Console.WriteLine($"      NotifyAcquired: {characteristicInfo.NotifyAcquired}");
        Console.WriteLine($"      Flags: {string.Join(", ", characteristicInfo.Flags)}");
        Console.WriteLine($"      Value: {BitConverter.ToString(characteristicInfo.Value)}");
    }
}