#define _CRT_SECURE_NO_WARNINGS

//字节序的问题
// 
//在计算机网络中，由于不同的计算机使用的 CPU 架构和字节顺序可能不同，因此在传输数据时需要对数据的字节序进行统一，以保证数据能够正常传输和解析。这就是网络字节序的作用。
//具体来说，计算机内部存储数据的方式有两种：大端序（Big - Endian）和小端序（Little - Endian）。
//在大端序中，高位字节存储在低地址处，而低位字节存储在高地址处；在小端序中，高位字节存储在高地址处，而低位字节存储在低地址处。
//在网络通信过程中，通常使用的是大端序。这是因为早期的网络硬件大多采用了 Motorola 处理器，而 Motorola 处理器使用的是大端序。此外，大多数网络协议规定了网络字节序必须为大端序。
//因此，在进行网络编程时，需要将主机字节序转换为网络字节序，也就是将数据从本地字节序转换为大端序。可以使用诸如 htonl、htons、ntohl 和 ntohs 等函数来实现字节序转换操作。
//综上所述，网络字节序的主要作用是统一不同计算机间的数据表示方式，以保证数据在网络中的正确传输和解析。


//如何区分本机字节序
//可以通过判断低地址存储的数据是否为低字节数据，如果是则为小端，否则为大端，下面写一段代码讲述这个逻辑

#include <iostream>

using namespace std;

bool is_big_endian() {
	int num = 1;
	if (*(char*)(&num) == 1) {
		return false;//小端序
	}
	else {
		return true;//大端序
	}
}

int main() {
	int num = 0x12345678;
	unsigned char* p = (unsigned char*)&num;

	if (is_big_endian()) {
		cout << "当前系统为大端序" << endl;
		cout << "字节序为：";
		for (int i = 0; i < sizeof(num); i++) {
			cout << hex << (int)*(p + i) << " ";
		}
		cout << endl;
	}
	else {
		cout << "当前系统为小端序" << endl;
		cout << "字节序为：";
		for (int i = sizeof(num) - 1; i >= 0; i--) {
			cout << hex << (int)*(p + i) << " ";
		}
		cout << endl;
	}

	return 0;
}