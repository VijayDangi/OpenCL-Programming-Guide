/**
 * @author : Vijaykumar Dangi
 * @date   : 21-July-2023
 */

#pragma once

#define __To_String(x) #x

/**
 * @brief appendBitfield()
 */
template<typename T>
void appendBitfield( T info, T value, std::string name, std::string &str)
{
    // code
    if( info & value)
    {
        if( str.length() > 0)
        {
            str.append(" | ");
        }
        str.append( name);
    }
}

/**
 * @brief class InfoDevice()
 * @description Diplay information for a particular device. As different calls to clGetDeviceInfo() may return
 *              values of different types a template is used.
 *              As some values returned are arrays of values, a template class is used so it can be specialized for
 *              this case.
 */
template<typename T>
class InfoDevice
{
    public:
        static void display( cl_device_id id, cl_device_info name, std::string str)
        {
            // variable declaration
            cl_int ocl_err;
            std::size_t param_value_size;

            // code
            ocl_err = clGetDeviceInfo( id, name, 0, nullptr, &param_value_size);
            if( ocl_err != CL_SUCCESS)
            {
                std::cerr << "Failed to find OpenCL device info " << str << "." << std::endl;
                return;
            }

            T *info = (T *) alloca( sizeof( T) * param_value_size);
            ocl_err = clGetDeviceInfo( id, name, param_value_size, info, nullptr);
            if( ocl_err != CL_SUCCESS)
            {
                std::cerr << "Failed to find OpenCL device info " << str << "." << std::endl;
                return;
            }

            switch( name)
            {
                case CL_DEVICE_TYPE:
                {
                    std::string device_type;

                    appendBitfield<cl_device_type>( *(reinterpret_cast<cl_device_type *> (info)), CL_DEVICE_TYPE_CPU,         __To_String(CL_DEVICE_TYPE_CPU),         device_type);
                    appendBitfield<cl_device_type>( *(reinterpret_cast<cl_device_type *> (info)), CL_DEVICE_TYPE_GPU,         __To_String(CL_DEVICE_TYPE_GPU),         device_type);
                    appendBitfield<cl_device_type>( *(reinterpret_cast<cl_device_type *> (info)), CL_DEVICE_TYPE_ACCELERATOR, __To_String(CL_DEVICE_TYPE_ACCELERATOR), device_type);
                    appendBitfield<cl_device_type>( *(reinterpret_cast<cl_device_type *> (info)), CL_DEVICE_TYPE_DEFAULT,     __To_String(CL_DEVICE_TYPE_DEFAULT),     device_type);

                    std::cout << "\t\t" << str << ": \t\t" << device_type << std::endl;
                }
                break;

                case CL_DEVICE_SINGLE_FP_CONFIG:    //floating-point config
                {
                    std::string fp_type;

                    appendBitfield<cl_device_fp_config>( *(reinterpret_cast<cl_device_fp_config *> (info)), CL_FP_DENORM          , __To_String(CL_FP_DENORM)          , fp_type);
                    appendBitfield<cl_device_fp_config>( *(reinterpret_cast<cl_device_fp_config *> (info)), CL_FP_INF_NAN         , __To_String(CL_FP_INF_NAN)         , fp_type);
                    appendBitfield<cl_device_fp_config>( *(reinterpret_cast<cl_device_fp_config *> (info)), CL_FP_ROUND_TO_NEAREST, __To_String(CL_FP_ROUND_TO_NEAREST), fp_type);
                    appendBitfield<cl_device_fp_config>( *(reinterpret_cast<cl_device_fp_config *> (info)), CL_FP_ROUND_TO_ZERO   , __To_String(CL_FP_ROUND_TO_ZERO)   , fp_type);
                    appendBitfield<cl_device_fp_config>( *(reinterpret_cast<cl_device_fp_config *> (info)), CL_FP_ROUND_TO_INF    , __To_String(CL_FP_ROUND_TO_INF)    , fp_type);
                    appendBitfield<cl_device_fp_config>( *(reinterpret_cast<cl_device_fp_config *> (info)), CL_FP_FMA             , __To_String(CL_FP_FMA)             , fp_type);
                    appendBitfield<cl_device_fp_config>( *(reinterpret_cast<cl_device_fp_config *> (info)), CL_FP_SOFT_FLOAT      , __To_String(CL_FP_SOFT_FLOAT)      , fp_type);

                    std::cout << "\t\t" << str << ": \t\t" << fp_type << std::endl;

                }
                break;

                case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:
                {
                    std::string memory_type;

                    appendBitfield<cl_device_mem_cache_type>( *(reinterpret_cast<cl_device_mem_cache_type *> (info)), CL_NONE            , __To_String(CL_NONE)            , memory_type);
                    appendBitfield<cl_device_mem_cache_type>( *(reinterpret_cast<cl_device_mem_cache_type *> (info)), CL_READ_ONLY_CACHE , __To_String(CL_READ_ONLY_CACHE) , memory_type);
                    appendBitfield<cl_device_mem_cache_type>( *(reinterpret_cast<cl_device_mem_cache_type *> (info)), CL_READ_WRITE_CACHE, __To_String(CL_READ_WRITE_CACHE), memory_type);

                    std::cout << "\t\t" << str << ": \t\t" << memory_type << std::endl;
                }
                break;

                case CL_DEVICE_LOCAL_MEM_TYPE:
                {
                    std::string memory_type;

                    appendBitfield<cl_device_local_mem_type>( *(reinterpret_cast<cl_device_local_mem_type *> (info)), CL_LOCAL , __To_String(CL_LOCAL) , memory_type);
                    appendBitfield<cl_device_local_mem_type>( *(reinterpret_cast<cl_device_local_mem_type *> (info)), CL_GLOBAL, __To_String(CL_GLOBAL), memory_type);

                    std::cout << "\t\t" << str << ": \t\t" << memory_type << std::endl;
                }
                break;

                case CL_DEVICE_EXECUTION_CAPABILITIES:
                {
                    std::string exec_cap;

                    appendBitfield<cl_device_exec_capabilities>( *(reinterpret_cast<cl_device_exec_capabilities *> (info)), CL_EXEC_KERNEL, __To_String(CL_EXEC_KERNEL) , exec_cap);
                    appendBitfield<cl_device_exec_capabilities>( *(reinterpret_cast<cl_device_exec_capabilities *> (info)), CL_EXEC_NATIVE_KERNEL, __To_String(CL_EXEC_NATIVE_KERNEL), exec_cap);

                    std::cout << "\t\t" << str << ": \t\t" << exec_cap << std::endl;
                }
                break;

                case CL_DEVICE_QUEUE_PROPERTIES:
                {
                    std::string queue_prop;

                    appendBitfield<cl_command_queue_properties>(
                        *(reinterpret_cast<cl_command_queue_properties *> (info)),
                        CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, __To_String(CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE),
                        queue_prop
                    );

                    appendBitfield<cl_command_queue_properties>(
                        *(reinterpret_cast<cl_command_queue_properties *> (info)),
                        CL_QUEUE_PROFILING_ENABLE, __To_String(CL_QUEUE_PROFILING_ENABLE),
                        queue_prop
                    );

                    std::cout << "\t\t" << str << ": \t\t" << queue_prop << std::endl;
                }
                break;

                default:
                    std::cout << "\t\t" << str << ": \t\t" << *info << std::endl;
                break;
            }
        }
};


/**
 * @brief Specialized version of InfoDevice class for array type
 */
template <typename T>
class ArrayType
{
    public:
        static bool isChar()
        {
            return false;
        }
};

    // specialized for char
template <>
class ArrayType<char>
{
    public:
        static bool isChar()
        {
            return true;
        }
};

template<typename T>
class InfoDevice<ArrayType<T>>
{
    public:
        static void display( cl_device_id id, cl_device_info name, std::string str)
        {
            // variable declaration
            cl_int ocl_err;
            std::size_t param_value_size;

            // code
            ocl_err = clGetDeviceInfo( id, name, 0, nullptr, &param_value_size);
            if( ocl_err != CL_SUCCESS)
            {
                std::cerr << "Failed to find OpenCL device info " << str << "." << std::endl;
                return;
            }

            T *info = (T *) alloca( sizeof( T) * param_value_size);
            ocl_err = clGetDeviceInfo( id, name, param_value_size, info, nullptr);
            if( ocl_err != CL_SUCCESS)
            {
                std::cerr << "Failed to find OpenCL device info " << str << "." << std::endl;
                return;
            }

            if( ArrayType<T>::isChar())
            {
                std::cout << "\t\t" << str << ": \t\t" << info << std::endl;
            }
            else if( name == CL_DEVICE_MAX_WORK_ITEM_SIZES)
            {
                cl_uint max_work_item_dimensions;

                ocl_err = clGetDeviceInfo( id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof( cl_uint), &max_work_item_dimensions, nullptr);
                if( ocl_err != CL_SUCCESS)
                {
                    std::cerr << "Failed to find OpenCL device info CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS." << std::endl;
                    return;
                }

                std::cout << "\t\t" << str << ": \t\t";
                for( cl_uint i = 0; i < max_work_item_dimensions; ++i)
                {
                    std::cout << info[i] << " ";
                }
                std::cout << std::endl;
            }
        }
};

/**
 * @brief DisplayPlatformInfo()
 */
static void DisplayPlatformInfo( cl_platform_id id, cl_platform_info name, std::string name_str)
{
    // variable declaration
    cl_int ocl_err;
    std::size_t param_value_size;

    // code
    ocl_err = clGetPlatformInfo( id, name, 0, nullptr, &param_value_size);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Failed to find OpenCL platform " << name_str << "." << std::endl;
        return;
    }

    char *info = (char *) alloca( sizeof( char) * param_value_size);
    ocl_err = clGetPlatformInfo( id, name, param_value_size, info, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Failed to find OpenCL platform " << name_str << "." << std::endl;
        return;
    }

    std::cout << "\t" << name_str << ":\t" << info << std::endl;
}

