# ZeroMQ Unity Bridge

A lightweight library to facilitate communication between Unity and other systems using ZeroMQ.

## Overview

The ZeroMQ Unity Bridge library provides an abstraction layer for using ZeroMQ in Unity projects, enabling efficient communication between Unity and external applications such as Python scripts, C++ programs, or other systems. This library is especially useful for simulation projects, like vehicle simulators, where you need to transmit sensor data from Unity to external systems and receive control commands.

## Features

- Support for all ZeroMQ communication patterns (PUB-SUB, REQ-REP, PUSH-PULL)
- Simple C API for use in Unity native plugins
- C# wrapper for easy integration with Unity scripts
- Thread-safe functions for use in concurrent environments
- Support for binary data and string transmission
- Example Python client for integration with external systems

## Communication Structure

The communication structure implemented by this library is as follows:

1. **Simulator Data Publication (PUB-SUB)**
   - Port 5555: Unity (simulator) publishes data such as camera images, vehicle telemetry, etc.
   - External clients subscribe to specific topics (e.g., "camera", "vehicle")

2. **Commands to the Simulator (PUSH-PULL or PUB-SUB)**
   - Port 5556: External clients send commands to the simulator (e.g., reset, change weather)
   - Unity receives and processes these commands

3. **Vehicle Controls (PUSH-PULL)**
   - Port 5557: External clients send controls to the vehicle (throttle, steering, brake)
   - Unity applies these controls to the simulated vehicle

## Requirements

- CMake 3.10+
- A compiler with C++17 support
- ZeroMQ (libzmq) and C++ headers (cppzmq)
- Unity 2019.4 or higher (for use as a Unity plugin)

## Building

```bash
# Clone the repository
git clone https://github.com/akadjoker/ZeroMQUnityBridge.git
cd ZeroMQUnityBridge

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build .
```

## Unity Integration

1. Copy the compiled library files to your Unity project:
   - Windows: `ZeroMQUnityBridge.dll` to `Assets/Plugins`
   - macOS: `ZeroMQUnityBridge.bundle` to `Assets/Plugins`
   - Linux: `libZeroMQUnityBridge.so` to `Assets/Plugins`

2. Add the `ZMQPlugin.cs` script to your Unity project

3. Attach the `ZMQPlugin` component to a GameObject in your scene

4. Configure the necessary sockets through the Inspector or via code

## Usage Example in Unity

```csharp
// Get the ZeroMQ plugin reference
ZMQPlugin zmq = GetComponent<ZMQPlugin>();

// Set up sockets
zmq.SetupPublisher("camera_publisher", "tcp://*:5555");
zmq.SetupSubscriber("command_subscriber", "tcp://*:5556", "command");
zmq.SetupPullSocket("control_receiver", "tcp://*:5557");

// Publish camera data
public void PublishCameraFrame(byte[] imageData)
{
    zmq.PublishData("camera_publisher", "camera", imageData);
}

// Publish vehicle telemetry
public void PublishVehicleData(Vector3 position, Vector3 rotation, float speed)
{
    string json = JsonUtility.ToJson(new {
        position = new float[] { position.x, position.y, position.z },
        rotation = new float[] { rotation.x, rotation.y, rotation.z },
        speed = speed
    });
    
    zmq.PublishString("camera_publisher", "vehicle", json);
}

// In the Update method to receive controls
void Update()
{
    string controlData = zmq.ReceiveString("control_receiver");
    if (controlData != null)
    {
        // Parse JSON and apply controls to the vehicle
        VehicleControl control = JsonUtility.FromJson<VehicleControl>(controlData);
        ApplyVehicleControl(control.throttle, control.steering, control.brake);
    }
}
```

## Python Client Example

An included Python client for easy integration with external systems:

```python
from simulator_client import SimulatorClient

# Create the client
client = SimulatorClient(host="localhost")

# Connect to the simulator
client.connect()

# Subscribe to camera data
def on_camera_data(data):
    print(f"Received camera frame: {len(data['raw_data'])} bytes")

client.subscribe("camera", on_camera_data)

# Send controls to the vehicle
client.send_vehicle_control(throttle=0.5, steering=0.0, brake=0.0)

# Send command to the simulator
client.send_command("reset_simulation")
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributions

Contributions are welcome! Please feel free to submit pull requests or open issues to improve this library.

## Adding New Features

The library is designed to be easily extensible. Here are some common ways to extend it:

1. **Adding new message topics:**
   - Simply define new topic names in your Unity code and Python clients
   - For example, to add LIDAR data, you can use a new "lidar" topic name

2. **Adding new command types:**
   - Extend the command processing logic in your Unity code
   - Use the same ZeroMQ sockets but interpret the JSON data differently

3. **Adding new socket patterns:**
   - If you need more specialized communication patterns, you can add new socket types in the C++ library
   - Extend the ZMQPlugin.cs script to expose these new socket types

4. **Performance optimizations:**
   - Implement compression for large data (like camera images)
   - Use binary serialization instead of JSON for high-frequency data

## Troubleshooting

1. **Socket binding errors:**
   - Make sure no other applications are using the same ports
   - Check firewall settings if communicating between different machines

2. **Data not being received:**
   - Ensure the topic names match exactly between publisher and subscriber
   - Check that the endpoints (IP addresses and ports) are correctly configured

3. **Unity crashes:**
   - Make sure ZeroMQ DLLs are correctly placed in the Plugins folder
   - Ensure you're not blocking the main Unity thread with synchronous operations

4. **High latency:**
   - Consider using a more efficient serialization method for large data
   - Optimize the polling frequency to balance responsiveness and CPU usage
