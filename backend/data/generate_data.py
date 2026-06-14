import csv
import random

# Number of properties to generate
NUM_PROPERTIES = 100000

# Geographic bounds (approximate contiguous USA)
MIN_LON = -125.0
MAX_LON = -65.0
MIN_LAT = 25.0
MAX_LAT = 49.0

# Cities for random locations
CITIES = ["New York", "Los Angeles", "Chicago", "Houston", "Phoenix", "Philadelphia", "San Antonio", "San Diego", "Dallas", "San Jose", "Austin", "Jacksonville", "Fort Worth", "Columbus", "San Francisco", "Charlotte", "Indianapolis", "Seattle", "Denver", "Washington"]
STREETS = ["Main St", "Oak St", "Pine St", "Maple Ave", "Cedar Ln", "Elm St", "Washington St", "Lakeview Dr", "Hillcrest Ave", "Sunset Blvd", "Park Ave", "Broadway", "Market St", "Spring St", "River Rd"]

def generate_property(index):
    # Location string
    street_num = random.randint(100, 9999)
    street = random.choice(STREETS)
    city = random.choice(CITIES)
    location = f"{street_num} {street}, {city}"
    
    # Pricing, Area, Bedrooms
    bedrooms = random.randint(1, 6)
    area = random.randint(500, 5000) * bedrooms / 3.0
    price = random.randint(100000, 2000000) * (area / 1000.0)
    
    # Bounding Box (Lat/Lon)
    # Generate a center point
    center_x = random.uniform(MIN_LON, MAX_LON)
    center_y = random.uniform(MIN_LAT, MAX_LAT)
    
    # Generate a small delta for the bounding box (property lot size)
    # Roughly 0.0001 to 0.0005 degrees
    delta_x = random.uniform(0.0001, 0.0005)
    delta_y = random.uniform(0.0001, 0.0005)
    
    x_min = center_x - delta_x
    x_max = center_x + delta_x
    y_min = center_y - delta_y
    y_max = center_y + delta_y
    
    return [location, round(price, 2), round(area, 2), bedrooms, round(x_min, 6), round(y_min, 6), round(x_max, 6), round(y_max, 6)]

def main():
    with open('properties.csv', 'w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(['location', 'price', 'area', 'bedrooms', 'x_min', 'y_min', 'x_max', 'y_max'])
        for i in range(NUM_PROPERTIES):
            writer.writerow(generate_property(i))
            if (i + 1) % 10000 == 0:
                print(f"Generated {i + 1} properties...")
    
    print("Successfully generated properties.csv")

if __name__ == '__main__':
    main()
