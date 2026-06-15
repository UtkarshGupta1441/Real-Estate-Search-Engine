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
#include <chrono>
#include <utility>

using namespace std;

// ============================================================================
// R-Tree Configuration
// ============================================================================
static const int MAX_NODE_ENTRIES = 64;   // Max children per node / entries per leaf
static const int DEFAULT_PER_PAGE = 50;   // Default results per page
static const int MAX_PER_PAGE = 200;      // Hard cap on results per page

// ============================================================================
// Spatial Data Structures
// ============================================================================

// Represents a bounding box or spatial region
struct BoundingBox {
    double x_min, y_min, x_max, y_max;

    BoundingBox(double x1 = 0, double y1 = 0, double x2 = 0, double y2 = 0)
        : x_min(x1), y_min(y1), x_max(x2), y_max(y2) {}

    bool intersects(const BoundingBox& other) const {
        return !(x_min > other.x_max || x_max < other.x_min ||
                 y_min > other.y_max || y_max < other.y_min);
    }

    double centerX() const { return (x_min + x_max) / 2.0; }
    double centerY() const { return (y_min + y_max) / 2.0; }
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

    crow::json::wvalue toJsonWithDistance(double dist_km) const {
        auto json_obj = toJson();
        json_obj["distance_km"] = std::round(dist_km * 100.0) / 100.0;
        return json_obj;
    }
};

// ============================================================================
// R-Tree Implementation — Sort-Tile-Recursive (STR) Bulk Loading
// ============================================================================

class RTreeNode {
public:
    BoundingBox bbox;
    bool is_leaf;
    std::vector<RTreeNode*> children;   // Internal node children
    std::vector<Property*> entries;      // Leaf node entries

    RTreeNode() : bbox(), is_leaf(true) {}

    // Recompute this node's bounding box from its contents
    void computeBoundingBox() {
        if (is_leaf) {
            if (entries.empty()) return;
            double xn = entries[0]->bbox.x_min, yn = entries[0]->bbox.y_min;
            double xx = entries[0]->bbox.x_max, yx = entries[0]->bbox.y_max;
            for (size_t i = 1; i < entries.size(); i++) {
                xn = std::min(xn, entries[i]->bbox.x_min);
                yn = std::min(yn, entries[i]->bbox.y_min);
                xx = std::max(xx, entries[i]->bbox.x_max);
                yx = std::max(yx, entries[i]->bbox.y_max);
            }
            bbox = BoundingBox(xn, yn, xx, yx);
        } else {
            if (children.empty()) return;
            double xn = children[0]->bbox.x_min, yn = children[0]->bbox.y_min;
            double xx = children[0]->bbox.x_max, yx = children[0]->bbox.y_max;
            for (size_t i = 1; i < children.size(); i++) {
                xn = std::min(xn, children[i]->bbox.x_min);
                yn = std::min(yn, children[i]->bbox.y_min);
                xx = std::max(xx, children[i]->bbox.x_max);
                yx = std::max(yx, children[i]->bbox.y_max);
            }
            bbox = BoundingBox(xn, yn, xx, yx);
        }
    }
};

class RTree {
    RTreeNode* root;

public:
    RTree() : root(nullptr) {}

    // Build an optimally packed R-Tree using Sort-Tile-Recursive (STR) bulk loading.
    // All properties must be loaded into a vector first, then passed here once.
    void bulkLoad(std::vector<Property*>& properties) {
        if (properties.empty()) {
            root = new RTreeNode();
            root->bbox = BoundingBox(-180, -90, 180, 90);
            return;
        }
        root = buildSTR(properties, 0, (int)properties.size(), 0);
        std::cout << "R-Tree stats: depth=" << getDepth()
                  << ", nodes=" << getNodeCount()
                  << ", entries=" << properties.size() << std::endl;
    }

    // Spatial range query: find all properties whose bbox intersects the given range
    std::vector<Property*> query(const BoundingBox& range) const {
        std::vector<Property*> results;
        if (root) queryRecursive(root, range, results);
        return results;
    }

    // Proximity query: find properties near (lon, lat) within distance_km, with filters.
    // Returns (property, distance_km) pairs sorted by distance ascending.
    std::vector<std::pair<Property*, double>> queryNearLocation(
        double lon, double lat, double distance_km,
        double max_price, double min_area, int min_bedrooms) const
    {
        // Convert km to approximate degrees for bounding box pre-filter
        double approx_degrees = distance_km / 111.0;
        BoundingBox search_area(
            lon - approx_degrees, lat - approx_degrees,
            lon + approx_degrees, lat + approx_degrees
        );

        auto candidates = query(search_area);

        std::vector<std::pair<Property*, double>> results;
        results.reserve(candidates.size());

        for (auto* prop : candidates) {
            double dist = calculateDistance(lon, lat, prop->bbox.centerX(), prop->bbox.centerY());
            if (dist <= distance_km &&
                prop->price <= max_price &&
                prop->area >= min_area &&
                prop->bedrooms >= min_bedrooms) {
                results.emplace_back(prop, dist);
            }
        }

        // Sort by distance (nearest first)
        std::sort(results.begin(), results.end(),
            [](const std::pair<Property*, double>& a, const std::pair<Property*, double>& b) {
                return a.second < b.second;
            });

        return results;
    }

    int getDepth() const { return root ? depthOf(root) : 0; }
    int getNodeCount() const { return root ? countNodes(root) : 0; }

private:
    // STR bulk-loading: recursively sort by alternating axis and partition into groups
    RTreeNode* buildSTR(std::vector<Property*>& props, int start, int end, int depth) {
        int count = end - start;

        // Base case: fits in a single leaf node
        if (count <= MAX_NODE_ENTRIES) {
            auto* node = new RTreeNode();
            node->is_leaf = true;
            node->entries.assign(props.begin() + start, props.begin() + end);
            node->computeBoundingBox();
            return node;
        }

        // Alternate sort axis: X at even depths, Y at odd depths
        if (depth % 2 == 0) {
            std::sort(props.begin() + start, props.begin() + end,
                [](const Property* a, const Property* b) {
                    return a->bbox.centerX() < b->bbox.centerX();
                });
        } else {
            std::sort(props.begin() + start, props.begin() + end,
                [](const Property* a, const Property* b) {
                    return a->bbox.centerY() < b->bbox.centerY();
                });
        }

        // Determine number of child groups (cap at MAX_NODE_ENTRIES fan-out)
        int num_groups = std::min(MAX_NODE_ENTRIES,
                                  (int)std::ceil((double)count / MAX_NODE_ENTRIES));
        int group_size = (int)std::ceil((double)count / num_groups);

        auto* node = new RTreeNode();
        node->is_leaf = false;

        for (int i = 0; i < num_groups; i++) {
            int child_start = start + i * group_size;
            int child_end = std::min(start + (i + 1) * group_size, end);
            if (child_start >= end) break;

            auto* child = buildSTR(props, child_start, child_end, depth + 1);
            node->children.push_back(child);
        }

        node->computeBoundingBox();
        return node;
    }

    // Recursive spatial intersection search
    void queryRecursive(RTreeNode* node, const BoundingBox& range,
                        std::vector<Property*>& results) const {
        if (!node->bbox.intersects(range)) return;

        if (node->is_leaf) {
            for (auto* prop : node->entries) {
                if (range.intersects(prop->bbox)) {
                    results.push_back(prop);
                }
            }
        } else {
            for (auto* child : node->children) {
                queryRecursive(child, range, results);
            }
        }
    }

    // Equirectangular approximation — accurate enough for property search radii
    static double calculateDistance(double lon1, double lat1, double lon2, double lat2) {
        const double R = 6371.0; // Earth radius in km
        double x = (lon2 - lon1) * std::cos((lat1 + lat2) / 2.0 * M_PI / 180.0);
        double y = (lat2 - lat1);
        return std::sqrt(x * x + y * y) * R * M_PI / 180.0;
    }

    static int depthOf(RTreeNode* node) {
        if (node->is_leaf) return 1;
        int d = 0;
        for (auto* c : node->children) d = std::max(d, depthOf(c));
        return 1 + d;
    }

    static int countNodes(RTreeNode* node) {
        int c = 1;
        if (!node->is_leaf)
            for (auto* ch : node->children) c += countNodes(ch);
        return c;
    }
};

// ============================================================================
// CSV Data Loader
// ============================================================================

std::vector<Property*> loadDataFromCSV(const std::string& filename) {
    std::vector<Property*> properties;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open the file: " << filename << std::endl;
        return properties;
    }

    std::string line;
    std::getline(file, line); // Skip header

    while (std::getline(file, line)) {
        // Simple manual CSV parse (handling quoted fields)
        bool in_quotes = false;
        std::vector<std::string> row;
        std::string current_val;
        for (char c : line) {
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (c == ',' && !in_quotes) {
                row.push_back(current_val);
                current_val.clear();
            } else {
                current_val += c;
            }
        }
        row.push_back(current_val);

        if (row.size() < 8) continue;

        std::string location = row[0];
        double price = std::stod(row[1]);
        double area = std::stod(row[2]);
        int bedrooms = std::stoi(row[3]);
        double x_min = std::stod(row[4]);
        double y_min = std::stod(row[5]);
        double x_max = std::stod(row[6]);
        double y_max = std::stod(row[7]);

        BoundingBox bbox(x_min, y_min, x_max, y_max);
        properties.push_back(new Property(location, price, area, bedrooms, bbox));
    }
    file.close();
    std::cout << "Loaded " << properties.size() << " properties from CSV." << std::endl;
    return properties;
}

// ============================================================================
// Main Application
// ============================================================================

int main() {
    auto app_start = std::chrono::high_resolution_clock::now();

    // ---- Load data and build spatial index ----
    std::cout << "Loading data..." << std::endl;
    auto properties = loadDataFromCSV("data/properties.csv");

    std::cout << "Building R-Tree index..." << std::endl;
    RTree tree;
    tree.bulkLoad(properties);

    auto app_ready = std::chrono::high_resolution_clock::now();
    double startup_ms = std::chrono::duration<double, std::milli>(app_ready - app_start).count();
    std::cout << "Server ready in " << (int)startup_ms << "ms" << std::endl;

    // ---- Initialize Crow App with CORS middleware ----
    crow::App<crow::CORSHandler> app;

    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors
      .global()
        .headers("X-Custom-Header", "Upgrade-Insecure-Requests", "Content-Type")
        .methods("POST"_method, "GET"_method, "OPTIONS"_method)
        .origin("*");

    // ================================================================
    // GET /api/properties/search — Proximity search (paginated)
    // ================================================================
    CROW_ROUTE(app, "/api/properties/search")
    ([&tree](const crow::request& req){
        auto t0 = std::chrono::high_resolution_clock::now();

        // Parse query parameters with defaults
        double x = req.url_params.get("x") ? std::stod(req.url_params.get("x")) : -95.0;
        double y = req.url_params.get("y") ? std::stod(req.url_params.get("y")) : 38.0;
        double distance_km = req.url_params.get("distance_km") ? std::stod(req.url_params.get("distance_km")) : 100.0;
        double max_price = req.url_params.get("max_price") ? std::stod(req.url_params.get("max_price")) : 100000000.0;
        double min_area = req.url_params.get("min_area") ? std::stod(req.url_params.get("min_area")) : 0.0;
        int min_bedrooms = req.url_params.get("min_bedrooms") ? std::stoi(req.url_params.get("min_bedrooms")) : 0;
        int page = req.url_params.get("page") ? std::stoi(req.url_params.get("page")) : 1;
        int per_page = req.url_params.get("per_page") ? std::stoi(req.url_params.get("per_page")) : DEFAULT_PER_PAGE;

        // Clamp pagination params
        page = std::max(1, page);
        per_page = std::max(1, std::min(per_page, MAX_PER_PAGE));

        // Execute spatial query (results are sorted by distance)
        auto all_results = tree.queryNearLocation(x, y, distance_km, max_price, min_area, min_bedrooms);

        int total = (int)all_results.size();
        int offset = (page - 1) * per_page;
        if (offset > total) offset = total;
        int end_idx = std::min(offset + per_page, total);
        bool has_more = end_idx < total;
        int count = end_idx - offset;

        // Serialize the current page of results
        std::vector<crow::json::wvalue> json_results;
        json_results.reserve(count);
        for (int i = offset; i < end_idx; i++) {
            json_results.push_back(all_results[i].first->toJsonWithDistance(all_results[i].second));
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        double query_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        std::cout << "SEARCH: (" << x << "," << y << ") r=" << distance_km
                  << "km -> " << total << " hits in " << query_ms << "ms"
                  << " [page " << page << ", showing " << count << "]" << std::endl;

        crow::json::wvalue final_result;
        final_result["count"] = count;
        final_result["total"] = total;
        final_result["page"] = page;
        final_result["per_page"] = per_page;
        final_result["has_more"] = has_more;
        final_result["query_time_ms"] = std::round(query_ms * 100.0) / 100.0;
        final_result["properties"] = std::move(json_results);
        return final_result;
    });

    // ================================================================
    // GET /api/properties/range — Bounding box range query (paginated)
    // ================================================================
    CROW_ROUTE(app, "/api/properties/range")
    ([&tree](const crow::request& req){
        auto t0 = std::chrono::high_resolution_clock::now();

        double x_min = req.url_params.get("x_min") ? std::stod(req.url_params.get("x_min")) : -180.0;
        double y_min = req.url_params.get("y_min") ? std::stod(req.url_params.get("y_min")) : -90.0;
        double x_max = req.url_params.get("x_max") ? std::stod(req.url_params.get("x_max")) : 180.0;
        double y_max = req.url_params.get("y_max") ? std::stod(req.url_params.get("y_max")) : 90.0;
        int page = req.url_params.get("page") ? std::stoi(req.url_params.get("page")) : 1;
        int per_page = req.url_params.get("per_page") ? std::stoi(req.url_params.get("per_page")) : DEFAULT_PER_PAGE;

        page = std::max(1, page);
        per_page = std::max(1, std::min(per_page, MAX_PER_PAGE));

        BoundingBox query_range(x_min, y_min, x_max, y_max);
        auto all_results = tree.query(query_range);

        int total = (int)all_results.size();
        int offset = (page - 1) * per_page;
        if (offset > total) offset = total;
        int end_idx = std::min(offset + per_page, total);
        bool has_more = end_idx < total;
        int count = end_idx - offset;

        std::vector<crow::json::wvalue> json_results;
        json_results.reserve(count);
        for (int i = offset; i < end_idx; i++) {
            json_results.push_back(all_results[i]->toJson());
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        double query_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        std::cout << "RANGE: [" << x_min << "," << y_min << " -> " << x_max << "," << y_max
                  << "] -> " << total << " hits in " << query_ms << "ms" << std::endl;

        crow::json::wvalue final_result;
        final_result["count"] = count;
        final_result["total"] = total;
        final_result["page"] = page;
        final_result["per_page"] = per_page;
        final_result["has_more"] = has_more;
        final_result["query_time_ms"] = std::round(query_ms * 100.0) / 100.0;
        final_result["properties"] = std::move(json_results);
        return final_result;
    });

    // Run the app on port 8080
    app.port(8080).multithreaded().run();

    return 0;
}