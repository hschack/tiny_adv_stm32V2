#ifndef Advanced_GsmLog_h
#define Advanced_GsmLog_h

#include <Print.h>
#include "../api/Shared.h"

#ifndef ADVGSM_LOG_SEVERITY
#define ADVGSM_LOG_SEVERITY 9
#endif

// See: https://opentelemetry.io/docs/reference/specification/logs/overview/

enum GsmSeverity {
  Trace = 1,
  Debug = 5,
  Info = 9,
  Warn = 13,
  Error = 17,
  Fatal = 21
};

#if ADVGSM_LOG_SEVERITY > 0
class GsmLog {
 public:
  Print* Log = nullptr;
  const char* Severity[6] = {"TRACE", "DEBUG", "INFO",
                             "WARN",  "ERROR", "FATAL"};
};

extern GsmLog AdvancedGsmLog;

// See: https://www.graylog.org/post/log-formats-a-complete-guide

#define ADVGSM_LOG(severity, tag, format, ...)                              \
  if (AdvancedGsmLog.Log != nullptr && severity >= ADVGSM_LOG_SEVERITY) {   \
    AdvancedGsmLog.Log->printf("[%d] <%s> %s: ", millis(),                  \
                               AdvancedGsmLog.Severity[(severity - 1) / 4], \
                               tag);                                        \
    AdvancedGsmLog.Log->printf(format __VA_OPT__(, ) __VA_ARGS__);          \
    AdvancedGsmLog.Log->print("\n");                                        \
  }
#else
#define ADVGSM_LOG(severity, tag, format, ...) ;
#endif

#endif