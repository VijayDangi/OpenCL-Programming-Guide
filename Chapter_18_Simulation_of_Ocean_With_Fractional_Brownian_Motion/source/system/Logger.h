#pragma once

#define Log(format, ...)  Logger::PrintLog( __LINE__, __FILE__, __FUNCTION__, format, __VA_ARGS__)

namespace Logger
{
    bool Initialize( const char *log_file_path);
    void PrintLog( int lineNo, char *fileName, char *functionName, char *format, ...);
} // namespace Logger

