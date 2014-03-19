#ifndef __TELEMETRY_H
#define __TELEMETRY_H

/* Initialise telemetry system
 *
 * One or both of telemetryf and telemetry_broker_uri must be
 * specified. 
 *
 * If telemetryf is specified, output will be written to that file
 *
 * If telemetry_broker_uri is specified, a zeromq PUB socket is connected
 * to that URI, and telemetry data is sent out there
 *
 * If client_name is provided, it will be prepended to each telemetry message
 * If client_name is NULL, a default will be used
 *
 * On success, 0 is returned
 * On error, -1 is returned and errno is set
 */
int init_telemetry(FILE *telemetryf, char *telemetry_broker_uri, char *client_name);
void shutdown_telemetry();

uint32_t telemetry_hash(const char *buf, size_t buflen);

void telemetry_vprintf(uint32_t tag, const char *format, va_list vargs);
void telemetry_printf(uint32_t tag, const char *format, ...);

#endif
