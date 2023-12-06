#include <circle/startup.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>

void exit(int code)
{
    halt();
}

time_t time(time_t* second)
{
    return 0;
}

struct tm *localtime (const time_t *_timer)
{
    static struct tm result;
    return &result;
}

int printf(const char* format, ...)
{
    return 0;
}

int puts(const char* text)
{
    return 0;
}

int snprintf(char* buffer, size_t size, const char* format, ...)
{
    return 0;
}

int vsnprintf(char* buffer, size_t size, const char* format, va_list arg)
{
    return snprintf(buffer, size, format, arg);
}