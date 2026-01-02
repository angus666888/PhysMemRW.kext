/*
 * Copyright (c) 2026 Angus. All rights reserved.
 * * This software is licensed under the Apache License 2.0 License.
 * Portions of this code may be used or modified, provided
 * that the original copyright notice is retained.
 */
#include <iostream>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include "shared_types.h"

int main() {
    kern_return_t kr;
    io_service_t service;
    io_connect_t connect;

    // 使用 kIOMasterPortDefault 以适配 macOS 11
    service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("PhysMemRW"));
    
    if (!service) {
        std::cerr << "[-] 无法匹配到 PhysMemRW 服务，请确认驱动已通过 kextutil 加载" << std::endl;
        return -1;
    }

    // 打开连接
    kr = IOServiceOpen(service, mach_task_self(), 0, &connect);
    IOObjectRelease(service);

    if (kr != kIOReturnSuccess) {
        std::cerr << "[-] 无法打开驱动连接: 0x" << std::hex << kr << std::endl;
        return -1;
    }

    std::cout << "[+] 成功连接到 PhysMemRW 驱动" << std::endl;

    mmio_request_t req;
    req.phys_addr = 0xFF00D400;
    req.size = 1;
    req.is_write = 0;
    req.data = 0xDEADBEEF; // 初始值

    size_t outputSize = sizeof(req); // 必须初始化为缓冲区大小

    // 同时传入 &req 作为输入和输出
    kr = IOConnectCallMethod(connect, 0,
                             NULL, 0,                // scalar input
                             &req, sizeof(req),      // structure input
                             NULL, NULL,             // scalar output
                             &req, &outputSize);     // structure output

    if (kr == kIOReturnSuccess) {
        std::cout << "[+] 读取成功! 数据: 0x" << std::hex << req.data << std::endl;
    } else {
        // 如果还是 0xe00002c2，请检查驱动的 externalMethod 参数校验
        std::cerr << "[-] 通信失败: 0x" << std::hex << kr << std::endl;
    }

    IOServiceClose(connect);
    return 0;
}
