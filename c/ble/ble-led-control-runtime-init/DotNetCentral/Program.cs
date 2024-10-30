using HashtagChris.DotNetBlueZ;
using HashtagChris.DotNetBlueZ.Extensions;

const string TARGET = "Montana's_Beacon";

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

// Get the LED service
Console.WriteLine($"Searching for LED service.");
var serviceUuid = "00001815-0000-1000-8000-00805f9b34fb";
var service = await device.GetServiceAsync(serviceUuid);
await PrintServiceInfo(service);

// Get the LED characteristic
Console.WriteLine($"Searching for LED characteristic.");
var characteristicUuid = "00001525-1212-efde-1523-785feabcd123";
var characteristic = await service.GetCharacteristicAsync(characteristicUuid);
await PrintCharacteristicInfo(characteristic);

// Write to the characteristic a few times
var state = true;
await 5.TimesAsync(async () => {
    Console.WriteLine("Writing to characteristic.");
    // This fails if the connected device responds to (acknowledges) the write
    await characteristic.WriteValueAsync([Convert.ToByte(state)], new Dictionary<string, object>());
    await Task.Delay(1000);
    state = !state;
});


return 0;



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

    // // Print Characteristics for the Service
    // foreach (var characteristic in await service.GetCharacteristicsAsync())
    // {
    //     await PrintCharacteristicInfo(characteristic);
    // }
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

public static class IntExtensions
{
    public static async Task TimesAsync(this int count, Func<Task> func)
    {
        for (int i = 0; i < count; i++)
        {
            await func();
        }
    }
}