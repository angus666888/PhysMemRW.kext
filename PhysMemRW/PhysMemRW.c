/*
 * Copyright (c) 2026 Angus. All rights reserved.
 * * This software is licensed under the Apache License 2.0 License.
 * Portions of this code may be used or modified, provided
 * that the original copyright notice is retained.
 */
#include <mach/mach_types.h>
#include <libkern/libkern.h>

// 显式导出符号，确保内核链接器能找到入口
__attribute__((visibility("default")))
kern_return_t PhysMemRW_start(kmod_info_t * ki, void *d) {
    // 可以在此处打印日志，确认驱动已加载
    printf("PhysMemRW: Generic C Entry Point Loaded.\n");
    return KERN_SUCCESS;
}

__attribute__((visibility("default")))
kern_return_t PhysMemRW_stop(kmod_info_t *ki, void *d) {
    printf("PhysMemRW: Generic C Entry Point Unloaded.\n");
    return KERN_SUCCESS;
}
