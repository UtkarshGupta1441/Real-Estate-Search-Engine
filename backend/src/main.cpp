#define _USE_MATH_DEFINES
#include "crow.h"
#include "crow/middlewares/cors.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

// Represents a bounding box or spatial region
struct BoundingBox {
    double x_min, y_min, x_max, y_max;

    BoundingBox(double x1 = 0, double y1 = 0, double x2 = 0, double y2 = 0)
        : x_min(x1), y_min(y1), x_max(x2), y_max(y2) {}

    bool intersects(const BoundingBox& other) const {
        return !(x_min > other.x_max || x_max < other.x_min || y_min > other.y_max || y_max < other.y_min);
    }
};

// Represents a property with its details and bounding box
class Property {
public:
    std::string location;
    double price;
    double area;
    int bedrooms;
    BoundingBox bbox;

    Property(const std::string& loc, double p, double a, int b, const BoundingBox& box)
        : location(loc), price(p), area(a), bedrooms(b), bbox(box) {}

    crow::json::wvalue toJson() const {
        crow::json::wvalue json_obj;
        json_obj["location"] = location;
        json_obj["price"] = price;
        json_obj["area"] = area;
        json_obj["bedrooms"] = bedrooms;
        json_obj["bbox"]["x_min"] = bbox.x_min;
        json_obj["bbox"]["y_min"] = bbox.y_min;
        json_obj["bbox"]["x_max"] = bbox.x_max;
        json_obj["bbox"]["y_max"] = bbox.y_max;
        return json_obj;
    }
};

// Represents an R-tree node which can either be an internal node or a leaf node
class RTreeNode {
public:
    std::vector<RTreeNode*> children;  // For internal nodes, this holds other nodes or properties
    std::vector<Property*> leaf_properties; // For leaf nodes, this holds properties
    BoundingBox bounding_box;
    bool is_leaf;

    RTreeNode(BoundingBox bbox, bool leaf = false)
        : bounding_box(bbox), is_leaf(leaf) {}

    // Insert a property into the node
    void insert(Property* child) {
        if (is_leaf) {
            leaf_properties.push_back(child);
            // Optional: split logic can go here for a proper R-tree
            // Currently behaving mostly as an expanding single-node or simple hierarchy
            updateBoundingBox();
        } else {
            children.push_back(new RTreeNode(child->bbox, true)); 
            children.back()->insert(child);
            updateBoundingBox();
        }
    }

    // Update the bounding box of this node based on its children
    void updateBoundingBox() {
        if (children.empty() && leaf_properties.empty()) return;
        double x_min = bounding_box.x_min;
        double y_min = bounding_box.y_min;
        double x_max = bounding_box.x_max;
        double y_max = bounding_box.y_max;

        if (is_leaf) {
            for (const auto& prop : leaf_properties) {
                x_min = std::min(x_min, prop->bbox.x_min);
                y_min = std::min(y_min, prop->bbox.y_min);
                x_max = std::max(x_max, prop->bbox.x_max);
                y_max = std::max(y_max, prop->bbox.y_max);
            }
        } else {
            for (const auto& node : children) {
                x_min = std::min(x_min, node->bounding_box.x_min);
                y_min = std::min(y_min, node->bounding_box.y_min);
                x_max = std::max(x_max, node->bounding_box.x_max);
                y_max = std::max(y_max, node->bounding_box.y_max);
            }
        }

        bounding_box = BoundingBox(x_min, y_min, x_max, y_max);
    }
};

// Represents the R-tree structure
class RTree {
    RTreeNode* root;

public:
    RTree() {
        root = new RTreeNode(BoundingBox(-180, -90, 180, 90), true); // Define an initial bounding box
    }

    // Insert a property into the R-tree
    void insert(Property* prop) {
        root->insert(prop);
    }

    // Query properties within a specified range
    std::vector<Property*> query(BoundingBox range) {
        std::vector<Property*> results;
        queryRecursive(root, range, results);
        return results;
    }

    // Query properties near a specified location and within a distance range
    std::vector<Property*> queryNearLocation(double x, double y, double distance_km, double max_price, double min_area, int min_bedrooms) {
        std::vector<Property*> results;
        // Approximate degrees to km: 1 degree latitude ~ 111 km
        double approx_degrees = distance_km / 111.0;
        BoundingBox search_area(x - approx_degrees, y - approx_degrees, x + approx_degrees, y + approx_degrees);
        std::vector<Property*> properties = query(search_area);

        for (const auto& prop : properties) {
            double dist = calculateDistance(x, y, (prop->bbox.x_min + prop->bbox.x_max) / 2.0, (prop->bbox.y_min + prop->bbox.y_max) / 2.0);
            if (dist <= distance_km &&
                prop->price <= max_price &&
                prop->area >= min_area &&
                prop->bedrooms >= min_bedrooms) {
                results.push_back(prop);
            }
        }

        return results;
    }

private:
    // Recursive function to perform the query
    void queryRecursive(RTreeNode* node, BoundingBox range, std::vector<Property*>& results) {
        if (!node->bounding_box.intersects(range)) return;

        if (node->is_leaf) {
            for (const auto& prop : node->leaf_properties) {
                if (range.intersects(prop->bbox)) {
                    results.push_back(prop);
                }
            }
        } else {
            for (const auto& child : node->children) {
                queryRecursive(child, range, results);
            }
        }
    }

    // Calculate the Euclidean distance between two points (latitude and longitude) in kilometers
    // Better approximation using Haversine or simple spherical equirectangular approximation
    double calculateDistance(double lon1, double lat1, double lon2, double lat2) {
        double R = 6371; // km
        double x = (lon2 - lon1) * std::cos((lat1 + lat2) / 2 * M_PI / 180.0);
        double y = (lat2 - lat1);
        double distance = std::sqrt(x * x + y * y) * R * M_PI / 180.0;
        return distance;
    }
};

void loadDataFromCSV(RTree& tree, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open the file: " << filename << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line); // Skip header

    int count = 0;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string location;
        std::string price_str, area_str, bedrooms_str, x_min_str, y_min_str, x_max_str, y_max_str;

        // format: location,price,area,bedrooms,x_min,y_min,x_max,y_max
        // Location has comma inside it (e.g., "123 Main St, New York"), so we need to handle quotes or just split correctly
        // Wait, Python's csv.writer handles quotes automatically. Let's parse CSV properly.
        
        // Simple manual CSV parse (handling quotes)
        bool in_quotes = false;
        std::vector<std::string> row;
        std::string current_val = "";
        for (char c : line) {
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (c == ',' && !in_quotes) {
                row.push_back(current_val);
                current_val = "";
            } else {
                current_val += c;
            }
        }
        row.push_back(current_val);

        if (row.size() < 8) continue;

        location = row[0];
        double price = std::stod(row[1]);
        double area = std::stod(row[2]);
        int bedrooms = std::stoi(row[3]);
        double x_min = std::stod(row[4]);
        double y_min = std::stod(row[5]);
        double x_max = std::stod(row[6]);
        double y_max = std::stod(row[7]);

        BoundingBox bbox(x_min, y_min, x_max, y_max);
        Property* prop = new Property(location, price, area, bedrooms, bbox);
        tree.insert(prop);
        count++;
    }
    file.close();
    std::cout << "Successfully loaded " << count << " properties into the R-Tree." << std::endl;
}

int main() {
    RTree tree;
    
    std::cout << "Loading data..." << std::endl;
    loadDataFromCSV(tree, "data/properties.csv");

    // Initialize Crow App with CORS middleware
    crow::App<crow::CORSHandler> app;

    // Setup CORS rules
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors
      .global()
        .headers("X-Custom-Header", "Upgrade-Insecure-Requests", "Content-Type")
        .methods("POST"_method, "GET"_method, "OPTIONS"_method)
        .origin("*");

    CROW_ROUTE(app, "/api/properties/search")
    ([&tree](const crow::request& req){
        double x = req.url_params.get("x") ? std::stod(req.url_params.get("x")) : -95.0; // lon
        double y = req.url_params.get("y") ? std::stod(req.url_params.get("y")) : 38.0; // lat
        double distance_km = req.url_params.get("distance_km") ? std::stod(req.url_params.get("distance_km")) : 100.0;
        double max_price = req.url_params.get("max_price") ? std::stod(req.url_params.get("max_price")) : 100000000.0;
        double min_area = req.url_params.get("min_area") ? std::stod(req.url_params.get("min_area")) : 0.0;
        int min_bedrooms = req.url_params.get("min_bedrooms") ? std::stoi(req.url_params.get("min_bedrooms")) : 0;

        auto results = tree.queryNearLocation(x, y, distance_km, max_price, min_area, min_bedrooms);
        
        std::vector<crow::json::wvalue> json_results;
        for (const auto& prop : results) {
            json_results.push_back(prop->toJson());
        }

        crow::json::wvalue final_result;
        final_result["properties"] = std::move(json_results);
        final_result["count"] = results.size();
        return final_result;
    });

    CROW_ROUTE(app, "/api/properties/range")
    ([&tree](const crow::request& req){
        double x_min = req.url_params.get("x_min") ? std::stod(req.url_params.get("x_min")) : -180.0;
        double y_min = req.url_params.get("y_min") ? std::stod(req.url_params.get("y_min")) : -90.0;
        double x_max = req.url_params.get("x_max") ? std::stod(req.url_params.get("x_max")) : 180.0;
        double y_max = req.url_params.get("y_max") ? std::stod(req.url_params.get("y_max")) : 90.0;

        BoundingBox query_range(x_min, y_min, x_max, y_max);
        auto results = tree.query(query_range);

        std::vector<crow::json::wvalue> json_results;
        // Limit results to 1000 to prevent huge JSON payloads for giant bounding boxes
        int limit = std::min(1000, (int)results.size());
        for (int i=0; i<limit; i++) {
            json_results.push_back(results[i]->toJson());
        }

        crow::json::wvalue final_result;
        final_result["properties"] = std::move(json_results);
        final_result["count"] = limit;
        final_result["total"] = results.size();
        return final_result;
    });

    // Run the app on port 8080
    app.port(8080).multithreaded().run();

    return 0;
}