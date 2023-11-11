#include <stdio.h>
#include "Logger.h"
#include <string>
#include <stdarg.h>

static std::string log_file_name;

namespace Logger
{
    /**
     * @brief Initialize()
     * @param log_file_path 
     * @return 
     */
    bool Initialize(const char *log_file_path)
    {
        // variable declarartion
        FILE *fp = nullptr;

        // code
        log_file_name = std::string( log_file_path);
        if( fopen_s( &fp, "LogFile.txt", "w") != 0)
        {
            return false;
        }

        fclose( fp);
        fp = nullptr;

        return fp;
    }

    /**
     * @brief PrintLog()
     * @param lineNo 
     * @param fileName 
     * @param functionName 
     * @param format 
     * @param  
     */
    void PrintLog( int lineNo, char *fileName, char *functionName, char *format, ...)
    {
        // variable declaration
        FILE *fp = nullptr;

        // code
        fopen_s( &fp, "LogFile.txt", "a");

        if( fp)
        {
            va_list argList;

            va_start( argList, format);

                fprintf( fp, "[%s\\%s() : %d]: ", fileName, functionName, lineNo);
                vfprintf( fp, format, argList);
                fprintf( fp, "\n");
                fflush( fp);

#if defined(CONSOLE) || defined(_CONSOLE)
                fprintf( stdout, "[%s\\%s() : %d]: ", fileName, functionName, lineNo);
                vfprintf( stdout, format, argList);
                fprintf( stdout, "\n");
                fflush( stdout);
#endif
            
            va_end( argList);

            fclose( fp);
            fp = nullptr;
        }
    }

} // 

