/*
 * Copyright (c) 2026 Angus. All rights reserved.
 * * This software is licensed under the Apache License 2.0 License.
 * Portions of this code may be used or modified, provided
 * that the original copyright notice is retained.
 */
// 确保在旧版系统编译环境下也能识别新版常量名
#ifndef kIOMainPortDefault
#define kIOMainPortDefault kIOMasterPortDefault
#endif
#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOMemoryDescriptor.h>
#include "shared_types.h" // 确保你有定义 mmio_request_t 的头文件

class PhysMemRW : public IOService {
    OSDeclareDefaultStructors(PhysMemRW)

public:
    // 驱动启动
    virtual bool start(IOService *provider) override {
        if (!IOService::start(provider)) return false;
        
        IOLog("PhysMemRW: IOKit Service started successfully.\n");
        
        // 注册服务，以便用户态可以通过 IOServiceMatching 找到它
        registerService();
        return true;
    }

    // 驱动停止
    virtual void stop(IOService *provider) override {
        IOLog("PhysMemRW: IOKit Service stopped.\n");
        IOService::stop(provider);
    }

    // MMIO 核心读写函数
    IOReturn perform_mmio(mmio_request_t *req) {
            // 1. 基础参数合法性检查
            if (!req || (req->size != 1 && req->size != 2 && req->size != 4 && req->size != 8)) {
                return kIOReturnBadArgument;
            }

            // 2. 增加对齐检查 (防止非对齐访问导致的硬件异常/崩溃)
            // 地址必须能被 size 整除
            if ((req->phys_addr % req->size) != 0) {
                IOLog("PhysMemRW: Error - Unaligned access at 0x%llx (size: %d)\n", req->phys_addr, req->size);
                return kIOReturnBadArgument;
            }

            // 3. 增加物理地址范围保护 (示例：假设不访问 0 附近和极高地址)
            // 实际开发中可以通过 gPEClockFrequencyInfo.physAddrMax 获取最大物理地址
            if (req->phys_addr < 0x100 || req->phys_addr > 0x1000000000ULL) { // 限制在 64GB 以内
                IOLog("PhysMemRW: Error - Address out of bounds: 0x%llx\n", req->phys_addr);
                return kIOReturnVMError;
            }

            // 4. 根据物理地址创建描述符
            IOMemoryDescriptor *desc = IOMemoryDescriptor::withPhysicalAddress(req->phys_addr, req->size, kIOMemoryDirectionInOut);
            if (!desc) {
                IOLog("PhysMemRW: Error - Failed to create descriptor for 0x%llx\n", req->phys_addr);
                return kIOReturnVMError;
            }

            // 5. 映射到内核虚拟空间
            IOMemoryMap *map = desc->map(kIOMapInhibitCache);
            if (!map) {
                IOLog("PhysMemRW: Error - Failed to map memory for 0x%llx\n", req->phys_addr);
                desc->release();
                return kIOReturnVMError;
            }

            volatile void *v_addr = (void *)map->getVirtualAddress();
            
            // 再次检查虚拟地址是否有效
            if (!v_addr) {
                map->release();
                desc->release();
                return kIOReturnVMError;
            }

            // 6. 执行实际的内存操作
            if (req->is_write) {
                switch (req->size) {
                    case 1: *(uint8_t  *)v_addr = (uint8_t)req->data;  break;
                    case 2: *(uint16_t *)v_addr = (uint16_t)req->data; break;
                    case 4: *(uint32_t *)v_addr = (uint32_t)req->data; break;
                    case 8: *(uint64_t *)v_addr = (uint64_t)req->data; break;
                }
            } else {
                switch (req->size) {
                    case 1: req->data = *(uint8_t  *)v_addr; break;
                    case 2: req->data = *(uint16_t *)v_addr; break;
                    case 4: req->data = *(uint32_t *)v_addr; break;
                    case 8: req->data = *(uint64_t *)v_addr; break;
                }
                IOLog("PhysMemRW: [READ] Phys: 0x%llx -> Data: 0x%llx (Size: %d)\n",
                      req->phys_addr, req->data, req->size);
            }

            // 7. 清理资源
            map->release();
            desc->release();
            return kIOReturnSuccess;
        }
};

// 宏定义：必须与类名一致
OSDefineMetaClassAndStructors(PhysMemRW, IOService)
#include <IOKit/IOUserClient.h>

class PhysMemRW_user_client : public IOUserClient {
    OSDeclareDefaultStructors(PhysMemRW_user_client)
    PhysMemRW* fProvider;
public:
    virtual bool start(IOService* provider) override {
        fProvider = OSDynamicCast(PhysMemRW, provider);
        return (fProvider != NULL) ? IOUserClient::start(provider) : false;
    }

    virtual IOReturn externalMethod(UInt32 selector, IOExternalMethodArguments* args,
                                    IOExternalMethodDispatch* dispatch, OSObject* target, void* reference) override {
        if (selector == 0) {
            // 验证输入和输出缓冲区是否存在且大小正确
            if (!args->structureInput || args->structureInputSize < sizeof(mmio_request_t)) return kIOReturnBadArgument;
            if (!args->structureOutput || args->structureOutputSize < sizeof(mmio_request_t)) return kIOReturnBadArgument;

            // 从输入缓冲区读取请求
            mmio_request_t *inReq = (mmio_request_t *)args->structureInput;
            
            // 创建一个临时拷贝用于操作，或者直接操作输出缓冲区
            mmio_request_t tempReq = *inReq;

            // 执行物理内存读取
            IOReturn kr = fProvider->perform_mmio(&tempReq);

            // 将处理后的结果（包含读取到的数据）拷贝到输出缓冲区
            memcpy(args->structureOutput, &tempReq, sizeof(mmio_request_t));
            
            // 告知系统实际写回了多少字节
            args->structureOutputSize = sizeof(mmio_request_t);

            return kr;
        }
        return kIOReturnBadArgument;
    }
};

OSDefineMetaClassAndStructors(PhysMemRW_user_client, IOUserClient)
