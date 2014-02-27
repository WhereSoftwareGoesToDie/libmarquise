#ifndef __TELEMETRY_H
#define __TELEMETRY_H

int init_telemetry();
void shutdown_telemetry();

uint32_t telemetry_hash(const char *buf, size_t buflen);

void telemetry_vprintf(uint32_t tag, const char *format, va_list vargs);
void telemetry_printf(uint32_t tag, const char *format, ...);

#endif
