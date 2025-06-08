#!/usr/bin/env python3
"""
Test script for ZYNQ7010 CDC-ACM Virtual Serial Port Device

This script can be used on a PC to communicate with the USB CDC-ACM virtual serial port device.
It will send data to the device and read the echo responses.

Requirements:
- PySerial library: pip install pyserial

Usage:
    python test_usb_device.py [COM_PORT]

    If COM_PORT is not specified, the script will try to auto-detect the device.

Examples:
    python test_usb_device.py              # Auto-detect device
    python test_usb_device.py COM3         # Windows - use specific port
    python test_usb_device.py /dev/ttyACM0 # Linux - use specific port

Features:
- Auto-detection of ZYNQ7010 CDC device (VID: 0x0D7D, PID: 0x1234)
- Fallback detection for other CDC-ACM devices
- Automated test message sending
- Interactive communication mode
- Support for Chinese characters and special characters
- Background reading thread for real-time responses

The script will:
1. Scan for compatible CDC-ACM devices
2. Connect to the selected device
3. Send several test messages
4. Enter interactive mode for manual testing
5. Display all responses from the device

Expected device behavior:
- Device should enumerate as "ZYNQ7010 CDC Virtual Serial Port"
- Each message should be echoed back with format: "[Echo #N] <your_message>"
- Response counter increments for each received message
"""

import serial
import serial.tools.list_ports
import time
import sys
import threading

def find_cdc_device():
    """Find the CDC-ACM virtual serial port device"""
    print("Scanning for CDC-ACM devices...")
    
    ports = serial.tools.list_ports.comports()
    cdc_ports = []
    
    for port in ports:
        # Look for devices that might be our ZYNQ CDC device
        # Check for our specific VID/PID first, then fallback to description matching
        if port.vid == 0x0d7d and port.pid == 0x1234:
            cdc_ports.append(port)
            print(f"Found ZYNQ7010 CDC device: {port.device} - {port.description}")
        elif port.description and (
           ('CDC' in port.description.upper()) or \
           ('SERIAL' in port.description.upper() and 'USB' in port.description.upper()) or \
           ('ZYNQ' in port.description.upper()) or \
           ('VIRTUAL' in port.description.upper() and 'COM' in port.description.upper())):
            cdc_ports.append(port)
            print(f"Found potential CDC device: {port.device} - {port.description}")
    
    if not cdc_ports:
        print("No CDC-ACM devices found.")
        print("Available ports:")
        for port in ports:
            print(f"  {port.device} - {port.description} (VID: 0x{port.vid:04x}, PID: 0x{port.pid:04x})")
        return None
    
    if len(cdc_ports) == 1:
        return cdc_ports[0].device
    else:
        print("Multiple CDC-ACM devices found:")
        for i, port in enumerate(cdc_ports):
            print(f"  {i+1}: {port.device} - {port.description}")
        try:
            choice = int(input("Select device (1-{}): ".format(len(cdc_ports)))) - 1
            if 0 <= choice < len(cdc_ports):
                return cdc_ports[choice].device
        except (ValueError, IndexError):
            pass
        print("Invalid selection.")
        return None

def communicate_with_device(port_name):
    """Send data to device and read responses"""
    try:
        # Open serial port
        print(f"Opening serial port: {port_name}")
        ser = serial.Serial(
            port=port_name,
            baudrate=115200,  # Common baud rate, adjust if needed
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1.0,
            xonxoff=False,
            rtscts=False,
            dsrdtr=False
        )
        
        print(f"Serial port opened: {ser.name}")
        print(f"Settings: {ser.baudrate} baud, {ser.bytesize}-{ser.parity}-{ser.stopbits}")
        
        # Give device time to initialize
        time.sleep(2)
          # Send test messages and read responses
        test_messages = [
            "Hello from PC!",
            "Test message 1",
            "Test message 2", 
            "Are you there?",
            "This is a longer test message to check buffer handling.",
            "中文测试消息",  # Test Chinese characters
            "Special chars: !@#$%^&*()",
        ]
        
        for i, msg in enumerate(test_messages):
            print(f"\nSending message {i+1}: {msg}")
            
            # Send data to device
            message_bytes = (msg + "\r\n").encode('utf-8')
            bytes_written = ser.write(message_bytes)
            ser.flush()  # Ensure data is sent
            print(f"Sent {bytes_written} bytes")
            
            # Read response
            try:
                # Read with timeout
                response = ser.read_until(b'\n', size=512)
                if response:
                    response_str = response.decode('utf-8', errors='ignore').strip()
                    print(f"Received: {response_str}")
                else:
                    print("No response received (timeout)")
            except Exception as e:
                print(f"Error reading response: {e}")
            
            time.sleep(1)
        
        # Interactive mode
        print("\nEntering interactive mode (type 'quit' to exit)...")
        print("Type messages and press Enter to send them to the device.")
        
        def read_thread():
            """Background thread to read from serial port"""
            while ser.is_open:
                try:
                    if ser.in_waiting > 0:
                        response = ser.read_until(b'\n', size=512)
                        if response:
                            response_str = response.decode('utf-8', errors='ignore').strip()
                            print(f"\nDevice: {response_str}")
                            print("You: ", end="", flush=True)
                except Exception as e:
                    if ser.is_open:
                        print(f"\nRead error: {e}")
                    break
                time.sleep(0.1)
        
        # Start read thread
        reader = threading.Thread(target=read_thread, daemon=True)
        reader.start()
        
        try:
            while True:
                user_input = input("You: ")
                if user_input.lower() in ['quit', 'exit', 'q']:
                    break
                
                # Send user input
                message_bytes = (user_input + "\r\n").encode('utf-8')
                ser.write(message_bytes)
                ser.flush()
                
        except KeyboardInterrupt:
            print("\nExiting interactive mode...")
        
        ser.close()
        print("Serial port closed.")
            
    except serial.SerialException as e:
        print(f"Serial Error: {e}")
    except Exception as e:
        print(f"Error: {e}")

def main():
    print("ZYNQ7010 CDC-ACM Virtual Serial Port Test")
    print("=========================================")
    
    port_name = None
    
    # Check if port is specified as command line argument
    if len(sys.argv) > 1:
        port_name = sys.argv[1]
        print(f"Using specified port: {port_name}")
    else:
        # Try to find the device automatically
        port_name = find_cdc_device()
    
    if port_name is None:
        print("No suitable device found or selected.")
        return 1
    
    # Communicate with the device
    communicate_with_device(port_name)
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
