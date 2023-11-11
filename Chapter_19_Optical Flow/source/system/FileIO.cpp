#include <string>
#include <fstream>
#include "Logger.h"

namespace FileIO
{
    /**
     * @brief ReadFile()
     * @param file_name : input
     * @param ret_file_data : output
     * @return bool
     */
    bool ReadFile( /* In */std::string file_name, /* Out */std::string& ret_file_data)
    {
        std::fstream file;
        file.open( file_name, std::ios::in);

        ret_file_data.clear();

        if( file.is_open())
        {
            file.seekg( 0, file.end);
            int length = file.tellg();
            file.seekg( 0, file.beg);

            char *buffer = new char[length];
            if( !buffer)
            {
                Log( "Error while reading file: \"%s\".", file_name.c_str());
                file.close();
                return false;
            }

            int index = 0;
            char ch;
            while( ( ch = file.get()) != EOF)
            {
                buffer[index++] = ch;
            }

            ret_file_data.assign( buffer, index);

            //Log( "Data: %s", ret_file_data.c_str());

            delete buffer;
            buffer = nullptr;

            file.close();

            return true;
        }

        return false;
    }

    /**
     * @brief WriteFile()
     * @param file_name : input
     * @param file_data : input
     * @return bool
     */
    bool Write( /* In */std::string file_name, /* Out */std::string& file_data)
    {
        std::fstream file;
        file.open( file_name, std::ios::out);

        if( file.is_open())
        {
            file.write( file_data.c_str(), file_data.size());
            file.close();

            return true;
        }

        return false;
    }

} // namespace FileIO
