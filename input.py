import csv
import random

def is_power_of_two(n):
    return (n & (n - 1) == 0) and n != 0

cache_line_size = int(input("Enter the cache line size (power of two in bytes): "))
while not is_power_of_two(cache_line_size):
    cache_line_size = int(input("Enter the cache line size (power of two in bytes): "))

num_cache_lines = 3

# Base addresses based on cache line size
base_addresses = list(range(num_cache_lines))

# Data values (each represents 4 bytes)
data_values = [
    "0x11111111", "0x22222222", "0x33333333", "0x44444444", "0x55555555",
    "0x66666666", "0x77777777", "0x88888888", "0x99999999"
]

# Initialize cache with None values
cache = [None] * (num_cache_lines * 4)

# Insert initial data into the cache
for i, base in enumerate(base_addresses):
    for j in range(4):  # Each base address can hold 4 data items
        cache[base * 4 + j] = data_values[i]

# Prepare accesses list for CSV writing
accesses = []

for i, base in enumerate(base_addresses):
    accesses.append(["W", f"0x{base:08X}", data_values[i]])

# Read and write operations with 0 to 3 byte offset
num_operations = 100
for _ in range(num_operations):
    base = random.choice(base_addresses)
    offset = random.randint(0, 3)  # Offset within the 4 bytes
    address = base * 4 + offset
    if random.choice([True, False]):
        accesses.append(["R", f"0x{address:08X}", cache[address]])
    else:
        data = random.choice(data_values)
        accesses.append(["W", f"0x{address:08X}", data])
        cache[address] = data

# Write accesses to CSV file
file_path = "cache_accesses.csv"
with open(file_path, mode="w", newline="") as file:
    writer = csv.writer(file)
    writer.writerows(accesses)

print(f"Generated cache accesses and saved to {file_path}")

# Function to print the cache
def print_cache(cache):
    print("Cache Contents:")
    for address in range(num_cache_lines):
        print(f"Base Address 0x{address:08X}: ", end="")
        for j in range(4):
            idx = address * 4 + j
            print(f"{cache[idx]} ", end="")
        print()
    print()

print_cache(cache)
