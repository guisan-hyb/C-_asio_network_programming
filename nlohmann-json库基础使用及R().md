### nlohmann-json库基础使用及R"()"

`nlohmann/json` 是 C++ 生态中最受欢迎的 JSON 库之一。它的设计理念是 **“像操作 STL 容器一样操作 JSON”**，这意味着你可以用非常直观的 C++ 语法来创建、修改和解析 JSON 数据。

以下是 `nlohmann/json` 的核心使用指南：

### 0. 基础准备
在代码中包含头文件，并起个别名：
```cpp
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
```

---

### 1. 创建 JSON 对象
你可以像给 `std::map` 或 `std::vector` 赋值一样创建 JSON。

```cpp
// 方式一：逐个赋值（最直观）
json book;
book["title"] = "C++ Primer";
book["pages"] = 800;
book["price"] = 99.5;
book["available"] = true;

// 添加数组
book["authors"] = {"Stanley B. Lippman", "Josée Lajoie"};

// 添加嵌套对象
book["publisher"]["name"] = "Addison-Wesley";
book["publisher"]["year"] = 2012;

// 方式二：使用初始化列表（一次性构建，适合写死的数据）
json j2 = {
    {"name", "Effective C++"},
    {"pages", 350},
    {"tags", {"C++", "Best Practices", "Programming"}}
};
```

---

### 2. 序列化（转成字符串）
将 JSON 对象转为字符串，使用 `.dump()` 方法。

```cpp
// 1. 紧凑模式（无空格，适合网络传输）
std::string compact_str = book.dump();
// 输出: {"available":true,"pages":800,...}

// 2. 美化模式（带缩进，适合保存到文件或打印日志）
// 参数 4 表示缩进 4 个空格
std::string pretty_str = book.dump(4);
std::cout << pretty_str << std::endl;
```

---

### 3. 反序列化（从字符串解析）
将 JSON 字符串解析为 JSON 对象，使用 `json::parse()`。

```cpp
std::string json_string = R"(
    {
        "user": "guisan",
        "age": 25,
        "skills": ["C++", "Protobuf", "vcpkg"]
    }
)";

// 解析字符串
json parsed_data = json::parse(json_string);

// 输出解析结果
std::cout << "User: " << parsed_data["user"] << std::endl;
```

---

### 4. 读取与类型转换
读取值时，`nlohmann/json` 会自动推断类型，或者你可以使用 `.get<T>()` 进行显式转换。

```cpp
// 自动推断类型（直接赋值给对应类型的变量）
std::string user = parsed_data["user"]; // 字符串
int age = parsed_data["age"];           // 整数

// 显式获取类型（推荐，更安全）
int age_safe = parsed_data["age"].get<int>();

// 读取数组中的元素
std::string first_skill = parsed_data["skills"][0];
std::cout << "First skill: " << first_skill << std::endl; // 输出: C++
```

---

### 5. 检查字段是否存在与类型判断
在实际开发中，从网络收到的 JSON 可能缺失某些字段，盲目读取会导致程序崩溃。使用 `.contains()` 检查字段是否存在。

```cpp
if (parsed_data.contains("email")) {
    std::string email = parsed_data["email"];
} else {
    std::cout << "email 字段不存在" << std::endl;
}

// 类型判断
if (parsed_data["age"].is_number()) {
    std::cout << "age is a number!" << std::endl;
}
if (parsed_data["skills"].is_array()) {
    std::cout << "skills is an array!" << std::endl;
}
```

---

### 6. 遍历 JSON
你可以像遍历 STL 容器一样遍历 JSON 对象或数组。

```cpp
// 遍历对象（获取键值对）
for (auto& el : parsed_data.items()) {
    std::cout << "Key: " << el.key() << ", Value: " << el.value() << std::endl;
}

// 遍历数组
for (auto& skill : parsed_data["skills"]) {
    std::cout << "Skill: " << skill << std::endl;
}
```

---

### 7. 删除和修改元素
```cpp
// 修改元素
parsed_data["age"] = 26;

// 删除元素
parsed_data.erase("user");
```

---

### 8. 结合 Protobuf 的实战场景
在实际项目中，经常需要把 Protobuf 序列化后的二进制数据，转成 Base64 字符串，再放进 JSON 里传输；或者用 JSON 作为配置文件来读取 Protobuf 的字段。

**示例：将配置文件读到 Protobuf 对象中**
```cpp
#define NOMINMAX
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include "msg.pb.h" // 你的 protobuf 头文件

using json = nlohmann::json;

int main() {
    // 假设 config.json 内容为: {"name": "Json Book", "pages": 200, "price": 45.0}
    std::ifstream file("config.json");
    json config_json;
    file >> config_json;

    // 将 JSON 数据映射到 Protobuf 对象
    Book book;
    book.set_name(config_json["name"].get<std::string>());
    book.set_pages(config_json["pages"].get<int32_t>());
    book.set_price(config_json["price"].get<float>());

    std::cout << "Protobuf Book Name: " << book.name() << std::endl;
    return 0;
}
```

### 总结
- 创建/修改：像 `map` 一样用 `[]`。
- 转字符串：`.dump(4)`。
- 解析字符串：`json::parse()`。
- 取值：`[]` 或 `.get<T>()`。
- 判断存在：`.contains("key")`。

`nlohmann/json` 非常强大且符合直觉，掌握了以上这些，日常 90% 以上的 JSON 处理需求都能轻松应对！



### 解析R"()"

这是一个非常实用的 C++11 语法，叫作 **原始字符串字面量**。

简单来说，它的作用就是：**让字符串里的特殊字符（比如双引号 `"`、反斜杠 `\`、换行符）不再需要转义，所见即所得。**

### 1. 为什么需要它？（痛点在哪）

在以前，如果你想在 C++ 里写一段带引号的 JSON 字符串，你必须用反斜杠 `\` 来转义内部的双引号，代码会变得非常难读：
```cpp
// 传统的写法：反斜杠满天飞，极易写错
std::string json_str = "{\"name\": \"guisan\", \"age\": 25}";
```

### 2. `R"(...)"` 怎么解决的？

加上 `R` 前缀后，编译器会原封不动地读取括号 `()` 里面的内容，**遇到双引号也不认为它是字符串的结束，遇到反斜杠也不当转义符处理**。

```cpp
// 使用 R"(...)" 的写法：清爽，所见即所得
std::string json_str = R"({"name": "guisan", "age": 25})";
```

### 3. 语法拆解
以 `R"(<内容>)"` 为例：
* `R`：代表 Raw（原始）。
* `"`：字符串的开始标志。
* `(`：**原始字符串的真正开始边界**。
* `<内容>`：你想写的任何字符，不用加转义符。
* `)`：**原始字符串的真正结束边界**。
* `"`：字符串的结束标志。

### 4. 另一个神仙用法：多行字符串
在传统 C++ 中，如果你想要一个多行字符串，必须用 `\n` 换行符，或者把多个字符串拼接起来：
```cpp
// 传统多行写法
std::string text = "Line 1\n"
                   "Line 2\n"
                   "Line 3";
```

但用 `R"(...)"`，你可以直接在代码里敲回车换行，换行符会被自动保留：
```cpp
std::string text = R"(
Line 1
Line 2
Line 3
)";
```
这在写 SQL 语句、HTML 片段、或者大段配置文本时简直是神器！

### 5. 进阶用法：解决“内容里包含 `)`”的冲突
万一你的字符串内容里刚好包含 `)"` 这个组合怎么办？比如 `R"("Hello")"`，编译器一看到 `)"` 就会以为字符串结束了。

为了解决这个问题，C++ 允许你在 `"` 和 `(` 之间加上一个**自定义的分隔符**（比如叫 `json`），然后在结尾的 `)` 和 `"` 之间也加上相同的分隔符：

```cpp
// 分隔符是 json
std::string str = R"json(
    这是一个包含 )" 符号的字符串，编译器不会在这里结束，会一直读到下面的 json)" 为止。
)json";
```

### 总结
在写 JSON、正则表达式、Windows 文件路径（如 `C:\vcpkg\...`）时，加上 `R"(...)"` 会极大地提升代码的可读性，彻底告别反斜杠地狱！