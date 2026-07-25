## WebSocket相关知识补充



### 详细对比websocket报文与http报文

这是一个非常硬核且经典的问题。要详细对比 WebSocket 报文与 HTTP 报文，我们必须先建立一个核心认知：**WebSocket 的“报文”分为“握手报文”和“数据帧（Frame）”，而 HTTP 的“报文”分为“请求/响应行、头部、实体主体”。**

两者在**生命周期（建立阶段 vs 传输阶段）**、**格式（文本 vs 二进制）**、**头部开销**以及**语义（半双工 vs 全双工）**上有着天壤之别。

以下是六个维度的深度硬核对比：

### 1. 报文的“第一帧”与“后续帧”（生命周期的差异）
这是两者最根本的逻辑分野：

- **HTTP 报文**：每个请求/响应都是**独立、自包含**的。一次完整的 HTTP 事务（Request + Response）结束后，TCP 连接既可以断开，也可以被复用（Keep-Alive），但**下一次请求依然是全新的、格式完整的 HTTP 报文**。
- **WebSocket 报文**：**只存在一次 HTTP 报文**（即握手请求/响应）。一旦握手成功（返回 101 状态码），**后续所有通讯都不再使用 HTTP 报文**，而是使用 WebSocket 协议定义的**数据帧（Data Frame）**。

---

### 2. 握手阶段的报文对比（唯一相似之处）
WebSocket 的握手报文**本质上就是 HTTP 报文**，但它带有特定的“升级”头。

| 对比维度          | **HTTP 请求/响应（普通）**       | **WebSocket 握手（请求/响应）**                              |
| :---------------- | :------------------------------- | :----------------------------------------------------------- |
| **请求行/状态行** | `GET /index.html HTTP/1.1`       | `GET /chat HTTP/1.1`                                         |
| **核心头部**      | `Host`, `User-Agent`             | `Host`, `Upgrade: websocket`, `Connection: Upgrade`, `Sec-WebSocket-Key`, `Sec-WebSocket-Version` |
| **响应状态码**    | `200 OK`                         | `101 Switching Protocols`                                    |
| **响应关键头**    | `Content-Type`, `Content-Length` | `Upgrade: websocket`, `Sec-WebSocket-Accept`                 |
| **Body 携带数据** | 允许携带（如 JSON、HTML）        | **绝对不能携带**（握手成功前 Body 必须为空）                 |

---

### 3. 数据传输阶段的报文结构（核心硬核差异）
握手结束后，HTTP 退场，WebSocket 的“帧（Frame）”登场。这是最大的不同：**HTTP 是文本协议，WebSocket 是二进制协议**。

#### HTTP 报文（文本格式，人类可读）
```http
POST /api/data HTTP/1.1
Host: example.com
Content-Type: application/json
Content-Length: 27

{"username":"admin","pwd":"123"}
```
- 特点：全部是 ASCII 明文，包含大量冗余的头部字符串。

#### WebSocket 数据帧（二进制格式，机器解析）
WebSocket 报文分为 **Header（固定头）** + **Payload（负载）**。其头结构极其紧凑（2 ~ 14 字节），格式如下（位级别）：

| 位              | 名称                   | 作用                                                      | 对比 HTTP                                            |
| :-------------- | :--------------------- | :-------------------------------------------------------- | :--------------------------------------------------- |
| **Bit 0-7**     | FIN + RSV1-3 + Opcode  | 标记消息是否结束、数据类型（文本/二进制/Ping/Pong/Close） | HTTP 没有“消息结束标记”，依赖 Content-Length         |
| **Bit 8-15**    | Mask + Payload len     | 标记是否掩码（客户端必须掩码）、负载长度                  | HTTP 是 `Content-Length` 字符串（如 "27" 占 2 字节） |
| **后续字节**    | Extended Length (可选) | 若负载 >125，用 2 或 8 字节表示真长度                     |                                                      |
| **后续 4 字节** | Masking-key (可选)     | 用于异或解码（防缓存投毒）                                | HTTP 无此机制                                        |
| **最后**        | **Payload Data**       | 真正的业务数据（如 `{"type":"msg"}`）                     | 对应 HTTP 的 Body                                    |

**举例**：如果发送一个长度为 5 的文本 `"Hello"`（无掩码），WebSocket 报文仅占 **2 字节头部 + 5 字节数据 = 7 字节**。同样发送 "Hello"，HTTP/1.1 至少需要 `POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\nHello`，约 40+ 字节。

---

### 4. 头部开销与性能对比（高频场景下的质变）

| 对比项       | **HTTP/1.1 报文**                              | **WebSocket 数据帧**                                         |
| :----------- | :--------------------------------------------- | :----------------------------------------------------------- |
| **头部大小** | 平均 **200 ~ 800 字节**（Cookie、UA 等）       | **2 ~ 14 字节**（仅包含长度、类型、掩码）                    |
| **重复性**   | 每次请求都要重复发送相同的 Cookie、Host 等头部 | 握手时传一次，后续帧不再重复传输头部信息                     |
| **压缩**     | HTTP/1.1 明文无压缩（HTTP/2 有 HPACK）         | WebSocket 无强制压缩，但扩展协议（如 permessage-deflate）可压缩 payload |
| **适合场景** | 请求量大、单次载荷大（如下载文件、API 调用）   | 消息频率极高、单次载荷小（如游戏、K线行情、聊天）            |

---

### 5. 通讯模式与消息边界（语义上的降维打击）

- **HTTP 报文（半双工 + 无状态）**：必须是 **“一问一答”**。客户端发请求，服务器回响应。同一个连接上，第二个请求必须等第一个响应完成才能发出（管道化 Pipeline 受限）。消息边界依赖 `Content-Length` 或 `Transfer-Encoding: chunked`。
- **WebSocket 报文（全双工 + 有状态）**：建立连接后，**双方地位平等**。服务器可以主动向客户端推送报文（不需要客户端先问）。**消息边界由帧头中的 `Opcode`（操作码）和 `Payload Length` 原生定义**，不会发生 TCP 粘包问题，开发者无需自己定义分隔符。

---

### 6. 报文中的“控制帧”与“数据帧”之分（WebSocket 独有）
HTTP 报文中没有“控制指令”的概念（RST 包属于 TCP 层）。

WebSocket 报文头中的 **Opcode** 将其分为两类：

- **数据帧**（Opcode 0x1 文本 / 0x2 二进制）：承载业务逻辑数据。
- **控制帧**（Opcode 0x8 Close / 0x9 Ping / 0xA Pong）：**可以插入在数据帧之间发送**。例如，发大文件时，可以随时发一个 Ping 帧探测对方是否存活，而 HTTP 必须等当前请求传输完才能发送下一个请求。

---

### 总结一张极简思维导图

| 维度           | **HTTP 报文**                             | **WebSocket 报文**                               |
| :------------- | :---------------------------------------- | :----------------------------------------------- |
| **组成阶段**   | 全程都是 HTTP 格式                        | **握手**用 HTTP 格式，**数据传输**用二进制帧格式 |
| **报文格式**   | ASCII 文本（头部）+ 二进制/文本 Body      | 位操作定义的紧凑二进制帧（Header + Payload）     |
| **头部大小**   | 重（几百字节）                            | 轻（2~14 字节）                                  |
| **连接复用**   | Keep-Alive 复用连接，但串行响应           | 单一长连接，多消息并行交错（多路复用）           |
| **服务器推送** | 不支持（需 HTTP/2 Server Push，但已废弃） | **原生支持**，随时可推送数据帧                   |
| **粘包问题**   | 依赖 `Content-Length` 自行解决            | **帧协议天然解决**，无粘包                       |

**最后一句真言**：你可以把 WebSocket 理解为“**披着 HTTP 羊皮（握手阶段）的 TCP 狼（传输阶段）**”。握手时它规规矩矩地写着 HTTP 协议以穿透防火墙，握手完后它立刻撕掉 HTTP 的文本外衣，露出了二进制轻量化帧的獠牙。这就是为什么游戏服务、金融行情、实时协作首选 WebSocket，而普通 RESTful API 依然用 HTTP 的原因。



### 为什么websocket服务器还需tcp::acceptor作前置启动

这是一个非常典型且容易产生困惑的问题，尤其是对于刚接触 WebSocket 底层原理的开发者。

**直接回答：** WebSocket **并不是**一种独立的传输协议，它**依赖 TCP** 作为传输层。`tcp::acceptor` 负责建立最底层的 **TCP 连接（三次握手）**，而 WebSocket 的“握手”和“数据传输”都必须在**已经建立好的 TCP Socket** 上进行。

简单来说，流程是：**TCP 连接建立（Acceptor） → HTTP 升级请求（WebSocket 握手） → WebSocket 数据帧通信**。

结合你的代码，可以从以下三个层面来理解为什么“前置启动”是必须的：

### 1. 协议栈的层级决定（根本原因）
网络协议是分层的：

- **传输层（TCP）**：负责 IP 寻址、端口监听、可靠传输和流量控制。
- **应用层（WebSocket）**：负责具体的业务消息格式（帧、掩码、关闭控制等）。

你的 `WebSocketServer` 类中的 `_acceptor`（`net::ip::tcp::acceptor`）工作在传输层。它必须先“占住”端口（10086），监听网络接口，等着客户端发起的 **TCP SYN 包**。如果连 TCP 连接都没建立，操作系统根本不会把任何数据包递交给应用层，WebSocket 的握手包（HTTP GET 请求）也就无从谈起。

### 2. Beast 库的设计流程（代码映射）
在你的代码中，`acceptor` 和 WebSocket 的分工非常明确：

- **第一步（你的 `StartAccept`）**：
  ```cpp
  _acceptor.async_accept(con_ptr->GetSocket(), ...)
  ```
  这里拿到的是**裸的 TCP Socket**（`net::ip::tcp::socket`）。这一步完成的是 TCP 三次握手，此时双方都不知道对方是否要用 WebSocket。

- **第二步（你的 `AsyncAccept`）**：
  ```cpp
  _ws_ptr->async_accept(...)
  ```
  注意，你的 `Connection` 构造时已经将**裸 TCP Socket** 注入到了 `beast::websocket::stream` 中。`async_accept` 的作用是在**这个现存的 TCP 连接上**，读取客户端发来的 HTTP Upgrade 请求，校验 `Sec-WebSocket-Key`，并回复 101 状态码（Switching Protocols）。

**如果没有前置的 `acceptor`**，`_ws_ptr` 根本没有底层的 `socket` 可用，`async_accept` 也就无处执行。

### 3. 为什么不能直接用 `websocket::acceptor`？
Boost.Beast 库**没有**提供 `websocket::acceptor` 这样的类。因为 WebSocket 的握手复用的是 HTTP 语义，但它不负责监听端口。所有的监听和接受连接（Accept）逻辑，必须由底层的 `net::ip::tcp::acceptor` 承担。

### 4. 进阶场景：连接复用
即使在复杂的场景中（比如同一个端口既支持 HTTP 静态资源，又支持 WebSocket），底层依然是 `tcp::acceptor`。只是在 `async_accept` 拿到 TCP 连接后，Server 会去解析 HTTP 请求头：

- 如果请求头包含 `Upgrade: websocket`，就调用 `ws_stream.async_accept()` 升级为 WebSocket；
- 如果是普通的 `GET /index.html`，就交给 HTTP 路由处理。

**这时候，`acceptor` 前置启动的重要性更明显**——它负责“分发”原始 TCP 流，而 WebSocket 只是其中一种“处理策略”。

---

### 总结
`tcp::acceptor` 是地基，WebSocket `async_accept` 是地基上的房子。**必须先启动 Acceptor 接受连接，才能谈得上在这个连接上“升级”为 WebSocket 协议**。如果省略 Acceptor，你的服务器根本无法感知到客户端的网络请求，因为操作系统在网络栈的 TCP 层就拒绝了（端口未监听）。

结合你的代码，`WebSocketServer::StartAccept` 是一个**无限循环的回调链**，它不断调用 `async_accept` 接收新 TCP 连接，随后每个新连接都会自动进入 `AsyncAccept` 进行协议升级——这个“前置启动”正是所有网络服务（HTTP、TCP、WebSocket）的通用范式。



### tcp不是传输层吗，为什么只有tcp也能完成通讯、传输

这个问题问到了网络编程的核心本质，非常有洞察力。

你的理解完全正确：**TCP 确实是传输层协议。** 但正因为它是传输层，它**本身就是为了“完成通讯和传输”而设计的**。它根本不依赖应用层（如 HTTP 或 WebSocket）就能把数据从A点搬到B点。

要解开这个困惑，我们需要区分 **“传输数据”（搬运）** 和 **“解读数据”（语义）** 这两个概念：

### 1. TCP 本身就是“完美的搬运工”
传输层（Layer 4）的核心职责就是**端到端的可靠传输**。TCP 提供了：
- **连接建立**（三次握手）
- **数据分包与重组**（Segmentation）
- **差错校验与重传**（Checksum & Retransmission）
- **流量控制与拥塞控制**（Flow & Congestion Control）

当你建立了一个 TCP 连接（也就是你代码中的 `async_accept` 拿到 `socket` 后），**你其实已经拿到了一条双向的、可靠的“数据管道”**。你可以直接调用 `socket.write("Hello")`，对端就能收到 "Hello"。**此时，通讯已经完成了**，完全不需要 HTTP 或 WebSocket 插手。

### 2. 既然 TCP 能传数据，为什么还要 WebSocket？（边界与格式）
既然 TCP 能传 "Hello"，为什么你还要费劲实现 WebSocket 的 `async_accept` 和帧封装呢？

因为 TCP 是**流式（Stream）**协议，它**没有“消息边界”**。

- **TCP 的行为**：你发送了 3 次 `send("Hello")`，接收方可能一次 `recv` 就收到了 "HelloHelloHello"（粘包）；或者你发送了一个 1MB 的数据，接收方分 10 次才收完（拆包）。
- **WebSocket 的作用**：它在 TCP 流之上加了一层**轻量级的帧（Frame）**，给数据加上“头”（Header），标记了“这条消息多长”、“这是文本还是二进制”、“这是不是结束帧”。接收方通过解析这个头，就能准确地把一条条消息切分出来。

**结论**：如果你和你的客户端**商量好**，比如“每条消息以换行符 `\n` 结尾”，或者“前4个字节表示消息长度”，那么**你完全不需要 WebSocket，直接用 TCP 裸 socket 就能完美通讯**。

### 3. 为什么浏览器环境非得用 WebSocket？（门禁与安全）
既然裸 TCP 这么轻量高效，为什么你的 Web 服务器前端代码（JavaScript）不能用 `new Socket()` 直接连你的 TCP 端口呢？

**因为浏览器的安全策略（同源策略和防火墙）**。浏览器不允许网页直接发起任意 TCP 原始连接，因为这会带来极大的安全风险（比如恶意网页扫你内网端口）。

- **WebSocket 的巧妙之处**：它利用 **HTTP 协议（端口 80/443）** 作为敲门砖，发送一个 `Upgrade: websocket` 的请求头。因为浏览器默认允许发 HTTP 请求，这就能穿透防火墙和代理。
- 一旦服务器（你的代码中的 `_ws_ptr->async_accept`）同意升级，**后续的通讯就不再走 HTTP 了，而是退化为对 TCP 流的直接操控**（只不过加上了上面提到的“帧头”）。

### 4. 举一个最极端的例子（Redis）
你看 Redis（内存数据库）的通讯协议（RESP），它**直接在裸 TCP 上运行**，没有 HTTP，更没有 WebSocket。

客户端发 `*2\r\n$4\r\nPING\r\n`，Redis 收到后解析换行符，回复 `+PONG\r\n`。Redis 的服务端监听端口也是通过 `tcp::acceptor` 实现的，它从来没写过任何 `websocket::stream` 的代码，但全世界的服务器都在用 Redis 做高速通讯。

---

### 总结你的代码逻辑
回到你的 C++ 代码：

1. **`tcp::acceptor`** 是负责拉管道的（TCP 建立）。
2. 管道拉通后，你其实**立即**就能调用 `socket.write` 发数据了。
3. 但你选择了把它包装进 `beast::websocket::stream`，并调用了 `async_accept`。这**不是必须的传输步骤**，而是**为了兼容浏览器客户端**，并且**帮你自动解决粘包/拆包问题**（帮你加上帧头）。

所以，你的观察“只有 TCP 也能完成通讯”是绝对真理。WebSocket 只是 TCP 字节流上的一件“标准化马甲”，穿这件马甲是为了：**① 让浏览器能穿（过防火墙），② 让双方不用自己写复杂的拆包逻辑（标准化的帧）**。如果你的服务端只是给公司内部的高性能 C++ 客户端用，脱掉 WebSocket 这件马甲，直接用裸 TCP，性能反而更高。



