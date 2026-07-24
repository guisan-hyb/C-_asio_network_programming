### http相关知识补充

结合你提供的 C++ HTTP 服务器代码，我来为你系统地补充 HTTP（超文本传输协议）的相关核心知识。理解这些知识，能让你明白代码中那些类、方法、字段背后的深层逻辑。

HTTP 是一个**应用层协议**，它建立在 TCP（传输控制协议）之上，规定了客户端（如浏览器）与服务器之间如何进行数据交换。

---

### 1. HTTP 的工作模型：请求-响应

HTTP 采用的是 **客户端请求 -> 服务器响应** 的模型。
*   **客户端**：主动发起连接并发送请求的一方（如你的浏览器、curl 命令、Postman）。
*   **服务器**：被动监听端口，接收请求并返回响应的一方（就是你这段 C++ 代码运行的程序）。

**映射到代码**：
*   `http::request<...> request_`：代表客户端发来的请求。
*   `http::response<...> response_`：代表服务器要返回的响应。
*   `http::async_read(...)`：读取并解析客户端的请求。
*   `http::async_write(...)`：将构造好的响应发回客户端。

---

### 2. HTTP 报文结构

HTTP 报文是纯文本格式的，严格遵守一定的结构。无论是请求还是响应，都由四部分组成：

#### (1) 请求报文（对应代码中的 `request_`）
```http
POST /email HTTP/1.1                <-- 【起始行】包含 方法(POST)、目标(/email)、版本(HTTP/1.1)
Host: 127.0.0.1:8080                <-- 【头部字段】键值对，提供元数据
Content-Type: application/json
Content-Length: 45
                                    <-- 【空行】极其重要，标识头部结束，下面是身体
{"email": "test@example.com"}       <-- 【请求体】业务数据（仅在 POST/PUT 等方法中出现）
```
**映射到代码**：
*   `request_.method()`：获取起始行中的方法（如 `POST`）。
*   `request_.target()`：获取起始行中的目标路径（如 `/email`）。
*   `request_.version()`：获取 HTTP 版本号（如 11 代表 HTTP/1.1）。
*   `request_.body()`：获取请求体的原始数据（代码中用 `buffers_to_string` 将其转为 JSON 字符串）。

#### (2) 响应报文（对应代码中的 `response_`）
```http
HTTP/1.1 200 OK                     <-- 【起始行】包含 版本(HTTP/1.1)、状态码(200)、原因短语(OK)
Server: Beast                       <-- 【头部字段】
Content-Type: text/json
Content-Length: 89
                                    <-- 【空行】
{"error":0, "email":"test@..."}     <-- 【响应体】服务器返回的实际数据
```
**映射到代码**：
*   `response_.result(http::status::ok)`：设置状态码为 200。
*   `response_.set(http::field::server, "Beast")`：添加头部字段 `Server: Beast`。
*   `response_.set(http::field::content_type, "text/json")`：添加头部字段 `Content-Type`。
*   `beast::ostream(response_.body()) << jsonstr`：将数据写入响应体。

---

### 3. HTTP 方法

方法告诉服务器客户端想要对资源执行什么操作。代码中处理了两种最常见的方法：

*   **GET（获取）**：请求服务器返回指定资源的表示形式。**GET 请求不应该有请求体**，参数通常通过 URL 的 Query String 传递（如 `/time?format=unix`）。
    *   *代码映射*：`case http::verb::get`，直接通过 `request_.target()` 判断路径，然后返回数据。
*   **POST（提交）**：向服务器提交数据，通常用于创建新资源或提交表单/API 调用。**POST 请求通常包含请求体**，数据放在 Body 中。
    *   *代码映射*：`case http::verb::post`，代码提取了 `request_.body()` 中的 JSON 数据进行解析。

其他常见方法还有：PUT（更新）、DELETE（删除）、PATCH（局部更新）等。

---

### 4. HTTP 状态码

状态码是服务器对客户端请求的处理结果的三位数字标识。代码中使用了以下几种：

*   **2xx (成功)**：`200 OK`（代码中的 `http::status::ok`），表示请求被成功接收并处理。
*   **4xx (客户端错误)**：
    *   `400 Bad Request`（代码中的 `http::status::bad_request`）：客户端发送了语法错误的请求（比如代码中遇到非 GET/POST 的方法）。
    *   `404 Not Found`（代码中的 `http::status::not_found`）：服务器找不到请求的资源（比如代码中请求了 `/unknown` 路径）。
*   **5xx (服务器错误)**：如 `500 Internal Server Error`（代码中未显式处理，但如果 JSON 解析抛出异常未被捕获，Beast 可能会返回底层错误）。

---

### 5. 关键的 HTTP 头部字段

头部字段是 HTTP 的“控制面板”，决定了报文如何被传输和解析。

*   **Content-Type**：告诉接收方（客户端或服务器），报文体的数据是什么格式的。
    *   *代码中的瑕疵*：代码中 POST 成功返回时用了 `text/json`，**正确的标准 MIME 类型应该是 `application/json`**。对于 GET 请求返回的 HTML，使用 `text/html` 是正确的。
*   **Content-Length**：告诉接收方报文体有多少字节。这对于接收方判断何时停止读取至关重要。
    *   *代码映射*：`response_.content_length(response_.body().size())`。如果没有这一行，客户端可能会一直等待（以为数据还没传完），或者服务器在 HTTP/1.0 下会被迫关闭连接来表示数据结束。
*   **Host**：在 HTTP/1.1 中，请求必须包含 Host 字段，指明请求的主机名和端口。由于代码运行在本地，客户端通常会发送 `Host: 127.0.0.1:8080`。

---

### 6. HTTP 连接管理：短连接 vs 长连接

这是理解代码中 `keep_alive(false)` 和 `socket_.shutdown` 的关键。

*   **HTTP/1.0 默认短连接**：每次 HTTP 请求/响应后，底层的 TCP 连接就会断开。如果要请求 10 个图片，就要建立 10 次 TCP 连接，非常耗时。
*   **HTTP/1.1 默认长连接**：TCP 连接建立后，可以在这个连接上连续发送多个 HTTP 请求和响应，直到一方主动关闭或超时。大幅降低了延迟和服务器开销。

**映射到代码**：
*   `response_.keep_alive(false)`：代码**强制关闭了长连接**，告诉客户端“我处理完这个请求就断开 TCP，你别复用连接了”。
*   `self->socket_.shutdown(tcp::socket::shutdown_send, ec)`：响应发送完毕后，服务器主动关闭了 TCP 连接的发送端。
*   *为什么不支持长连接？* 支持长连接需要服务器在一个连接上**循环读取**后续的请求（即 `read_request` 完成后再调用 `read_request`），代码当前的逻辑是一次性的，处理完就销毁连接对象，这是最简单的 HTTP 服务器实现模式。

---

### 7. HTTP 与底层 TCP 的关系

HTTP 是基于 TCP 的。TCP 提供了可靠的、面向连接的字节流传输。

**映射到代码**：
*   `tcp::acceptor`：负责在操作系统层面监听 8080 端口，完成 TCP 的三次握手，建立 TCP 连接。
*   `tcp::socket`：代表一个已经建立的 TCP 连接通道。
*   `net::steady_timer`（超时定时器）：TCP 连接建立后，如果客户端迟迟不发 HTTP 请求（比如恶意攻击者只连不发），服务器资源会被白白占用。代码设置了 60 秒超时，超时则强制切断 TCP 连接。

---

### 💡 结合 HTTP 知识对代码的进阶建议

1.  **修正 Content-Type**：
    将 `this->response_.set(http::field::content_type, "text/json");` 改为 `this->response_.set(http::field::content_type, "application/json");`。
2.  **支持长连接**：
    目前的代码是典型的“一问一答即断开”模式。在高性能服务器中，应该检查 `request_.keep_alive()`，如果客户端希望保持连接，服务器在 `write_response` 后不应该立即 shutdown socket，而是重新调用 `read_request()` 等待下一个请求。
3.  **处理请求体大小限制**：
    当前代码直接读取整个请求体。如果有人发送一个 10GB 的 POST 请求，服务器内存就会爆掉。实际生产中需要配置 `http::request` 的最大体大小（如 `request_.body_limit(10 * 1024 * 1024)`限制为10MB）。