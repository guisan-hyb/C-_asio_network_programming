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
---

---

---



## 补充篇：nlohmann-json 进阶用法与生产环境注意事项

> 原文已经覆盖了创建、序列化、反序列化、取值、遍历、删除和 `R"()"` 的基础用法。
> 这一篇补充的是实际项目中最容易踩坑、也最值得提前建立习惯的部分，尤其是网络请求解析、类型转换、异常处理和本项目注册接口里遇到的问题。

---

### 9. 工程集成方式

**vcpkg + CMake 集成**

```bash
vcpkg install nlohmann-json
```

```cmake
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(chat_server PRIVATE
    nlohmann_json::nlohmann_json
)
```

`nlohmann/json` 是 header-only 库，链接的是接口目标，不会产生额外的 `.lib`，但版本管理交给 vcpkg 仍然很重要。

**头文件与前向声明**

在头文件里如果只需要引用 `json` 类型，不需要完整定义，可以用前向声明：

```cpp
#include <nlohmann/json_fwd.hpp>

void handle_request(const nlohmann::json& request);
```

在 `.cpp` 里再包含完整头文件：

```cpp
#include <nlohmann/json.hpp>
```

这样能减少编译时间。`nlohmann/json` 是模板很重的库，如果每个头文件都直接 `#include <nlohmann/json.hpp>`，项目一大编译时间会明显上升。

**编译选项**

```cpp
// 关闭隐式类型转换，强制使用 get<T>()、value() 等显式接口
#define JSON_USE_IMPLICIT_CONVERSIONS 0
```

关闭隐式转换后代码会更啰嗦，但能避免很多“看起来能编译、运行时抛异常”的问题。对网络服务这种要处理不可信输入的场景，推荐开启严格模式。

**ABI 注意事项**

不同版本的 nlohmann/json 使用不同的 ABI 命名空间，例如 `json_abi_v3_12_0`。不要把一个版本生成的 `nlohmann::json` 对象跨 DLL 边界传给另一个版本编译的模块，否则可能出现符号或行为不一致。服务端各个模块统一使用 vcpkg 的同一个版本即可。

---

### 10. 构造与初始化的常见坑

**空 json 不是对象，也不是数组**

```cpp
json j;          // null
json obj = json::object();
json arr = json::array();
```

只有当你给对象写键值时，它才会变成 object：

```cpp
json j;
j["name"] = "guisan";   // 现在 j 是 object
```

如果要明确表示数组，必须用 `json::array()`：

```cpp
json arr = json::array();
arr.push_back(1);
arr.push_back(2);
```

**初始化列表的歧义**

```cpp
json a = {1, 2, 3};                       // array
json b = {{"name", "guisan"}, {"age", 25}}; // object
```

嵌套结构容易让初始化列表变得难以阅读，涉及多层结构时建议逐步构造：

```cpp
json user;
user["name"] = "guisan";
user["age"] = 25;
user["skills"] = json::array({"C++", "Asio", "gRPC"});
```

**数字类型**

```cpp
json i = 1;     // integer
json d = 1.0;   // floating point
json n = 1e10;  // floating point
```

JSON 规范本身不区分整数和小数；nlohmann 内部会区分为 integer、unsigned 和 float。读取时优先使用明确的类型：

```cpp
int32_t pages = j.at("pages").get<int32_t>();
double price = j.at("price").get<double>();
```

不要对大整数使用 `int`，也不要用 `double` 存订单号、秒级时间戳这类需要精确的整数。

---

### 11. 安全取值：`[]`、`at`、`value`、`get`、`get_if`

这是实际项目里最重要的一节。不同写法对“字段不存在”和“类型不匹配”的行为不同：

| 写法 | 字段不存在 | 类型不匹配 | 建议 |
|---|---|---|---|
| `j["key"]` | 非 const 会插入 null | 取决于后续操作 | 只用于构造或修改 |
| `j.at("key")` | 抛 `out_of_range` | 返回引用，取 `get<T>` 时抛 `type_error` | 必填字段 |
| `j.value("key", def)` | 返回 `def` | 可能抛 `type_error` | 可选字段 |
| `j.get<T>()` | 当前节点类型不符会抛 `type_error` | 抛异常 | 确知类型时 |
| `j.get_if<T>(&ptr)` | 失败返回 false，不抛 | 失败返回 false | 不确定类型时 |

`contains` 和 `is_*` 负责判断：

```cpp
if (!j.contains("email")) {
    // 字段缺失
}

if (j.contains("email") && j["email"].is_string()) {
    std::string email = j["email"].get<std::string>();
}
```

**强烈建议不要这样写**

```cpp
std::string email = j["email"];  // 字段缺失或类型错误时都可能抛异常
int age = j["age"];              // 同理
```

**推荐的辅助函数**

```cpp
std::string get_string(const json& j, const char* key,
                       const std::string& def = "") {
    if (!j.contains(key)) {
        return def;
    }
    const auto& value = j.at(key);
    if (!value.is_string()) {
        return def;
    }
    return value.get<std::string>();
}

int get_int(const json& j, const char* key, int def = 0) {
    if (!j.contains(key)) {
        return def;
    }
    const auto& value = j.at(key);
    if (!value.is_number_integer()) {
        return def;
    }
    return value.get<int>();
}
```

**`value()` 的一个重要细节**

```cpp
std::string email = j.value("email", std::string(""));
```

这行代码在“字段不存在”时返回空字符串；但如果字段存在、类型却是数字或对象，`value()` 仍会尝试转换并可能抛 `type_error`。所以对不可信输入，最好先 `contains` + `is_string`，再 `get`。

---

### 12. `parse` 与异常处理

**解析失败会抛异常**

```cpp
try {
    json j = json::parse(body);
}
catch (const json::parse_error& e) {
    std::cerr << "parse error: " << e.what() << std::endl;
    std::cerr << "byte position: " << e.byte << std::endl;
}
```

`parse_error` 继承自 `json::exception`，通常还会带 `id`、`byte` 等定位信息。网络服务里不要直接让异常逃出去，否则 HTTP handler 可能在没有写回响应的情况下结束，客户端就会看到“空响应”或“网络请求错误”。

**不抛异常的解析方式**

```cpp
json j = json::parse(body, nullptr, false);
if (j.is_discarded()) {
    // 解析失败
}
```

第三个参数 `allow_exceptions = false` 时，解析失败返回 discarded value，可以用 `is_discarded()` 判断。适合希望在错误处理上完全自己控制的场景。

**语法限制**

默认解析器只接受严格 JSON：

- 不接受注释。
- 不接受尾随逗号。
- 字符串必须使用双引号。
- 必须使用 UTF-8 编码。

较新版本提供 `ignore_comments` 之类的扩展选项，但不要把“解析宽松 JSON”当成正常业务逻辑，服务端协议应当保持严格。

**从流读取**

```cpp
std::ifstream file("config.json");
if (!file.is_open()) {
    // 处理文件不存在
}

json config;
file >> config;   // 失败会抛 parse_error
```

文件输入同样要检查打开结果和异常。

---

### 13. `dump` 与序列化细节

```cpp
std::string compact = j.dump();
std::string pretty  = j.dump(4);
```

`dump` 的常用参数：

```cpp
j.dump(4, ' ', false);
```

- 第一个参数是缩进；`-1` 表示紧凑输出。
- 第二个参数是缩进字符，默认空格。
- 第三个参数是 `ensure_ascii`，默认 `false`，即默认直接输出 UTF-8 中文，而不是 `\uXXXX`。

如果输入包含非法 UTF-8，可以用错误处理器：

```cpp
j.dump(-1, ' ', false, json::error_handler_t::replace);
```

**对象顺序**

默认 `nlohmann::json` 的对象使用有序 map，键会按字典序输出。如果你需要保持插入顺序，使用 `ordered_json`：

```cpp
#include <nlohmann/json.hpp>

nlohmann::ordered_json j;
j["z"] = 1;
j["a"] = 2;   // 输出依然是 z 在前
```

**浮点特殊值**

不要依赖 `NaN`、`Infinity` 的 JSON 表示。标准 JSON 没有这些值，`dump` 时通常会输出 `null`。业务层应在写入前就把这些值处理掉。

---

### 14. 类型转换与自定义类型

**自定义结构体**

```cpp
struct User {
    std::string name;
    int age = 0;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(User, name, age)
```

之后可以：

```cpp
User user = j.get<User>();
json out = user;
```

也可以手写 ADL 形式的 `to_json` / `from_json`，适合需要字段改名或做默认值处理的场景。

**枚举**

```cpp
enum class Color { Red, Green, Blue };

NLOHMANN_JSON_SERIALIZE_ENUM(Color, {
    {Color::Red, "red"},
    {Color::Green, "green"},
    {Color::Blue, "blue"},
})
```

**STL 容器**

```cpp
std::vector<int> nums = j.at("nums").get<std::vector<int>>();
std::map<std::string, int> scores = j.at("scores").get<std::map<std::string, int>>();
std::optional<std::string> nick = j.value("nick", std::optional<std::string>{});
```

**二进制数据不要直接塞 JSON**

Protobuf 的二进制序列化结果不适合直接放进 JSON 字符串。常见做法是 Base64：

```cpp
std::string bytes = message.SerializeAsString();
std::string base64 = Base64Encode(bytes);
j["payload"] = base64;
```

如果协议两端都是 C++，也可以考虑 CBOR、MessagePack 等二进制格式，nlohmann 提供了相应的转换接口。

---

### 15. 数组、对象、迭代与修改

**对象遍历必须用 `items()`**

```cpp
for (auto& [key, value] : j.items()) {
    std::cout << key << " = " << value << std::endl;
}
```

如果直接 `for (auto& v : j)` 遍历对象，拿到的是 value，不包含 key。

**数组遍历**

```cpp
for (const auto& item : j.at("skills")) {
    std::cout << item.get<std::string>() << std::endl;
}
```

**删除时注意迭代器失效**

```cpp
for (auto it = j.begin(); it != j.end(); ) {
    if (should_remove(*it)) {
        it = j.erase(it);   // erase 返回下一个迭代器
    }
    else {
        ++it;
    }
}
```

**JSON Pointer 与 Patch**

```cpp
using json_pointer = nlohmann::json::json_pointer;

auto value = j["/user/name"_json_pointer];
j["/user/age"_json_pointer] = 26;
```

`patch`、`diff`、`merge_patch` 可以用于配置合并和版本差异计算，但要注意它们遵循的是 JSON Patch / Merge Patch 规范，字段语义和普通赋值不同。

---

### 16. 性能与线程安全

**编译期成本**

`nlohmann/json` 是重量级模板库。工程变大后常见优化手段：

- 头文件里用 `json_fwd.hpp`，只在 `.cpp` 里包含 `json.hpp`。
- 用 PIMPL 隐藏 JSON 相关实现。
- 使用预编译头。
- 不要为了一个 bool 字段在公共头文件里引入整个 JSON 库。

**运行期成本**

- 参数使用 `const json&`，避免拷贝。
- 需要转移所有权时用 `std::move`。
- `dump()` 会分配字符串；不要在循环里反复序列化同一个对象。
- 超大 JSON 可以优先考虑 SAX 解析，而不是一次性构造完整 DOM。
- `ordered_json` 保持顺序，但内存和查找开销通常高于普通 `json`。

**线程安全**

不同的 `json` 对象可以在不同线程中独立使用；但同一个对象不要在多线程里一边读一边写。需要共享时，用 `std::mutex` 保护，或者复制一份给只读线程。

**异常开关**

可以定义 `JSON_NOEXCEPTION` 让库在部分错误场景不抛异常，但代价是错误处理要完全手工做，而且一些接口行为会变化。业务代码保持默认异常模式并正确捕获，通常更清晰。

---

### 17. 与本项目强相关的实战坑

这一节对应我们实际在注册接口里遇到的问题，建议重点记住。

**不要拿 `std::string` 和 `json` 直接做加法或混合比较**

错误写法：

```cpp
bool ok = RedisMgr::GetInst()->Get(CODEPREFIX + src_root["email"], verify_code);
```

`src_root["email"]` 的类型是 `json`，不是 `std::string`。它可能隐式转换，也可能在某些编译器上表现出不同的重载选择，最终在运行时抛类型异常。正确写法是先取字符串：

```cpp
std::string email = src_root.value("email", std::string(""));
std::string email_key = std::string(CODEPREFIX) + email;
bool ok = RedisMgr::GetInst()->Get(email_key, verify_code);
```

**不要混用请求对象和响应对象**

错误写法：

```cpp
json root;       // 响应
json src_root;   // 请求

if (verify_code != root["verifycode"]) { ... }
```

`root` 此时还没有 `verifycode`，访问它会得到一个 null，比较结果不符合预期。正确写法：

```cpp
if (verify_code != src_root.value("verifycode", std::string(""))) { ... }
```

**Handler 的所有分支都要写回响应**

如果解析、Redis 查询或数据库操作抛异常，而异常没有被处理，HTTP connection 可能在没写响应的情况下结束，客户端 Qt 会收到空响应，显示“网络请求错误”。推荐的结构：

```cpp
RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
    auto body_str = beast::buffers_to_string(connection->_request.body().data());
    connection->_response.set(http::field::content_type, "text/json");

    json root;

    try {
        json src_root = json::parse(body_str);

        if (!src_root.is_object()) {
            root["error"] = ErrorCodes::Error_Json;
        }
        else {
            std::string email = src_root.value("email", std::string(""));
            std::string code = src_root.value("verifycode", std::string(""));

            std::string redis_key = std::string(CODEPREFIX) + email;
            std::string cached_code;
            if (!RedisMgr::GetInst()->Get(redis_key, cached_code)) {
                root["error"] = ErrorCodes::Verify_Expired;
            }
            else if (cached_code != code) {
                root["error"] = ErrorCodes::Verify_Code_Error;
            }
            else {
                root["error"] = ErrorCodes::Success;
            }
        }
    }
    catch (const json::parse_error& e) {
        std::cerr << "parse error: " << e.what() << std::endl;
        root["error"] = ErrorCodes::Error_Json;
    }
    catch (const std::exception& e) {
        std::cerr << "register error: " << e.what() << std::endl;
        root["error"] = ErrorCodes::Error_Json;
    }

    std::string jsonstr = root.dump();
    beast::ostream(connection->_response.body()) << jsonstr;
    return true;
});
```

**必填字段校验清单**

对每个来自客户端的 JSON，至少检查：

- 是否解析成功。
- 顶层是否是 object。
- 必填字段是否存在。
- 字段类型是否正确。
- 字段取值范围是否合法。
- 字符串是否为空、是否超长。

不要直接把客户端的 JSON 字段当成可信的 `std::string` / `int` 使用。

**不要在响应里回显密码**

注册接口当前把 `passwd` 原样放回了响应。即使是开发阶段，也建议尽快去掉；生产环境更不能返回密码、token、验证码等敏感字段。

---

### 18. `R"()"` 原始字符串补充

原文已经介绍了 `R"(...)"`，这里补充几个容易忽略的细节。

**开头的换行**

```cpp
std::string s = R"(
{
    "name": "guisan"
}
)";
```

字符串实际以一个换行符开头，以一个换行符结尾。JSON 解析器会把这些空白当合法空白，所以没问题；但如果用于比较字符串或作为精确文本，就要注意。

**分隔符规则**

内容里如果出现 `)"`，必须使用自定义分隔符：

```cpp
std::string s = R"json(
这段内容里有 )" 字符，不会被提前结束。
)json";
```

分隔符最多 16 个字符，不能包含空格、括号、反斜杠和控制字符。

**编码前缀**

```cpp
u8R"({"name":"中文"})"   // UTF-8
LR"(wide raw string)"
uR"(char16_t raw string)"
UR"(char32_t raw string)"
```

**原始字符串不处理转义**

```cpp
std::string path = R"(C:\vcpkg\installed\x64-windows)";
```

反斜杠会原样保留，这正是它在 Windows 路径和正则表达式里好用的原因。

但要注意：原始字符串不能解决 JSON 本身的语法问题。即使写成：

```cpp
std::string bad = R"({"a": 1,})";
```

尾随逗号仍然是非法 JSON，解析时一样会抛 `parse_error`。

---

### 19. 常见异常与排查对照

| 异常信息 | 常见原因 | 处理方式 |
|---|---|---|
| `parse_error.101` | JSON 语法错误 | 打印 `e.byte`，检查请求体 |
| `type_error.302 type must be string, but is null` | 字段缺失或值为 null | `contains` + `is_string` 检查 |
| `out_of_range.403 key 'x' not found` | `at()` 访问不存在的键 | 改用 `contains` / `value` |
| `type_error.305 cannot use operator[] with ...` | 对数组用了字符串下标 | 确认 object/array 类型 |
| `type_error.306 cannot use value() with ...` | 对非 object 调用 `value` | 先判断 `is_object()` |

---

### 20. 速查总结

```cpp
// 解析
json j = json::parse(body);                 // 可能抛 parse_error
json j = json::parse(body, nullptr, false); // 失败返回 discarded

// 判断
j.is_object();
j.is_array();
j.contains("email");
j["email"].is_string();

// 读取
j.at("email").get<std::string>();           // 必填，失败抛异常
j.value("email", std::string(""));          // 可选，缺失用默认值
j.value("age", 0);                          // 注意类型不匹配仍可能抛异常

// 安全读取
if (j.contains("email") && j["email"].is_string()) {
    std::string email = j["email"].get<std::string>();
}

// 序列化
std::string s = j.dump();      // 紧凑
std::string p = j.dump(4);     // 缩进 4 空格

// 顺序
nlohmann::ordered_json oj;

// 自定义类型
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(User, name, age)
```

最后用一句话概括：**nlohmann-json 用起来像 STL，但它处理的是不可信的外部输入。凡是来自网络、文件、数据库的 JSON，都要先判断结构和类型，再取值。**
