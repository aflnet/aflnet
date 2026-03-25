
import struct
import sys
import os

def create_modbus_frame(tid, pid, uid, func, data):
    # Transaction ID (2 bytes)
    # Protocol ID (2 bytes, 0 for Modbus/TCP)
    # Length (2 bytes, Unit ID + Func Code + Data Length)
    # Unit ID (1 byte)
    # Function Code (1 byte)
    # Data (n bytes)
    
    length = 1 + 1 + len(data)
    frame = struct.pack('>HHHBB', tid, pid, length, uid, func) + data
    return frame

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 create_seed.py <output_dir>")
        sys.exit(1)

    output_dir = sys.argv[1]
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    seeds = []

    # Seed 1: Read Holding Registers (Func 03)
    # Read 10 registers starting at address 0
    seeds.append(("read_holding_regs", create_modbus_frame(1, 0, 1, 3, struct.pack('>HH', 0, 10))))

    # Seed 2: Write Single Register (Func 06)
    # Write value 0x1234 to address 1
    seeds.append(("write_single_reg", create_modbus_frame(2, 0, 1, 6, struct.pack('>HH', 1, 0x1234))))

    # Seed 3: Read Input Registers (Func 04)
    # Read 5 registers starting at address 0
    seeds.append(("read_input_regs", create_modbus_frame(3, 0, 1, 4, struct.pack('>HH', 0, 5))))
    
    # Seed 4: Write Multiple Registers (Func 16)
    # Write 2 registers starting at address 0
    # Data: 2 bytes byte count, then values
    data = struct.pack('>HHBHH', 0, 2, 4, 0xAAAA, 0xBBBB)
    seeds.append(("write_multiple_regs", create_modbus_frame(4, 0, 1, 16, data)))

    # Seed 5: Read Coils (Func 01)
    # Read 10 coils starting at 0
    seeds.append(("read_coils", create_modbus_frame(5, 0, 1, 1, struct.pack('>HH', 0, 10))))


    for name, content in seeds:
        with open(os.path.join(output_dir, name + ".raw"), "wb") as f:
            f.write(content)
        print(f"Created {name}.raw")

if __name__ == "__main__":
    main()
