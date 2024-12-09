using HashtagChris.DotNetBlueZ;
using HashtagChris.DotNetBlueZ.Extensions;
using System.Text;
using System.Text.Json;

// Bluetooth Information
const string TARGET                 = "Smart Plug";

// Define the ADT service and characteristic UUIDs
const string ADT_SERVICE_UUID =                                "cc3aab60-0001-0000-81e0-c88f19fb28cb";
const string ADT_SERVICE_MTU_CHARACTERISTIC_UUID =             "cc3aab60-0001-0001-81e0-c88f19fb28cb";
const string ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID =    "cc3aab60-0001-0002-81e0-c88f19fb28cb";
const string ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID =         "cc3aab60-0001-0003-81e0-c88f19fb28cb";

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
/// Create the ADT service
///////////////////////////////////////////////////////////////////////////////

AdtService adtService = new(
    ADT_SERVICE_UUID,
    ADT_SERVICE_MTU_CHARACTERISTIC_UUID,
    ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID,
    ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID,
    async (List<Byte> data) => {
        Console.WriteLine($"Received data: {Encoding.ASCII.GetString([.. data])}");
    }
);

// Initialize the service
if (!await adtService.Init(device)) {
    Console.WriteLine("Failed to initialize the ADT service.");
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
/// Transmit a SetBleConfig message to the device
///////////////////////////////////////////////////////////////////////////////

// Create an object to store the connection information
var setBleConfigMessage = new {
    type = "SetBleConfig",
    data = new {
        deviceName = TARGET,
    }
};

// Convert to string using JSON serialization
string jsonString = JsonSerializer.Serialize(setBleConfigMessage);
Console.WriteLine(jsonString);

var success = await adtService.Transmit([.. Encoding.ASCII.GetBytes(jsonString)]);
Console.WriteLine($"Transmit success: {success}");

while (true) {
    await Task.Delay(1000);
}

return 0;

///////////////////////////////////////////////////////////////////////////////
/// Helper functions
///////////////////////////////////////////////////////////////////////////////

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