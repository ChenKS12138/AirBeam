// Copyright (c) 2025 ChenKS12138

#pragma once

#ifdef BUILD_DEBUG
#include <stdio.h>
#include <syslog.h>

#define ABDebugLog(format, ...)                                               \
  do {                                                                        \
    fprintf(stderr, "[AirBeamDebug] %s:%d: " format "\n", __FILE__, __LINE__, \
            ##__VA_ARGS__);                                                   \
    syslog(LOG_DEBUG, "[AirBeamDebug] %s:%d: " format, __FILE__, __LINE__,    \
           ##__VA_ARGS__);                                                    \
  } while (0)
#else
#define ABDebugLog(format, ...)
#endif
