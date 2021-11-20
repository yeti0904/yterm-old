#pragma once
#include <stddef.h>

typedef struct string {
    char* buf;
    size_t size;
} string;