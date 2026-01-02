/*
 * Copyright (c) 2026 Angus. All rights reserved.
 * * This software is licensed under the Apache License 2.0 License.
 * Portions of this code may be used or modified, provided
 * that the original copyright notice is retained.
 */
#ifndef shared_types_h
#define shared_types_h

#include <stdint.h>

typedef struct {
    uint64_t phys_addr;
    uint64_t data;
    uint32_t size;      // 1, 2, 4, 8
    uint32_t is_write;  // 1 for write, 0 for read
} mmio_request_t;

#define k_phys_mmio_call 1

#endif
