import csv
import random
import sys

def is_power_of_two(n):
    return (n & (n - 1) == 0) and n != 0

if len(sys.argv) < 2:
    print("Usage: python script_name.py <cache_line_size>")
    sys.exit(1)

try:
    cache_line_size = int(sys.argv[1])
    if not is_power_of_two(cache_line_size):
        print("The cache line size must be a power of two.")
        sys.exit(1)
except ValueError:
    print("Invalid cache line size. Please enter a valid integer.")
    sys.exit(1)

num_cache_lines = 512

base_addresses = [i * cache_line_size for i in range(num_cache_lines)]
data_values = [
    "0x11111111", "0x22222222", "0x33333333", "0x44444444", "0x55555555",
    "0x66666666", "0x77777777", "0x88888888", "0x99999999","0x12345678",
    "0x79824691", "0x16420982", "0x15946378", "0x687b890a","0xf809b234",
]

accesses = []

#for i, base in enumerate(base_addresses):
#    accesses.append(["W", f"0x{base:08X}", data_values[i]])

num_operations = 1000
for i in range(num_operations):
    base = random.choice(base_addresses)
    offset = random.randint(0, 3)
    address = base + offset
    if random.choice([True, False]):
        accesses.append(["R", f"0x{address:08X}", ""])
    else:
        data = random.choice(data_values)
        accesses.append(["W", f"0x{address:08X}", data])

file_path = "cache_accesses.csv"
with open(file_path, mode="w", newline="") as file:
    writer = csv.writer(file)
    writer.writerows(accesses)

print(f"Generated cache accesses and saved to {file_path}")