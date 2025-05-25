// Copyright (c) 2025 ChenKS12138

#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

#include <string>

namespace AirBeamCore {
namespace helper {
#ifdef BUILD_DEBUG
#warning message("Building in debug mode")
void ABDebugLog(const char* format, ...) {
  std::string with_prefix = "AirBeamASPDebug " + std::string(format);
  va_list args;
  va_start(args, format);
  vsyslog(LOG_NOTICE, with_prefix.c_str(), args);
  va_end(args);
}

#else
void ABDebugLog(const char* format, ...) {}
#endif
}  // namespace helper
}  // namespace AirBeamCore
