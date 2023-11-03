#pragma once

#include <string>

namespace FileIO
{
    /**
     * @brief ReadFile()
     * @param file_name : input
     * @param ret_file_data : output
     * @return bool
     */
    bool ReadFile( /* In */std::string file_name, /* Out */std::string& ret_file_data);

    /**
     * @brief WriteFile()
     * @param file_name : input
     * @param file_data : input
     * @return bool
     */
    bool Write( /* In */std::string file_name, /* Out */std::string& file_data);

} // namespace FileIO
