﻿using System.Text;
using HashtagChris.DotNetBlueZ;
using HashtagChris.DotNetBlueZ.Extensions;

const string TARGET = "ADT Service";

// ADT service UUIDs
const string ADT_SERVICE_UUID =                                "cc3aab60-0001-0000-81e0-c88f19fb28cb";
const string ADT_SERVICE_MTU_CHARACTERISTIC_UUID =             "cc3aab60-0001-0001-81e0-c88f19fb28cb";
const string ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID =    "cc3aab60-0001-0002-81e0-c88f19fb28cb";
const string ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID =         "cc3aab60-0001-0003-81e0-c88f19fb28cb";

const int MTU_RESERVED_BYTES = 3;

///////////////////////////////////////////////////////////////////////////////
/// Get and print the bluetooth adapter and device information
///////////////////////////////////////////////////////////////////////////////

// Get the bluetooth adapters
IReadOnlyList<Adapter> adapters = await BlueZManager.GetAdaptersAsync();

// Verify at least one adapter has been found
if (adapters.Count < 1) {
    throw new Exception("Could not find any bluetooth adapter.");
}

// Print the adapter info
Console.WriteLine($"Found {adapters.Count} adapters.");
foreach(var currentAdapter in adapters) {
    await PrintAdapterInfo(currentAdapter);
}

// Select the first adapter
var adapter = adapters[0];
Console.WriteLine($"Selecting first adapter '{await adapter.GetAliasAsync()}'.");

///////////////////////////////////////////////////////////////////////////////
/// Setup the device found handler and search for the target device
///////////////////////////////////////////////////////////////////////////////

// Setup the device found handler
Console.WriteLine($"Searching for device '{TARGET}'.");
CancellationTokenSource cancellationTokenSource = new ();
Device? device = null;
adapter.DeviceFound += async (Adapter sender, DeviceFoundEventArgs eventArgs) => {
    Device? currentDevice = await HandleDeviceFound(sender, eventArgs, TARGET, cancellationTokenSource);
    if( currentDevice != null) device = currentDevice;
};

// Start the discovery
var timeoutSeconds = 30;
Console.WriteLine($"Searching for {timeoutSeconds} seconds.");
await adapter.StartDiscoveryAsync();
try {
    await Task.Delay(1000 * timeoutSeconds, cancellationTokenSource.Token);
} catch (TaskCanceledException) {
    // Do nothing
}
await adapter.StopDiscoveryAsync();

// Check if we've found the device
if (device == null) {
    Console.WriteLine("Could not find device.");
    return 1;
}

// Print the device info
await PrintDeviceInfo(device);

// Connect to the device
Console.WriteLine("Connecting to device.");
TimeSpan timeout = TimeSpan.FromSeconds(15);
await device.ConnectAsync();
await device.WaitForPropertyValueAsync("Connected", value: true, timeout);
await device.WaitForPropertyValueAsync("ServicesResolved", value: true, timeout);
Console.WriteLine("Connected to device.");

///////////////////////////////////////////////////////////////////////////////
/// Get arbitrary data transfer service and characteristics
///////////////////////////////////////////////////////////////////////////////

// Get the Arbitrary Data Transfer Service
Console.WriteLine($"Searching for Arbitrary Data Transfer Service.");
var arbitraryDataTransferService = await device.GetServiceAsync(ADT_SERVICE_UUID);
if (arbitraryDataTransferService == null) {
    var services = await device.GetServicesAsync();
    Console.WriteLine($"Could not find dimmer service with ID '{ADT_SERVICE_UUID}'. Instead found the following:");
    foreach(var found in services) {
        await PrintServiceInfo(found);
    }
    return 0;
}

await PrintServiceInfo(arbitraryDataTransferService);

// Get the MTU characteristic
Console.WriteLine($"Searching for MTU characteristic.");
var mtuCharacteristic = await arbitraryDataTransferService.GetCharacteristicAsync(ADT_SERVICE_MTU_CHARACTERISTIC_UUID);
if (mtuCharacteristic == null) {
    var characteristics = await arbitraryDataTransferService.GetCharacteristicsAsync();
    Console.WriteLine($"Could not find MTU characteristic with ID '{ADT_SERVICE_MTU_CHARACTERISTIC_UUID}'. Instead found the following:");
    foreach(var found in characteristics) {
        await PrintCharacteristicInfo(found);
    }
    return 0;
}
await PrintCharacteristicInfo(mtuCharacteristic);

// Read the MTU characteristic
var mtuBytes = await mtuCharacteristic.ReadValueAsync(new Dictionary<string, object>());
var mtu = BitConverter.ToUInt16(mtuBytes, 0);
Console.WriteLine($"MTU: {mtu}");

// Get the transmission characteristic
Console.WriteLine($"Searching for Transmission characteristic.");
var transmissionCharacteristic = await arbitraryDataTransferService.GetCharacteristicAsync(ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID);
if (transmissionCharacteristic == null) {
    var characteristics = await arbitraryDataTransferService.GetCharacteristicsAsync();
    Console.WriteLine($"Could not find Transmission characteristic with ID '{ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID}'. Instead found the following:");
    foreach(var found in characteristics) {
        await PrintCharacteristicInfo(found);
    }
    return 0;
}
await PrintCharacteristicInfo(transmissionCharacteristic);

// Get the receive characteristic
Console.WriteLine($"Searching for Receive characteristic.");
var receiveCharacteristic = await arbitraryDataTransferService.GetCharacteristicAsync(ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID);
if (receiveCharacteristic == null) {
    var characteristics = await arbitraryDataTransferService.GetCharacteristicsAsync();
    Console.WriteLine($"Could not find Receive characteristic with ID '{ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID}'. Instead found the following:");
    foreach(var found in characteristics) {
        await PrintCharacteristicInfo(found);
    }
    return 0;
}
await PrintCharacteristicInfo(receiveCharacteristic);

///////////////////////////////////////////////////////////////////////////////
/// Subscribe to the receive characteristic
///////////////////////////////////////////////////////////////////////////////

// Reading the characteristic will also affect it's value and cause an event to be emitted (due to the peripheral changing the value)
// Using a unique characteristic specifically for notifications mitigates this
Dictionary<UInt16, MessageInformation> messageInfos = [];
receiveCharacteristic.Value += async (GattCharacteristic sender, GattCharacteristicValueEventArgs eventArgs) => {
    Console.WriteLine("Value changed");

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
                await MessageReceived(data);
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
            await MessageReceived(messageInfos[messageId].Data);

            // Remove the message from the dictionary
            messageInfos.Remove(messageId);

            break;
        }
        default:
            throw new Exception($"Unknown event type: {eventType}");
    }
};
await receiveCharacteristic.StartNotifyAsync();

///////////////////////////////////////////////////////////////////////////////
/// Attempt to transfer large data
///////////////////////////////////////////////////////////////////////////////

// Attempt to transfer large data
var partA = new string ('a', 249);
var partB = new string ('b', 249);
var partC = new string ('c', 249);
var message = $"{partA}{partB}{partC}";
await TransmitLargeData(transmissionCharacteristic, mtu, [.. Encoding.ASCII.GetBytes(message)]);

while(true) {
    await Task.Delay(1000);
}

async Task TransmitLargeData(GattCharacteristic transmissionCharacteristic, UInt16 mtu, List<byte> data) {
    /*
    Chunk structure:
        bytes: 1,1: Event type
        bytes: 2,2: Total chunks (for event type START) or chunk number (for event type DATA)
        bytes: 3,4: Message ID
        bytes: 5,n: Message
    */

    // Calculate the header size
    var TRANSMISSION_HEADER_SIZE = 4;

    // Calculate the chunk size
    var chunkSize = mtu - MTU_RESERVED_BYTES - TRANSMISSION_HEADER_SIZE; // 249
    Console.WriteLine($"Chunk size: {chunkSize}");

    // Get the total bytes that must be transmitted
    var totalBytes = data.Count;
    Console.WriteLine($"Total bytes: {totalBytes}");

    // Calculate the number of chunks
    int totalChunks = (int)Math.Ceiling(((double) totalBytes) / chunkSize);
    if (totalChunks > 255) {
        throw new Exception($"Total chunks is greater than 255: {totalChunks}");
    }
    Console.WriteLine($"Total chunks: {totalChunks}");

    // Create a message ID
    Random random = new();
    UInt16 randomValue = (UInt16)random.Next(0, ushort.MaxValue + 1);
    Console.WriteLine($"Message ID: {randomValue}");

    // Create the start event header
    var startEventHeader = new byte[TRANSMISSION_HEADER_SIZE];
    startEventHeader[0] = 0; // Event type
    startEventHeader[1] = (byte)totalChunks; // Total chunks
    byte[] messageBytes = BitConverter.GetBytes(randomValue);
    Console.WriteLine($"Message ID bytes size: {messageBytes.Length}");
    messageBytes.CopyTo(startEventHeader, 2); // Message ID

    // Create the start event message
    byte[] payload = [..startEventHeader, ..getNextChunk(data, chunkSize)];

    // Write the start event message
    await transmissionCharacteristic.WriteValueAsync(payload, new Dictionary<string, object>());

    // Loop through the data and send the chunks
    int chunkNumber = 1;
    while(data.Count > 0) {
        // Create the data event header
        var dataEventHeader = new byte[TRANSMISSION_HEADER_SIZE];
        dataEventHeader[0] = 1; // Event type
        dataEventHeader[1] = (byte)chunkNumber; // Chunk number
        messageBytes.CopyTo(dataEventHeader, 2); // Message ID

        // Create the data event message
        payload = [..dataEventHeader, ..getNextChunk(data, chunkSize)];

        // Write the data event message
        await transmissionCharacteristic.WriteValueAsync(payload, new Dictionary<string, object>());

        // Increment the chunk number
        chunkNumber++;
    }
}

byte[] getNextChunk(List<byte> data, int chunkSize) {
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



async Task<Device?> HandleDeviceFound(Adapter sender, DeviceFoundEventArgs eventArgs, String target, CancellationTokenSource cancellationTokenSource)
{
    var device = eventArgs.Device;
    var props = await device.GetAllAsync();
    if (props.Name == target) {
        Console.WriteLine($"Found target device");
        cancellationTokenSource.Cancel();
        return device;
    } else {
        Console.WriteLine($"Found non-target device: '{props.Name}'");
    }
    return null;
};

async Task PrintAdapterInfo(Adapter adapter)
{
    // Get adapter properties
    var address = await adapter.GetAddressAsync();
    var alias = await adapter.GetAliasAsync();
    var powered = await adapter.GetPoweredAsync();
    var discovering = await adapter.GetDiscoveringAsync();
    var uuids = await adapter.GetUUIDsAsync();

    // Print the information
    Console.WriteLine("Adapter Info:");
    Console.WriteLine($"  Address: {address}");
    Console.WriteLine($"  Alias: {alias}");
    Console.WriteLine($"  Powered: {powered}");
    Console.WriteLine($"  Discovering: {discovering}");
    Console.WriteLine("  UUIDs: ");
    foreach (var uuid in uuids)
    {
        Console.WriteLine($"    {uuid}");
    }
}

async Task PrintDeviceInfo(Device device)
{
    var address = await device.GetAddressAsync();
    var name = await device.GetNameAsync();
    var alias = await device.GetAliasAsync();
    var paired = await device.GetPairedAsync();
    var connected = await device.GetConnectedAsync();

    Console.WriteLine("Device Info:");
    Console.WriteLine($"  Address: {address}");
    Console.WriteLine($"  Name: {name}");
    Console.WriteLine($"  Alias: {alias}");
    Console.WriteLine($"  Paired: {paired}");
    Console.WriteLine($"  Connected: {connected}");
}

async Task PrintServiceInfo(IGattService1 service)
{
    // Retrieve all details about the service
    var serviceInfo = await service.GetAllAsync();

    // Print Service details
    Console.WriteLine($"  Service: {serviceInfo.UUID}");
    Console.WriteLine($"    Includes: {string.Join(", ", serviceInfo.Includes.Select(include => include.ToString()))}");
    Console.WriteLine($"    Primary: {serviceInfo.Primary}");

    // Print Characteristics for the Service
    foreach (var characteristic in await service.GetCharacteristicsAsync())
    {
        await PrintCharacteristicInfo(characteristic);
    }
}

async Task PrintCharacteristicInfo(IGattCharacteristic1 characteristic)
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

async Task MessageReceived(List<byte> data) {
    Console.WriteLine($"Received message of length: {data.Count}");
    Console.WriteLine($"Message: {Encoding.ASCII.GetString(data.ToArray())}");
}

class MessageInformation {
    public UInt16 TotalChunks { get; set; }
    public List<byte> Data { get; set; } = [];
}
