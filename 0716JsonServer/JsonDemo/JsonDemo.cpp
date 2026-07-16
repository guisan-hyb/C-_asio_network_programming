#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    json book;
    book["name"] = "C++ Primer";
    book["pages"] = 800;
    book["price"] = 99.5;
    book["authors"] = { "Stanley B. Lippman", "Josée Lajoie" };

    // 序列化为字符串
    std::string json_str = book.dump();
    std::cout << "JSON String: " << json_str << std::endl;

    // 反序列化（从字符串解析）
    std::string input_str = R"({"user": "guisan","age": 25,"skills": ["C++", "Protobuf", "vcpkg"]})";

    json parsed_data = json::parse(input_str);

    std::cout << "User name: " << parsed_data["user"] << std::endl;
    std::cout << "Age: " << parsed_data["age"] << std::endl;
    for (auto& skill : parsed_data["skills"]) {
        std::cout << "Skill: " << skill << " ";
    }
    std::cout << std::endl;
    

    return 0;
}
