import csv
import random


def is_power_of_two(n):
  return (n & (n - 1) == 0) and n != 0


cache_line_size = int(input("Enter the cache line size (power of two): "))
while not is_power_of_two(cache_line_size):
  print("The cache line size must be a power of two.")
  cache_line_size = int(input("Enter the cache line size (power of two): "))

# initial writing
num_cache_lines = 3

# calculate base addresses based on cache line size
base_addresses = [i * cache_line_size for i in range(num_cache_lines)]
data_values = [
    "0x11111111", "0x22222222", "0x33333333", "0x44444444", "0x55555555",
    "0x66666666", "0x77777777", "0x88888888", "0x99999999"
]

accesses = []

for i, base in enumerate(base_addresses):
  accesses.append(["W", f"0x{base:08X}", data_values[i]])

# read and write operations with 0 to 3 byte offset
num_operations = 100
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
