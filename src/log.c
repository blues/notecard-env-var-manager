#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "note-c/note.h"

#include "log.h"

static uint8_t maxLogLevel = LOG_LEVEL_ERROR;

// TODO: Better bounds checking in this function. I'm nearly positive it's
//       buggy.
void logMsg(uint8_t level, const char *file, int line, const char *func,
            const char *format, ...)
{
    if (level > maxLogLevel) {
        return;
    }

    char msg[512];
    size_t remaining = sizeof(msg);
    const char *levelStr = "";

    switch (level) {
    case LOG_LEVEL_INFO:
        levelStr = "INFO";
        break;
        break;
    case LOG_LEVEL_ERROR:
        levelStr = "ERROR";
        break;
    case LOG_LEVEL_DEBUG:
        levelStr = "DEBUG";
        break;
    default:
        break;
    }

    char *idx = msg;
    int written = snprintf(idx, remaining, "%s @ %s:%d: (%s) ", func, file,
                           line, levelStr);
    if (written < 0) {
        return;
    }
    idx += written;
    remaining -= written;

    va_list args;
    va_start(args, format);
    written = vsnprintf(idx, remaining, format, args);
    va_end(args);
    if (written < 0) {
        return;
    }
    idx += written;
    remaining -= written;

    written = snprintf(idx, remaining, "\r\n");
    if (written < 0) {
        return;
    }

    NoteDebug(msg);
}

void setMaxLogLevel(uint8_t level)
{
    maxLogLevel = level;
}
