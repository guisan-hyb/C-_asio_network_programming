## 浅谈C++网络编程读写API

在 C++ 网络编程（尤其是结合 Boost.Asio 和底层 Socket API）时，这几组函数的命名很容易让人混淆。

要彻底理清它们，我们需要从 **“操作系统底层 API”** 和 **“Boost.Asio 封装”** 两个层级来对比。

简单来说：
*   `send` / `receive` (实际上是 `recv`) 是**操作系统底层**的词。
*   `read` / `write` 是**操作系统底层**的通用词，也是 **Asio** 中“保证读完/写完”的高级封装。
*   `read_some` / `write_some` 是 **Asio** 中“能读多少算多少”的基础封装。

下面为你详细拆解：

---

### 第一层级：操作系统底层 API (POSIX / WinSock)

在网络操作系统底层，所有的 Socket 通信本质都是文件 I/O。

#### 1. `read` / `write`
*   **来源**：标准 POSIX 文件 I/O 函数。
*   **原型**：`ssize_t read(int fd, void *buf, size_t count);`
*   **特点**：因为 Linux 中“一切皆文件”，Socket 也是文件描述符，所以你可以直接用 `read` 和 `write` 来操作 Socket。
*   **行为**：它们是“短读/短写”。也就是说，你让它写 100 字节，可能底层缓冲区满了，它只写了 30 字节就返回了，剩下的 70 字节需要你自己写循环处理。

#### 2. `send` / `recv` 

*   **来源**：Berkeley Sockets (BSD) 标准，专为网络设计的 API。
*   **原型**：`ssize_t send(int sockfd, const void *buf, size_t len, int flags);`
*   **特点**：与 `read`/`write` 功能几乎一样，但多了一个极其重要的 `flags` 参数。
*   **行为**：也是“短读/短写”。
*   **优势**：可以通过 `flags` 设置网络特有行为。例如：
    *   `MSG_OOB`：发送/接收带外数据。
    *   `MSG_PEEK`：偷看缓冲区数据但不移除。
    *   `MSG_DONTWAIT`：非阻塞操作。

> **底层总结**：在底层，`send`/`recv` 和 `read`/`write` 对 Socket 的作用基本一样，只是 `send`/`recv` 提供了网络专用的控制标志。

---

### 第二层级：Boost.Asio 的封装

Boost.Asio 为了提供现代化的 C++ 异步接口，对底层 API 进行了封装，并引入了新的命名规则。

#### 1. `read_some` / `write_some` (尽力而为型)
*   **性质**：Asio 中 `socket` 对象的**成员函数**。
*   **行为**：**“能读/写多少就算多少”**。
    *   调用 `write_some` 时，你传给它一个 100 字节的 Buffer，如果底层此刻只能发 30 字节，它就只发 30 字节，然后立刻返回 `30`。
*   **应用场景**：常用于底层异步框架的回调中，或者你需要极其精细控制每一次 I/O 边缘情况的场景。
*   **缺点**：如果你要发 100 字节，你必须自己写 `while` 循环去处理剩余的 70 字节（就像你之前写的代码一样）。

#### 2. `read` / `write` (完整交付型)
*   **性质**：Asio 提供的**全局自由函数**（不是成员函数，需要把 socket 作为参数传进去）。
*   **原型**：`template <typename SyncWriteStream, typename ConstBufferSequence> std::size_t write(SyncWriteStream& s, const ConstBufferSequence& buffers);`
*   **行为**：**“保证全部读完/写完，否则阻塞或报错”**。
    *   调用 `boost::asio::write(sock, buffer)` 时，你传 100 字节，Asio 内部会自动帮你写一个 `while` 循环，不断调用底层的 `write_some`，直到 100 字节全部塞进底层缓冲区才返回。
*   **应用场景**：当你不需要关心底层的“短读写”细节，只求“把这一坨数据完整发过去”时，用这个最省心。

---

### 详细对比表

| 函数名                         | 层级归属    | 作用域/调用方式   | 核心行为 (针对 100 字节请求)             | 是否自动循环处理剩余数据? | 典型使用场景                                   |
| :----------------------------- | :---------- | :---------------- | :--------------------------------------- | :------------------------ | :--------------------------------------------- |
| **`send` / `recv`**            | OS 底层 API | C 全局函数        | 短读/短写，可能只处理 30 字节            | 否                        | C 语言网络编程；需要使用 `flags` (如 MSG_PEEK) |
| **`read` / `write`**           | OS 底层 API | C 全局函数        | 短读/短写，可能只处理 30 字节            | 否                        | Linux 系统编程，把 Socket 当普通文件处理       |
| **`read_some` / `write_some`** | Boost.Asio  | Socket 的成员函数 | 短读/短写，可能只处理 30 字节            | 否                        | 精细控制每一次 I/O；自定义底层网络框架         |
| **`read` / `write`**           | Boost.Asio  | 全局自由函数      | **完整读/写**，必须处理完 100 字节才返回 | **是**                    | 业务层逻辑，确保一整个数据包完整发送/接收      |

*(注意：Asio 中也有 `async_read_some` / `async_write_some` 和 `async_read` / `async_write`，它们的区别同样对应上述表格中的“是否自动循环”。)*

---

### 代码对比体会（Boost.Asio 场景）

**场景：发送一个 1024 字节的数据包**

**方式一：使用 `write_some`（麻烦，需手动循环）**
```cpp
std::string data(1024, 'A');
size_t total_sent = 0;
while (total_sent != data.size()) {
    // 必须自己偏移指针，计算剩余长度
    total_sent += sock.write_some(
        boost::asio::buffer(data.data() + total_sent, data.size() - total_sent)
    );
}
```

**方式二：使用 Boost.Asio 的 `write`（简单，一行搞定）**
```cpp
std::string data(1024, 'A');
// Asio 内部自动帮你循环调用 write_some
std::size_t bytes_transferred = boost::asio::write(sock, boost::asio::buffer(data));
// bytes_transferred 必定等于 1024 (除非网络断开抛出异常)
```

### 总结建议

1. **在 Boost.Asio 的练习和日常业务开发中**：尽量多使用 `boost::asio::read` 和 `boost::asio::write`（或其异步版本 `async_read` / `async_write`），它们帮你省去了处理“短读写”的麻烦。
2. **理解 `write_some` 非常重要**：因为 `write_some` 是底层真实的 I/O 行为，Asio 的 `write` 不过是对 `write_some` 的安全封装。理解了 `write_some`，你才算真正理解了网络字节流的本质。