### 浅谈co_await、co_spawn、detached、use_awaitable

要彻底弄懂这四个概念，你需要把它们分成两类：
1.  **C++ 语言级特性**：`co_await`（这是 C++20 编译器认识的关键字）。
2.  **Boost.Asio 库级适配器**：`use_awaitable`、`co_spawn`、`detached`（这是 Asio 为了让异步 API 能和 `co_await` 结合而提供的工具）。

下面我们按照**“如何启动” -> “如何等待” -> “如何连接” -> “如何善后”**的实际运行顺序，逐一拆解。

---

### 1. `co_spawn`：创造生命的火花（启动器）

**问题**：你写了一个返回 `awaitable<void>` 的协程函数，但协程函数和普通函数不同，你直接调用它，**它不会执行任何代码！** 它只会返回一个类似句柄的东西。

**作用**：`co_spawn` 的作用就是**把协程真正地“跑”起来**，把它挂载到 `io_context` 的事件循环中。

**怎么用**：
它通常需要三个参数：

```cpp
boost::asio::co_spawn(
    executor_or_io_context,  // 1. 在哪儿跑？（执行环境）
    your_coroutine_function(), // 2. 跑什么？（协程函数调用，注意带括号）
    completion_token          // 3. 跑完/出错后怎么办？（完成令牌）
);
```

**常见场景**：

*   **在 `main` 函数中启动第一个协程**：
    
    ```cpp
    boost::asio::io_context ioc;
    // 启动根协程 listener，用 detached 表示不管它的死活
    boost::asio::co_spawn(ioc, listener(), boost::asio::detached);
    ioc.run();
    ```
*   **在一个协程中启动另一个协程**（如前面代码中为新连接启动 `echo` 协程）：
    
    ```cpp
    // 在当前协程的执行器上，启动 echo 协程
    boost::asio::co_spawn(executor, echo(std::move(socket)), boost::asio::detached);
    ```

---

### 2. `co_await`：暂停与恢复（发动机）

**作用**：这是 C++20 的关键字，它的意思是**“我在这里等，等不到结果我就睡觉（挂起），把 CPU 让给别人；结果来了再叫醒我，我接着往下跑。”**

**怎么用**：
只能用在返回 `awaitable` 的协程函数内部。放在任何异步操作的前面。

```cpp
awaitable<void> my_coroutine() {
    // 遇到 co_await，当前协程挂起。
    // 直到 socket 读取到数据，协程才在此处恢复执行。
    std::size_t n = co_await socket.async_read_some(buffer, ...);
    
    // 继续往下走...
    co_await async_write(socket, ...);
}
```

**灵魂拷问：它和普通的阻塞函数（如 `socket.read_some()`）有什么区别？**
*   **阻塞**：线程卡死在这里，什么也干不了，浪费系统资源。
*   **`co_await` 挂起**：线程根本没卡死！线程偷偷溜走去 `io_context` 里执行其他就绪的协程了。等数据来了，线程再回来从断点继续执行。**代码看起来像同步，底层是完全异步的。**

---

### 3. `use_awaitable`：Asio 的魔法适配器（翻译官）

**问题**：Asio 以前是靠回调函数工作的（`async_read(..., my_callback)`）。现在你想用 `co_await`，但 Asio 的异步函数怎么知道你要用协程而不是回调呢？

**作用**：`use_awaitable` 是一个**完成令牌**。你把它传给 Asio 的异步函数，就是告诉 Asio：**“不要调回调了，请把结果变成一个可以 `co_await` 的对象给我。”**

**怎么用**：
永远和 `co_await` 绑定使用，作为 Asio 异步函数的最后一个参数。

```cpp
// 旧版回调写法：
socket.async_read_some(buffer, [](error_code ec, size_t n){
    // 回调地狱...
});

// 新版协程写法：
// use_awaitable 让 async_read_some 返回一个 awaitable，
// 然后 co_await 负责挂起并获取结果。
size_t n = co_await socket.async_read_some(buffer, boost::asio::use_awaitable);
```

**总结**：`co_await` 是 C++ 编译器的能力，`use_awaitable` 是 Asio 提供的接口，两者结合，才抹平了异步 API 和协程之间的鸿沟。

---

### 4. `detached`：我不关心结果（甩手掌柜）

**问题**：`co_spawn` 启动一个协程后，这个协程最终可能返回一个值，或者抛出异常。我们怎么接收这个结果？

**作用**：`detached` 的意思是**“分离的、不关心的”**。把它传给 `co_spawn`，表示：**启动这个协程后，我就不管它了。它成功也好，抛异常也罢，随它去。**

**怎么用**：
只能作为 `co_spawn` 的第三个参数。

```cpp
boost::asio::co_spawn(ioc, my_task(), boost::asio::detached);
```

**⚠️ 极其危险的陷阱**：
如果用 `detached` 启动的协程内部抛出了异常，且异常逃出了协程函数，**你的程序会直接崩溃（调用 `std::terminate`）！**

这就是为什么在之前的 Echo 服务器代码中，`echo` 协程内部必须要有 `try-catch`：
```cpp
awaitable<void> echo(tcp::socket socket) {
    try {
        // ... 读写逻辑，客户端断开会抛出 EOF 异常
    } catch (std::exception& e) {
        // 必须自己吃掉异常！因为外层 co_spawn 用了 detached，没人替你兜底！
        std::printf("Exception: %s\n", e.what());
    }
}
```

---

### 进阶：如果不用 `detached`，我想拿到协程的返回值怎么办？

`detached` 适合“发后即忘”的任务（如独立处理一个客户端连接）。但有时我们需要主协程等待子协程的结果。

**方法：在 `co_spawn` 前面加 `co_await`，并使用 `awaitable` 作为完成令牌！**

```cpp
// 子协程：返回一个 int
awaitable<int> calculate_something() {
    co_await boost::asio::this_coro::executor; // 模拟耗时
    co_return 42; // 协程返回值用 co_return
}

// 父协程
awaitable<void> parent_task() {
    auto executor = co_await boost::asio::this_coro::executor;

    // 启动子协程，第三个参数传 boost::asio::use_awaitable
    // 然后用 co_await 等待这个 co_spawn 完成，并获取返回值
    int result = co_await boost::asio::co_spawn(
        executor, 
        calculate_something(), 
        boost::asio::use_awaitable // 注意这里！
    );
    
    std::cout << "Got result: " << result << std::endl; // 输出 42
}
```

### 总结图谱

| 组件                | 属于       | 作用                                     | 经典搭配                                         |
| :------------------ | :--------- | :--------------------------------------- | :----------------------------------------------- |
| **`co_await`**      | C++20 语言 | 挂起当前协程，等待异步操作完成           | 永远放在 Asio 异步函数前                         |
| **`use_awaitable`** | Boost.Asio | 告诉异步函数返回可等待对象               | 永远作为异步函数的**最后一个参数**               |
| **`co_spawn`**      | Boost.Asio | 真正启动并运行一个协程                   | `main` 函数入口、新建并发任务                    |
| **`detached`**      | Boost.Asio | 告诉 `co_spawn` 不需要管协程的死活和结果 | 作为 `co_spawn` 的第三个参数，注意内部必须抓异常 |