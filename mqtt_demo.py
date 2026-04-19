#!/usr/bin/env python3
"""
Demo script for ESP32 Signal Sensor MQTT control.

Requirements:
    pip install paho-mqtt

Usage:
    python3 mqtt_demo.py <broker_ip> [broker_port]
    
Example:
    python3 mqtt_demo.py 192.168.1.134 1883
"""

import paho.mqtt.client as mqtt
import json
import queue
import time
import sys
import ssl
import os
import threading

BROKER_IP = "192.168.1.134"
BROKER_PORT = 8883
CA_CERT = os.path.join(os.path.dirname(__file__), "mosquitto", "certs", "ca.crt")
COMMAND_TOPIC = "iot/sensor/command"
BENCHMARK_TOPIC = "iot/sensor/benchmark"
AGGREGATE_TOPIC = "iot/sensor/aggregate"
GENERATOR_TOPIC = "iot/generator/signal"

client = None
received_messages = []
connected_event = threading.Event()
message_queue = queue.Queue()
print_lock = threading.Lock()
current_prompt = ""
_printer_running = False


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"✓ Connected to broker at {BROKER_IP}:{BROKER_PORT}")
        client.subscribe(BENCHMARK_TOPIC)
        client.subscribe(AGGREGATE_TOPIC)
        client.subscribe(GENERATOR_TOPIC)
        print(f"✓ Subscribed to {BENCHMARK_TOPIC}")
        print(f"✓ Subscribed to {AGGREGATE_TOPIC}")
        print(f"✓ Subscribed to {GENERATOR_TOPIC}")
        connected_event.set()
    else:
        print(f"✗ Connection failed with code {rc}")


def _format_message(topic, data, is_raw):
    if is_raw:
        return f"  [{topic}] {data}"
    if "type" in data:
        lines = [f"  [{topic}] {data['type']}:"]
        for key, value in data.items():
            if key == "type":
                continue
            lines.append(f"    {key}: {value:.2f}" if isinstance(value, float) else f"    {key}: {value}")
        return "\n".join(lines)
    parts = ", ".join(
        f"{k}: {v:.2f}" if isinstance(v, float) else f"{k}: {v}"
        for k, v in data.items()
    )
    return f"  [{topic}] {parts}"


def _background_printer():
    """Background thread: drains the message queue and reprints the active prompt."""
    global _printer_running
    while _printer_running:
        try:
            topic, data, is_raw = message_queue.get(timeout=0.2)
        except queue.Empty:
            continue
        received_messages.append((topic, data))
        with print_lock:
            # Clear current prompt line, print message, reprint prompt
            sys.stdout.write(f"\r\033[K{_format_message(topic, data, is_raw)}\n")
            if current_prompt:
                sys.stdout.write(current_prompt)
            sys.stdout.flush()


def start_printer():
    global _printer_running
    _printer_running = True
    t = threading.Thread(target=_background_printer, daemon=True)
    t.start()


def stop_printer():
    global _printer_running
    _printer_running = False


def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode())
        message_queue.put((msg.topic, payload, False))
    except json.JSONDecodeError:
        message_queue.put((msg.topic, msg.payload.decode(), True))


def prompt_input(prompt_text):
    """input() wrapper that registers the active prompt for the background printer."""
    global current_prompt
    with print_lock:
        current_prompt = prompt_text
        sys.stdout.write(prompt_text)
        sys.stdout.flush()
    result = sys.stdin.readline().rstrip("\n")
    with print_lock:
        current_prompt = ""
    return result


def on_disconnect(client, userdata, rc):
    if rc != 0:
        print(f"✗ Unexpected disconnection: {rc}")


def send_command(cmd_name, cmd_dict):
    """Send a command via MQTT."""
    global client
    payload = json.dumps(cmd_dict)
    print(f"\n→ Sending: {cmd_name}")
    print(f"  Payload: {payload}")
    
    info = client.publish(COMMAND_TOPIC, payload)
    if info.rc == mqtt.MQTT_ERR_SUCCESS:
        print(f"  ✓ Published successfully")
    else:
        print(f"  ✗ Publish failed: {info.rc}")
    
    time.sleep(0.5)


def demo_interactive():
    """Interactive demo mode."""
    global client
    
    print("\n" + "="*60)
    print("ESP32 Signal Sensor - Interactive MQTT Control Demo")
    print("="*60)
    print("\nAvailable commands:")
    print("  1. Lock sample rate to 50 Hz")
    print("  2. Lock sample rate to 100 Hz")
    print("  3. Lock sample rate to 10 Hz")
    print("  4. Release sample rate (adaptive mode)")
    print("  5. Set FFT window size (64 samples - max resolution)")
    print("  6. Set FFT window size (128 samples - balanced)")
    print("  7. Set FFT window size (256 samples - min latency)")
    print("  8. Run max sampling benchmark")
    print("  9. Run sampling analysis demo")
    print("  10. Reset device")
    print("  11. Exit")
    
    while True:
        try:
            choice = prompt_input("\nSelect command (1-11): ").strip()
            
            if choice == "1":
                send_command("Lock sample rate to 50 Hz", 
                           {"cmd": "set_sample_rate", "value": 50.0})
                time.sleep(1)
            elif choice == "2":
                send_command("Lock sample rate to 100 Hz", 
                           {"cmd": "set_sample_rate", "value": 100.0})
                time.sleep(1)
            elif choice == "3":
                send_command("Lock sample rate to 10 Hz", 
                           {"cmd": "set_sample_rate", "value": 10.0})
                time.sleep(1)
            elif choice == "4":
                send_command("Release sample rate (back to adaptive)", 
                           {"cmd": "release_sample_rate"})
                time.sleep(1)
            elif choice == "5":
                send_command("Set FFT window to 64 samples (max resolution)", 
                           {"cmd": "set_fft_window", "value": 64})
                time.sleep(1)
            elif choice == "6":
                send_command("Set FFT window to 128 samples (balanced)", 
                           {"cmd": "set_fft_window", "value": 128})
                time.sleep(1)
            elif choice == "7":
                send_command("Set FFT window to 256 samples (min latency)", 
                           {"cmd": "set_fft_window", "value": 256})
                time.sleep(1)
            elif choice == "8":
                send_command("Start max sampling benchmark", 
                           {"cmd": "start_benchmark"})
                time.sleep(3)
            elif choice == "9":
                send_command("Start sampling analysis demo", 
                           {"cmd": "sampling_demo"})
                time.sleep(3)
            elif choice == "10":
                confirm = prompt_input("Are you sure? This will restart the device (y/n): ").strip()
                if confirm.lower() == "y":
                    send_command("Reset device", {"cmd": "reset"})
                    time.sleep(5)
                    break
                else:
                    print("  Cancelled.")
            elif choice == "11":
                break
            else:
                print("  Invalid choice, try again.")
        except KeyboardInterrupt:
            print("\n\nExiting...")
            break


def demo_automated():
    """Automated demo sequence."""
    global client
    
    print("\n" + "="*60)
    print("ESP32 Signal Sensor - Automated MQTT Demo Sequence")
    print("="*60)
    print("\nStarting automated demo in 2 seconds...\n")
    time.sleep(2)
    
    # 1. Max sampling benchmark
    print("\n[STEP 1] Running max sampling benchmark...")
    print("This tests the raw ADC throughput of your ESP32.")
    send_command("Start benchmark", {"cmd": "start_benchmark"})
    time.sleep(4)
    
    # 2. Sampling analysis
    print("\n[STEP 2] Running sampling analysis demo...")
    print("This shows Nyquist vs. adaptive sampling rates.")
    send_command("Start sampling demo", {"cmd": "sampling_demo"})
    time.sleep(4)
    
    # 3. Lock sample rate demo
    print("\n[STEP 3] Demonstrating sample rate lock/release...")
    print("Valid range: 10–1000 Hz (clamped automatically)")
    
    print("\n  3a. Locking sample rate to 100 Hz...")
    print("  Rate stays fixed regardless of FFT frequency detection.")
    send_command("Lock sample rate to 100 Hz", 
               {"cmd": "set_sample_rate", "value": 100.0})
    time.sleep(2)
    
    print("\n  3b. Locking sample rate to 10 Hz...")
    print("  Low rate: reduces data volume and power consumption.")
    send_command("Lock sample rate to 10 Hz", 
               {"cmd": "set_sample_rate", "value": 10.0})
    time.sleep(2)
    
    print("\n  3c. Releasing sample rate back to adaptive mode...")
    print("  adaptSamplingRate() takes over on the next FFT window.")
    send_command("Release sample rate to adaptive", 
               {"cmd": "release_sample_rate"})
    time.sleep(2)
    
    # 4. FFT window tradeoff demo
    print("\n[STEP 4] Demonstrating FFT window size tradeoff...")
    print("Valid range: 64–256 samples (clamped automatically)")
    
    print("\n  4a. Setting FFT window to 64 samples (max resolution)...")
    print("  Frequency resolution: sampleRate / 64 Hz/bin")
    print("  Latency: Slower (more samples to fill)")
    send_command("Set FFT window to 64 samples", 
               {"cmd": "set_fft_window", "value": 64})
    time.sleep(2)
    
    print("\n  4b. Setting FFT window to 256 samples (min latency)...")
    print("  Frequency resolution: sampleRate / 256 Hz/bin")
    print("  Latency: Faster (fewer samples to fill)")
    send_command("Set FFT window to 256 samples", 
               {"cmd": "set_fft_window", "value": 256})
    time.sleep(2)
    
    print("\n" + "="*60)
    print("Demo complete!")
    print("="*60)


def main():
    global client
    
    # Parse arguments
    if len(sys.argv) > 1:
        broker_ip = sys.argv[1]
    else:
        broker_ip = BROKER_IP
    
    if len(sys.argv) > 2:
        broker_port = int(sys.argv[2])
    else:
        broker_port = BROKER_PORT
    
    print("="*60)
    print("ESP32 Signal Sensor - MQTT Control Demo")
    print("="*60)
    print(f"Broker: {broker_ip}:{broker_port}")
    print(f"Command topic: {COMMAND_TOPIC}\n")
    
    # Connect to MQTT broker
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect
    
    if os.path.exists(CA_CERT):
        client.tls_set(ca_certs=CA_CERT, tls_version=ssl.PROTOCOL_TLSv1_2)
        client.tls_insecure_set(False)
        print(f"TLS enabled with CA: {CA_CERT}")
    else:
        print(f"WARNING: CA cert not found at {CA_CERT}, connecting without TLS")
    
    try:
        print(f"Connecting to {broker_ip}:{broker_port}...")
        client.connect(broker_ip, broker_port, keepalive=60)
        client.loop_start()
        start_printer()
        
        # Wait for connection to be established
        if not connected_event.wait(timeout=5):
            print("✗ Failed to connect within 5 seconds")
            return
        
        print()  # Blank line for readability
        
        # Show menu
        print("Select demo mode:")
        print("  1. Automated sequence")
        print("  2. Interactive mode")
        
        choice = prompt_input("\nChoice (1 or 2): ").strip()
        
        if choice == "1":
            demo_automated()
        elif choice == "2":
            demo_interactive()
        else:
            print("Invalid choice.")
        
        # Show collected messages
        if received_messages:
            print("\n" + "="*60)
            print("Received Messages Summary:")
            print("="*60)
            for topic, payload in received_messages:
                print(f"\n{topic}:")
                for key, value in payload.items():
                    if isinstance(value, float):
                        print(f"  {key}: {value:.2f}")
                    else:
                        print(f"  {key}: {value}")
        
    except KeyboardInterrupt:
        print("\n\nInterrupted by user.")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        stop_printer()
        print("\nDisconnecting...")
        client.loop_stop()
        client.disconnect()
        print("Done!")


if __name__ == "__main__":
    main()
