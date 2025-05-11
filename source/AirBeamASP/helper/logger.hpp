// Copyright (c) 2025 ChenKS12138

#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

#include <string>

#ifdef BUILD_DEBUG
#warning message("Building in debug mode")
inline void ABDebugLog(const char* format, ...) {
  std::string with_prefix = "AirBeamASPDebug " + std::string(format);
  va_list args;
  va_start(args, format);
  vsyslog(LOG_NOTICE, with_prefix.c_str(), args);
  va_end(args);
}

#else
inline void ABDebugLog(const char* format, ...) {}

#endif