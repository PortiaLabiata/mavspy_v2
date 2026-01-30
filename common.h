#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(v) (void)v

typedef uint8_t     u8_t;
typedef uint16_t    u16_t;
typedef uint32_t    u32_t;
typedef uint64_t    u64_t;

typedef int8_t     i8_t;
typedef int16_t    i16_t;
typedef int32_t    i32_t;
typedef int64_t    i64_t;

typedef struct {
    int code;
    const char *msg;
} msg_t;

#define RET_OK() ((msg_t){0, ""})
#define RET_ERR(c, m) ((msg_t){c, m})
#define IS_OK(m) (m.code == 0)
#define IS_ERR(m) (m.code != 0)

#define ASSERT_OK(m, s) do { \
    if (IS_ERR(m)) { \
        fprintf(stderr, "%s: ", s); \
        fprintf(stderr, "%s\n", m.msg); \
        exit(-m.code); \
    } \
} while (0)

#define streq(s1, s2) (strcmp(s1, s2) == 0)

#endif
