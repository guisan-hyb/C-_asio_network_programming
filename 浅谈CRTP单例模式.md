### 浅谈CRTP单例模式

在 CRTP（Curiously Recurring Template Pattern，奇异递归模板模式）实现的单例模式中，基类（即单例模板类）的构造函数通常被声明为 `protected` 而不是 `private`，其根本原因在于 **C++ 的继承与对象构造机制**。

简单来说：**`private` 会阻断派生类的构造，导致编译报错；而 `protected` 既能阻止外部直接实例化，又能允许派生类完成自身的构造初始化。**

下面为您详细拆解背后的原理：

### 1. CRTP 单例的基本结构
首先，我们看一个典型的 CRTP 单例模式代码骨架：

```cpp
template <typename T>
class Singleton {
public:
    static T& GetInstance() {
        static T instance; // 在此调用 T 的构造函数
        return instance;
    }

protected: // <--- 为什么这里是 protected？
    Singleton() = default;
    ~Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};

// 业务逻辑类，继承自单例模板
class MyConfig : public Singleton<MyConfig> {
    friend class Singleton<MyConfig>; // 声明基类为友元，以便基类能调用派生类的私有构造函数
private:
    MyConfig() = default; // 派生类自己的构造函数是 private 的
public:
    void DoSomething() {}
};
```

### 2. 为什么不能用 `private`？（对象构造的调用链）

在 C++ 中，当创建一个派生类对象时，会**先调用基类的构造函数，然后再调用派生类的构造函数**。

在上述 `GetInstance()` 方法中，执行了 `static T instance;`（即 `MyConfig instance;`）。这个过程在编译期的底层逻辑等价于：
1. 分配内存。
2. 调用基类 `Singleton<MyConfig>` 的构造函数。
3. 调用派生类 `MyConfig` 的构造函数。

**如果基类 `Singleton` 的构造函数是 `private`：**
* `private` 成员的特性是：**只有类自身（和友元）能访问**。
* 当尝试构造 `MyConfig` 时，编译器发现需要调用 `Singleton<MyConfig>` 的构造函数，但由于该构造函数是 `private` 的，且 `MyConfig` 无法访问基类的 `private` 成员。
* **结果**：编译器会直接报错，提示 `‘Singleton<T>::Singleton()’ is private within this context`。你的单例类根本无法被实例化。

### 3. 为什么 `protected` 是完美的解？

`protected` 成员的特性是：**类自身、其派生类（子类）可以访问，但外部不可访问**。

* **对外防实例化**：在类外部（比如 `main` 函数里），你无法直接写 `Singleton<MyConfig> s;`，因为外部无法访问 `protected` 构造函数。这保证了单例模式“禁止外部直接构造”的核心要求。
* **对内放行构造链**：当 `GetInstance()` 内部构造 `MyConfig` 时，由于 `MyConfig` 继承自 `Singleton<MyConfig>`，它拥有访问基类 `protected` 成员的权限。因此，构造 `MyConfig` 时调用基类 `Singleton` 的构造函数是合法的，编译通过。

### 4. 关于派生类（业务类）的构造函数

你可能会注意到，在上面的代码中，**派生类 `MyConfig` 的构造函数反而是 `private` 的**。

* 因为真正的单例要求外部不能随便 `new MyConfig()` 或者 `MyConfig m;`，所以派生类的构造函数必须是 `private` 的。
* 但是 `GetInstance()` 里面又要 `static T instance;` 构造它，这就产生矛盾了：基类怎么能调用派生类的 `private` 构造函数呢？
* 这就是为什么派生类内部必须写一句 `friend class Singleton<MyConfig>;` 的原因。通过声明基类为友元，基类的 `GetInstance()` 方法就获得了调用派生类 `private` 构造函数的特权。

### 总结

在 CRTP 单例模式中：
* **基类（模板类）构造函数用 `protected`**：是为了让派生类在构造时能合法地调用基类构造函数，完成对象初始化链。
* **派生类（业务类）构造函数用 `private`**：是为了禁止外部直接实例化派生类，配合基类的 `friend` 声明，只允许通过基类的 `GetInstance()` 来创建唯一实例。

这套机制完美地实现了**既阻止了外部的随意实例化，又保证了 C++ 对象构造语法树的合法通过**。