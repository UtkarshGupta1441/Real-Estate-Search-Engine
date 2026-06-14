# Real Estate Search Engine 🏙️

A lightning-fast, highly scalable Real Estate Search Engine powered by a custom **C++ R-Tree Spatial Index**, served through a **Crow C++ REST API**, and visualized on a luxurious **React + Vite** frontend.

![Screenshot](https://images.unsplash.com/photo-1600596542815-ffad4c1539a9?auto=format&fit=crop&w=1200&q=80)

## Overview
This project was engineered to solve the problem of spatial data querying. When searching for properties within a specific radius or geographic bounding box, iterating through millions of rows is incredibly slow. 

To solve this, we implemented a custom in-memory **R-Tree Index** in C++ that can organize and query 100,000+ geographical data points in sub-10 millisecond speeds.

### Key Features
- **C++ Backend**: Built with the Crow Microframework for high-performance REST API routing.
- **R-Tree Spatial Index**: Custom hierarchical Minimum Bounding Rectangle (MBR) tree algorithm to process spatial range searches instantly.
- **Data Generation**: Python scripts pre-configured to generate 100,000 synthetic properties across the United States.
- **React Frontend**: A stunning, dark-mode "Obsidian & Chrome" luxury UI built with Vite and Tailwind CSS v4.
- **Interactive Dark Map**: Integrated with `react-leaflet` to visualize real-world street maps with our custom dataset. 
- **Dockerized**: Fully containerized using `docker-compose` for zero-configuration, cross-environment deployment.

## Quick Start

You can run the entire stack locally using Docker.

```bash
# Start the Backend and Frontend together
docker compose up --build
```

- **Frontend Application**: [http://localhost:80](http://localhost:80)
- **Backend API Server**: [http://localhost:8080](http://localhost:8080)

## Project Architecture

```text
.
├── backend/                  # C++ Crow Backend
│   ├── CMakeLists.txt        # Build instructions
│   ├── src/main.cpp          # R-Tree implementation and API endpoints
│   ├── data/                 # 100,000 property dataset (properties.csv)
│   └── Dockerfile            # C++ Alpine build image
├── frontend/                 # React UI
│   ├── src/                  # React components (InteractiveMap.tsx, App.tsx)
│   ├── tailwind.config.js    # Tailwind configuration
│   └── Dockerfile            # Nginx Production Image
└── docker-compose.yml        # Orchestration
```

## API Endpoints

### `GET /api/properties/search`
Find properties near a specific coordinate.
- `x`: Longitude
- `y`: Latitude
- `distance_km`: Radius in Kilometers
- `max_price`: (Optional) Maximum price filter
- `min_area`: (Optional) Minimum area filter
- `min_bedrooms`: (Optional) Minimum bedrooms filter

### `GET /api/properties/range`
Find properties exactly within a spatial bounding box.
- `x_min, y_min`: Bottom-Left coordinates
- `x_max, y_max`: Top-Right coordinates

---

Built with ❤️ using C++, React, and Docker.