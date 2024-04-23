#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

int main() {
    // Create a root tree
    pt::ptree root;

    // Add some data directly to the root
    root.put("name", "John Doe");
    root.put("age", 30);

    // Create a nested object for address
    pt::ptree address;
    address.put("city", "New York");
    address.put("zip", "10001");

    // Add the nested address object to the root
    root.add_child("address", address);

    // Create a nested array for hobbies
    pt::ptree hobbies;
    hobbies.push_back(std::make_pair("", pt::ptree()));
    hobbies.back().second.put_value(100);
    hobbies.push_back(std::make_pair("", pt::ptree()));
    hobbies.back().second.put_value(200);

    // Add the nested array of hobbies to the root
    root.add_child("hobbies", hobbies);

    // Convert the tree to a JSON string
    std::ostringstream oss;
    pt::write_json(oss, root);
    std::string json_string = oss.str();

    // Output the JSON string
    std::cout << json_string << std::endl;

    return 0;
}
