#include <stdio.h>
#include <stdarg.h>
#include <txc/libc.h>

TM_WAIVER int printf(const char *format, ...);
TM_WAIVER int fprintf(FILE *stream, const char *format, ...);
TM_WAIVER int sprintf(char *str, const char *format, ...);
TM_WAIVER int snprintf(char *str, size_t size, const char *format,    ...);
TM_WAIVER int vprintf(const char *format, va_list ap);
TM_WAIVER int vfprintf(FILE *stream, const        char *format, va_list ap);
TM_WAIVER int vsprintf(char *str, const char *format, va_list ap);
TM_WAIVER int vsnprintf(char *str, size_t size, const char   *format, va_list ap);

TM_CALLABLE
int
txc_printf(const char *format, ...) {
	int ret;
  va_list ap;

  va_start(ap, format);
  ret = vprintf(format, ap);
  va_end(ap);
  return ret;
}

TM_CALLABLE
int
txc_fprintf(FILE *stream, const char *format, ...) {
	int ret;
  va_list ap;

  va_start(ap, format);
  ret = vfprintf(stream, format, ap);
  va_end(ap);
  return ret;
}

TM_CALLABLE
int
txc_sprintf(char *str, const char *format, ...) {
	int ret;
  va_list ap;

  va_start(ap, format);
  ret = vsprintf(str, format, ap);
  va_end(ap);
  return ret;
}

TM_CALLABLE
int
txc_snprintf(char *str, size_t size, const char *format, ...) {
	int ret;
  va_list ap;

  va_start(ap, format);
  ret = vsnprintf(str, size, format, ap);
  va_end(ap);
	return ret;
}

TM_CALLABLE 
int 
txc_vprintf(const char *format, va_list ap) {
	int ret;
	ret = vprintf(format, ap);
	return ret;
}

TM_CALLABLE 
int 
txc_vfprintf(FILE *stream, const        char *format, va_list ap) {
	int ret;
	ret = vfprintf(stream, format, ap);
	return ret;
}

TM_CALLABLE 
int 
txc_vsprintf(char *str, const char *format, va_list ap) {
	int ret;
	ret = vsprintf(str, format, ap);
	return ret;
}

TM_CALLABLE
int
txc_vsnprintf(char *str, size_t size, const char   *format, va_list ap) {
	int ret;
	ret = vsnprintf(str, size, format, ap);
	return ret;
}
