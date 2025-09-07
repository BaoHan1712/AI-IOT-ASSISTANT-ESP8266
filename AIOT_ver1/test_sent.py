import socket
import time

ESP32_IP = "192.168.1.16"  # thay bằng IP của ESP32
ESP32_PORT = 8080
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((ESP32_IP, ESP32_PORT))

def send_packet(data_byte):
    
    packet = bytes([0x02, ord(data_byte), 0x03])
    s.send(packet)
    print(f"Sent: {[hex(b) for b in packet]}")

while True:
    time.sleep(4)
    send_packet('1')
    time.sleep(0.3)
    send_packet('3')
    time.sleep(0.3)
    send_packet('5')
    time.sleep(5)
    send_packet('0')
    time.sleep(0.3)
    send_packet('4')
    time.sleep(0.3)
    send_packet('6')
    time.sleep(0.3)
    
    
