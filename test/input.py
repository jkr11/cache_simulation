import csv
import random

def generate_cache_simulation_data(num_entries, address_pool_size, repetition_probability):
    operations = ['R', 'W']
    address_pool = [random.getrandbits(32) for _ in range(address_pool_size)]
    data = []

    for _ in range(num_entries):
        operation = random.choice(operations)
        address1 = random.choices(address_pool, k=1)[0] if random.random() < repetition_probability else random.getrandbits(32)
        address2 = random.choices(address_pool, k=1)[0] if random.random() < repetition_probability else random.getrandbits(32)
        data.append((operation, f"0x{address1:08X}", f"0x{address2:08X}"))
    
    return data

def save_to_csv(data, filename):
    with open(filename, mode='w', newline='') as file:
        writer = csv.writer(file)
        for row in data:
            writer.writerow(row)

if __name__ == "__main__":
    num_entries = 1000 
    address_pool_size = 900
    repetition_probability = 0.0

    data = generate_cache_simulation_data(num_entries, address_pool_size, repetition_probability)
    save_to_csv(data, 'cache_simulation_data.csv')
    print(f"{num_entries} written to 'cache_simulation_data.csv'")