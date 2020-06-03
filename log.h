#pragma once

#include <stdio.h>
#include <errno.h>
#include <string.h>

#define LOG_PRINT(fmt, ...)     printf(fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)      printf("%s[%d] %s: "fmt"", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)       printf("%s[%d] %s: "fmt"", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define LOG_STDERR(fmt, ...)    printf("%s[%d] %s: errno(%d) errinfo(%s) >> "fmt"", __FILE__, __LINE__, \
                                        __FUNCTION__, errno, strerror(errno), ##__VA_ARGS__)
