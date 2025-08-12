#!/usr/bin/env python3

import serial
import socket
import signal
import sys
import argparse
import threading
import time

SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 115200

COSMOS_HARDLINE_TELEMETRY_PORT = 2950
COSMOS_HARDLINE_OUTGOING_COMMANDS_PORT = 2951
CFDP_TEST_APID = 0x00

SYNC = b'\x35\x2e\xf8\x53'
cosmos_socket = None # global used to exit successfully

DEBUG = False

# Debugging function to print messages if DEBUG is enabled
def log(msg):
    if DEBUG:
        print(msg)

# Splits packets up by the header and returns an array of the packets
def split_packets(buffer: bytes, header: bytes):
    header_len = len(header)
    packets = []
    start = 0

    while True:
        # Find the header in the buffer starting from the current position
        start_pos = buffer.find(header, start)
        if start_pos == -1:
            # No more headers found
            break
        # Find the next header (or end of buffer)
        next_start_pos = buffer.find(header, start_pos + header_len)
        #check if there is another header (packet)
        if next_start_pos == -1:
            
            candidate = buffer[start_pos:]
            if len(candidate) < 10:
                break  # wait for more data
           
            packets.append(candidate)
            
            break
        else:
            # add the candidate packet to packets
            packets.append(buffer[start_pos:next_start_pos])
            # Update start position to search for the next packet
            start = next_start_pos

    return packets


def hardline_tlm(simulator_port):
    global cosmos_socket
    ser = None
    while True:
        try:
            port = simulator_port if simulator_port else SERIAL_PORT
            ser = serial.Serial(port, BAUD_RATE)
            break
        except Exception:
            print("FSW data port not found, retrying...")
            time.sleep(10)

    print("✅ Connected to FSW data port")

    cosmos_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    cosmos_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    cosmos_socket.bind(('0.0.0.0', COSMOS_HARDLINE_TELEMETRY_PORT))
    cosmos_socket.listen(1)
    cosmos_client, _ = cosmos_socket.accept()
    cosmos_client.setblocking(False)
    print("✅ Connected to COSMOS - hardline telemetry")

    serial_buffer = b""

    while True:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            log(f"\n📥 New serial chunk ({len(data)} bytes): {data.hex()}")
            serial_buffer += data

            packets = split_packets(serial_buffer, SYNC)
            total_consumed = 0

            for i, packet in enumerate(packets):
                if len(packet) < 10:
                    log(f"⚠️ Packet #{i+1} too short, skipping")
                    continue

                if packet[:4] != SYNC:
                    log(f"❌ Invalid sync bytes: {packet[:4].hex()}")
                    continue

                apid = int.from_bytes(packet[4:6], 'big') & 0x7FF
                seq = int.from_bytes(packet[6:8], 'big')
                declared_len = int.from_bytes(packet[8:10], 'big')
                actual_len = len(packet) - 11

                log(f"\n--- PACKET #{i+1} ({len(packet)} bytes) ---")
                log(packet.hex())
                log(f"🛠️ APID: {apid} | Seq: {seq}")
                log(f"📏 Declared: {declared_len} | Actual: {actual_len}")

                if declared_len != actual_len:
                    log("❌ CCSDS length mismatch")
                    continue

                try:
                    cosmos_client.send(packet)
                    log("✅ Sent to COSMOS")
                except Exception as e:
                    log(f"🚨 Send failed: {e}")

                total_consumed += len(packet)

            if total_consumed > 0:
                serial_buffer = serial_buffer[total_consumed:]


def hardline_commands(simulator_port):
    cosmos_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    cosmos_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    cosmos_socket.bind(('0.0.0.0', COSMOS_HARDLINE_OUTGOING_COMMANDS_PORT))
    cosmos_socket.listen(1)
    cosmos_client, _ = cosmos_socket.accept()
    cosmos_client.setblocking(True)
    print("✅ Connected to COSMOS - hardline commands")

    while True:
        dat = cosmos_client.recv(257)
        if dat == b'':
            continue

        log(f"⬅️ Command from COSMOS ({len(dat)} bytes): {dat.hex()}")

        # define the serial port to use
        port = simulator_port if simulator_port else SERIAL_PORT
        with serial.Serial(port, baudrate=BAUD_RATE) as ser:
            ser.write(dat)  # pass along data to the radio serial port




def signal_handler(sig, frame):
    print('🛑 Closing COSMOS socket...')
    if cosmos_socket != None: cosmos_socket.close()
    sys.exit(0)


def main():
    global DEBUG
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', type=str, help="Simulator port (e.g. /dev/pts/7)")
    parser.add_argument('--debug', action='store_true', help="Enable debug output")
    args = parser.parse_args()

    DEBUG = args.debug
    sim_port = args.s

    signal.signal(signal.SIGINT, signal_handler)
    while True:
        try:

            thread1 = threading.Thread(target=hardline_tlm, args=(sim_port,))
            thread2 = threading.Thread(target=hardline_commands, args=(sim_port,))

            thread1.start()
            thread2.start()
            thread1.join()
            thread2.join()

        except Exception as e:
            if e == KeyboardInterrupt: break
            print("ERROR:", e)

if __name__ == "__main__":
    main()
