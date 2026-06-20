### 继续浅谈C++网络编程读写API

在 Boost.Asio 中，网络和底层 I/O 操作的 API 设计兼具灵活性与复杂性。理解同步与异步、全量与部分、通用流与 Socket 特定 API 之间的差异，是掌握 Asio 的关键。

以下是对您提到的 12 个 API 的十分详细的总结、对比与扩展。

---

### 第一部分：核心概念与分类基础

在深入具体 API 之前，我们需要明确几个 Asio 设计中的核心概念：

1.  **全量操作 vs 部分操作 (`_some`)**
    *   **全量操作** (`write`/`read`/`async_write`/`async_read`)：底层会自动循环调用部分操作，直到请求的缓冲区数据全部读写完毕，或者发生错误。它们是**组合操作**。
    *   **部分操作** (`_some`/`send`/`receive`/`async_*`)：底层只执行一次系统调用（如 `send`/`recv`/`read`/`write`），读写多少字节取决于内核缓冲区和网络状态，**不保证**一次性处理完所有数据。
2.  **自由函数 vs 成员函数**
    *   `write/read/async_write/async_read` 是**自由函数**，接受满足 `SyncWriteStream` / `SyncReadStream` 等概念的任意对象（如 `tcp::socket`, `ssl::stream`, `serial_port`）。
    *   `send/receive/async_send/async_receive` 是**Socket 特有的成员函数**，属于 `AsyncWriteStream` / `AsyncReadStream` 概念，通常用于底层 Socket 操作。
3.  **同步 vs 异步**
    *   同步 API 会阻塞当前线程，直到操作完成或出错。
    *   异步 API 会立即返回，将操作注册到 I/O 多路复用模型（如 `epoll`），操作完成后通过 I/O 对象关联的 `io_context` 调度回调（Completion Handler）。

---

### 第二部分：同步读写 API 详细对比

#### 1. `write_some` vs `send`
*   **`write_some` (成员函数)**：
    *   **语义**：向流写入数据，**至少写入 1 个字节**即返回。
    *   **适用范围**：所有满足 `SyncWriteStream` 概念的对象（Socket、串口、SSL 流等）。
    *   **特点**：不保证写完所有缓冲区数据。返回值为实际写入的字节数。
*   **`send` (Socket 成员函数)**：
    *   **语义**：与 `write_some` 几乎完全相同，底层同样调用系统级的 `send` 或 `write`。
    *   **适用范围**：仅限 Socket（`basic_socket` 及其派生类）。
    *   **特点**：**支持传递 Socket 特有的 flags**（如 `message_peek`, `message_out_of_band`）。这是它与 `write_some` 的唯一显著区别。

#### 2. `read_some` vs `receive`
*   **`read_some` (成员函数)**：
    *   **语义**：从流读取数据，**至少读取 1 个字节**即返回。
    *   **特点**：阻塞等待直到有数据可读。返回读取的字节数。如果对端关闭连接，会抛出 `boost::system::system_error` (EOF) 异常或返回错误码。
*   **`receive` (Socket 成员函数)**：
    *   **语义**：与 `read_some` 相同。
    *   **特点**：同样支持 Socket 级别的 flags 参数。对于 UDP 这种数据报套接字，`receive` 会读取一个完整的 UDP 数据包。

#### 3. `write` vs `read` (自由函数)
*   **`write` (自由函数)**：
    *   **语义**：**全量写入**。内部通过循环调用 `write_some`，直到缓冲区中的所有数据都被写入。
    *   **参数**：接受 `ConstBufferSequence`（如 `boost::asio::buffer`）或 `Streambuf`。
    *   **返回值**：写入的总字节数（通常等于缓冲区大小）。
    *   **注意**：对于 TCP，如果对端一直不读数据导致本端发送缓冲区满，`write` 会无限期阻塞。
*   **`read` (自由函数)**：
    *   **语义**：**全量读取**。内部循环调用 `read_some`，直到填满指定的缓冲区。
    *   **参数**：接受 `MutableBufferSequence` 或 `Streambuf`。
    *   **注意**：如果对端只发了 10 个字节，但你要求 `read` 读取 100 个字节，当前线程会一直阻塞，直到收到剩余 90 个字节或连接断开。这在网络编程中极易造成死锁（例如基于长度头的协议解析时）。

---

### 第三部分：异步读写 API 详细对比

异步 API 的行为逻辑与对应的同步 API 一致，但执行机制完全不同。它们不阻塞，而是将操作交由操作系统和 `io_context` 管理，完成后执行用户传入的**回调函数**。

#### 1. `async_write_some` vs `async_send`
*   **`async_write_some`**：
    *   **语义**：异步写入至少 1 个字节。立即返回。
    *   **回调时机**：底层 socket 变为可写时，执行一次写操作，然后调用回调。
    *   **回调参数**：`void(const boost::system::error_code& ec, std::size_t bytes_transferred)`
*   **`async_send`**：
    *   **语义**：同 `async_write_some`，仅限 Socket。
    *   **特点**：支持 Socket flags。底层调用系统级的异步发送接口。

#### 2. `async_read_some` vs `async_receive`
*   **`async_read_some`**：
    *   **语义**：异步读取至少 1 个字节。立即返回。
    *   **回调时机**：底层 socket 有数据可读时，执行一次读操作，然后调用回调。
    *   **特点**：常用于异步读取数据头的场景，因为不知道包大小，不能盲目等待缓冲区填满。
*   **`async_receive`**：
    *   **语义**：同 `async_read_some`，仅限 Socket。
    *   **特点**：支持 Socket flags。UDP 异步接收数据包的标准方式。

#### 3. `async_write` vs `async_read` (自由函数)
*   **`async_write`**：
    *   **语义**：异步**全量写入**。
    *   **内部机制**：Asio 会在内部自动、连续地发起多次 `async_write_some` 操作，直到所有数据发完。**你只需提供一次回调**，它会在所有数据发送完毕后被调用一次。
    *   **极其重要的警告**：数据缓冲区的生命周期必须一直持续到回调被调用！绝不能传递局部变量的缓冲区，否则会导致内存非法访问（悬垂引用）。
*   **`async_read`**：
    *   **语义**：异步**全量读取**。
    *   **内部机制**：内部连续发起多次 `async_read_some`，直到指定的缓冲区被完全填满。
    *   **警告**：同上，要求缓冲区生命周期持续到回调完成。并且要小心对端不发包导致的永久等待。

---

### 第四部分：核心对比矩阵

| API 名称               | 读写量 | 范围 (自由/成员) | 支持 Flags | 适用场景 (TCP/UDP/通用流) | 主要风险                         |
| :--------------------- | :----- | :--------------- | :--------- | :------------------------ | :------------------------------- |
| **`write_some`**       | 部分   | 成员函数         | 否         | 通用流                    | 返回字节数少于预期，需上层循环   |
| **`send`**             | 部分   | 成员函数         | 是         | Socket                    | 同上                             |
| **`read_some`**        | 部分   | 成员函数         | 否         | 通用流                    | 阻塞等待，EOF报错                |
| **`receive`**          | 部分   | 成员函数         | 是         | Socket                    | 同上                             |
| **`write`**            | 全量   | 自由函数         | 否         | 通用流                    | 对端不读导致无限阻塞             |
| **`read`**             | 全量   | 自由函数         | 否         | 通用流                    | 对端不写满指定长度导致无限阻塞   |
| **`async_write_some`** | 部分   | 成员函数         | 否         | 通用流                    | 需在回调中判断是否写完，否则丢包 |
| **`async_send`**       | 部分   | 成员函数         | 是         | Socket                    | 同上                             |
| **`async_read_some`**  | 部分   | 成员函数         | 否         | 通用流                    | 回调时数据可能不完整             |
| **`async_receive`**    | 部分   | 成员函数         | 是         | Socket                    | 同上                             |
| **`async_write`**      | 全量   | 自由函数         | 否         | 通用流                    | 缓冲区生命周期管理（悬垂引用）   |
| **`async_read`**       | 全量   | 自由函数         | 否         | 通用流                    | 同上，且协议需保证定长返回       |

---

### 第五部分：扩展与深度剖析

#### 1. 缓冲区与生命周期
*   **同步 API**：同步 API 在函数返回时，I/O 操作已经完成，因此可以使用栈上的局部数组或局部 `std::vector` 作为缓冲区。
*   **异步 API**：异步 API 立即返回，实际读写发生在未来的某个时刻。因此，**传入异步 API 的缓冲区必须使用堆内存（如 `std::shared_ptr<std::vector<char>>`、`std::string` 或 `boost::asio::streambuf`）**，并且需要通过值传递或智能指针在回调链中保持其生命周期。这是 Asio 异步编程中导致崩溃的头号原因。

#### 2. EOF（End Of File / 连接关闭）的语义
*   在 `read_some` / `async_read_some` 中，如果对端关闭连接：
    *   如果此时刚好读取了部分数据，Asio 会返回这部分数据，不报错（`error_code` 为 0）。
    *   如果再次调用读取且无数据可读，Asio 会返回 `boost::asio::error::eof`。
*   在 `read` / `async_read`（全量读取）中，如果要求读取 N 个字节，但中途对端关闭了连接：
    *   Asio 会立即终止组合操作，返回已读取的字节数，并通过 `error_code` 返回 `boost::asio::error::eof`。**全量操作绝不会静默忽略 EOF**。

#### 3. `dynamic_buffer` 与 `streambuf` 的配合
`read` 和 `async_read` 虽然要求“全量读取”，但在解析 HTTP 等变长协议时，我们不知道具体要读多少字节。Asio 提供了 `boost::asio::dynamic_buffer` 和 `streambuf`。
配合使用时，`read(stream, dynamic_buffer)` 会一直读取直到缓冲区填满（如果你设了上限）或 EOF。但更常见的是使用 `read_until`（虽然你没问，但这是 Asio 的重要组成部分），它能读取到特定的分隔符（如 `\r\n\r\n`）为止。

#### 4. 为什么 `async_write` 比 `async_write_some` 危险？
`async_write` 隐藏了内部多次调用的细节。如果你在一个类的成员函数中这样写：
```cpp
void send_msg() {
    std::string msg = "Hello World";
    // 错误！msg 在函数结束时销毁，但 async_write 还没写完
    boost::asio::async_write(socket, boost::asio::buffer(msg), handler); 
}
```
这会导致未定义行为（大概率崩溃）。
如果用 `async_write_some`，开发者通常会意识到只写了一部分，会更谨慎地管理缓冲区和后续的写入逻辑。
**最佳实践**：使用 `async_write` 时，将缓冲区绑定到 `std::shared_ptr<std::string>`，并确保其在 handler 中被捕获。

#### 5. UDP 中的特殊行为
对于 UDP（`udp::socket`），由于它是无连接的数据报协议：
*   **不应使用 `write`/`read`**：因为 UDP 没有流的概念，一次 `send` 就是一个完整的数据包，一次 `receive` 也是一个完整的数据包。
*   **必须使用 `send`/`receive` 或 `async_send`/`async_receive`**：如果用 `receive` 接收，给的缓冲区必须足够大，否则多出的数据会被丢弃；如果用 `async_receive`，回调中的 `bytes_transferred` 就是该数据包的实际大小。

### 总结
在 Boost.Asio 中：
1.  **追求简单与阻塞**：用 `read`/`write`，但要小心死锁（TCP 粘包/半包导致等待）。
2.  **追求底层控制**：用 `send`/`receive` (Socket) 或 `write_some`/`read_some` (通用流)。
3.  **高性能异步服务器**：几乎总是使用 `async_read_some` 和 `async_write`。用 `async_read_some` 探测数据头，解析出包长后，再通过 `async_read` 读取包体，最后用 `async_write` 一次性发送整个响应。同时极其注意缓冲区的生命周期管理。