#pragma once

#include <iostream>
#include <memory>
#include <mutex>

template <typename T>
class Singleton {
public:
	~Singleton() {
		std::cout << "Singleton destructed" << std::endl;
	}

	static std::shared_ptr<T> GetInstance() {
		static std::once_flag s_flag; //全局
		std::call_once(s_flag, [&]() {
			_instance = std::shared_ptr<T>(new T);
		});

		return _instance;
	}

protected:
	Singleton() = default;
	Singleton(const Singleton&) = delete;
	Singleton& operator =(const Singleton&) = delete;

	static std::shared_ptr<T> _instance;
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

//模板在编译阶段不会做太多的检测，只有在被实例化的时候，才会做检测

