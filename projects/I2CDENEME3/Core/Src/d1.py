
import sys
import time
import serial

PORT = "COM11"   # <- USB-TTL'nin COM numarası
BAUD = 115200
import sys, time
import serial

CMD_OUTPUT_DISABLE = bytes.fromhex("5A 05 07 00 66")
CMD_OUTPUT_ENABLE  = bytes.fromhex("5A 05 07 01 67")
CMD_SET_I2C_MODE   = bytes.fromhex("5A 05 0A 01 6A")
CMD_SAVE           = bytes.fromhex("5A 04 11 6F")
CMD_RESET          = bytes.fromhex("5A 04 02 60")

def hx(b): return " ".join(f"{x:02X}" for x in b)

def read_some(s, seconds=0.6):
    end = time.time() + seconds
    buf = bytearray()
    while time.time() < end:
        n = s.in_waiting
        if n:
            buf += s.read(n)
        else:
            time.sleep(0.01)
    return bytes(buf)

def send(s, cmd, wait=0.08, readwin=0.8):
    s.write(cmd)
    time.sleep(wait)
    r = read_some(s, readwin)
    print("TX:", hx(cmd))
    print("RX:", hx(r[-40:]) if r else "(no bytes)")
    print("-"*40)
    return r

def main():
    if len(sys.argv) < 2:
        print("Kullanim: python tfmini_diag_uart_i2c.py COM7 [baud]")
        return

    port = sys.argv[1]
    baud = int(sys.argv[2]) if len(sys.argv) >= 3 else 115200

    with serial.Serial(port, baud, timeout=0.1) as s:
        time.sleep(0.2)
        s.reset_input_buffer()

        print("\n[1] OUTPUT DISABLE")
        send(s, CMD_OUTPUT_DISABLE)

        print("\n[2] SET I2C MODE (cevap gelmeyebilir)")
        send(s, CMD_SET_I2C_MODE, readwin=0.4)

        # Burada iki senaryo var:
        # - Bazı fw'lerde UART hemen kesilir -> aşağıdaki komutlara cevap gelmez
        # - Bazılarında UART reset'e kadar devam eder -> save/reset cevap verebilir
        print("\n[3] SAVE (UART uzerinden deniyoruz)")
        send(s, CMD_SAVE, readwin=1.2)

        print("\n[4] RESET (UART uzerinden deniyoruz)")
        send(s, CMD_RESET, readwin=1.2)

        print("\n[5] OUTPUT ENABLE ile UART hala var mi testi")
        r = send(s, CMD_OUTPUT_ENABLE, readwin=1.0)

        if r:
            print("SONUC: UART hala cevap veriyor -> Sensor muhtemelen I2C moda GECMEMIS.")
        else:
            print("SONUC: UART cevapsiz -> Sensor I2C'ye gecmis OLABILIR, ama I2C hat/pull-up/wiring sorunlu olabilir.")

    print("\nBitti. Simdi sensoru POWER-CYCLE yap (5V'u kes-tak) ve Nucleo'da I2C scan dene.")

if __name__ == "__main__":
    main()
