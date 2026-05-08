import sys

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 hex2mem.py <input.hex> <output.mem>")
        sys.exit(1)
        
    in_file = sys.argv[1]
    out_file = sys.argv[2]
    
    memory = {}
    
    # Simple Intel HEX parser
    with open(in_file, 'r') as f:
        ext_addr = 0
        for line in f:
            line = line.strip()
            if not line.startswith(':'): continue
            
            byte_count = int(line[1:3], 16)
            address = int(line[3:7], 16)
            record_type = int(line[7:9], 16)
            
            if record_type == 0x00: # Data Record
                data = line[9:9+(byte_count*2)]
                for i in range(byte_count):
                    abs_addr = ext_addr + address + i
                    memory[abs_addr] = int(data[i*2:i*2+2], 16)
            elif record_type == 0x04: # Extended Linear Address Record
                ext_addr = int(line[9:13], 16) << 16
            elif record_type == 0x01: # End Of File Record
                break

    # Find max address to know how much to write
    if not memory:
        max_addr = 0
    else:
        max_addr = max(memory.keys())
        
    # Align to 4 byte boundary
    max_addr = (max_addr + 3) & ~3
    
    # Write to memh format (32-bit words, hex)
    with open(out_file, 'w') as f:
        for addr in range(0, max_addr, 4):
            b0 = memory.get(addr, 0)
            b1 = memory.get(addr+1, 0)
            b2 = memory.get(addr+2, 0)
            b3 = memory.get(addr+3, 0)
            
            word = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0
            f.write(f"{word:08X}\n")

if __name__ == "__main__":
    main()
