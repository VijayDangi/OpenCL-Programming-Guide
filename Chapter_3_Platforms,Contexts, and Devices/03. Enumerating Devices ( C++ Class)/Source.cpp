/**
 * @author : Vijaykumar Dangi
 * @date   : 21-July-2023
 */

#include <iostream>
#include <string>

#include <CL/cl.h>

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
 * @brief main() : Entry-point function
 */
int main( int argc, char **argv)
{
    // function declaration
    void displayInfo( void);

    // code
    displayInfo();

    return 0;
}

/**
 * @brief displayInfo()
 */
void displayInfo( void)
{
    // function declaration
    void DisplayPlatformInfo( cl_platform_id, cl_platform_info, std::string);

    // variable declaration
    cl_int ocl_err;
    cl_uint ocl_num_platforms;
    cl_platform_id *ocl_platformIds;
    cl_context ocl_context = nullptr;

    // code
        // First, query the total number of platforms
    ocl_err = clGetPlatformIDs( 0, nullptr, &ocl_num_platforms);
    if( (ocl_err != CL_SUCCESS) || ( ocl_num_platforms <= 0))
    {
        std::cerr << "Failed to find any OpenCL platform." << std::endl;
        return;
    }

        // Next, allocate memory for the installed platforms, and query to get the list
    ocl_platformIds = ( cl_platform_id *) alloca( sizeof( cl_platform_id) * ocl_num_platforms);

        // Get all platforms
    ocl_err = clGetPlatformIDs( ocl_num_platforms, ocl_platformIds, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Failed to find any OpenCL platforms." << std::endl;
        return;
    }

    std::cout << "Number of platforms: \t" << ocl_num_platforms << std::endl;

        // Iterate through the list of platforms displaying associated information
    for( cl_uint i = 0; i < ocl_num_platforms; ++i)
    {
            // First we display information associated with the platform
        DisplayPlatformInfo( ocl_platformIds[i], CL_PLATFORM_PROFILE, "CL_PLATFORM_PROFILE");
        DisplayPlatformInfo( ocl_platformIds[i], CL_PLATFORM_VERSION, "CL_PLATFORM_VERSION");
        DisplayPlatformInfo( ocl_platformIds[i], CL_PLATFORM_VENDOR, "CL_PLATFORM_VENDOR");
        DisplayPlatformInfo( ocl_platformIds[i], CL_PLATFORM_EXTENSIONS, "CL_PLATFORM_EXTENSIONS");

        std::cout << "\n\n";

        cl_uint ocl_num_devices = 0;
        cl_device_id *ocl_deviceIds;

            // Get number of devices
        ocl_err = clGetDeviceIDs( ocl_platformIds[i], CL_DEVICE_TYPE_ALL, 0, nullptr, &ocl_num_devices);
        if( ocl_num_devices < 1)
        {
            std::cout << "\tNo device found for Device." << std::endl;
        }
        else
        {
                // Next, allocate memory for the available devices, and query to get the list
            ocl_deviceIds = ( cl_device_id *) alloca( sizeof( cl_device_id) * ocl_num_devices);

                // Get all devices
            ocl_err = clGetDeviceIDs( ocl_platformIds[i], CL_DEVICE_TYPE_ALL, ocl_num_devices, ocl_deviceIds, nullptr);
            if( ocl_err != CL_SUCCESS)
            {
                std::cerr << "Failed to find any OpenCL device." << std::endl;
                return;
            }

            std::cout << "\tNumber of Devices: \t" << ocl_num_devices << std::endl;

            std::cout << "\tDevices: " << std::endl;

            for( cl_uint j = 0; j < ocl_num_devices; ++j)
            {
                InfoDevice< ArrayType<char>>::display( ocl_deviceIds[j],    CL_DEVICE_NAME, __To_String(CL_DEVICE_NAME));
                InfoDevice< ArrayType<char>>::display( ocl_deviceIds[j],  CL_DEVICE_VENDOR, __To_String(CL_DEVICE_VENDOR));
                InfoDevice< ArrayType<char>>::display( ocl_deviceIds[j], CL_DRIVER_VERSION, __To_String(CL_DRIVER_VERSION));
                InfoDevice< ArrayType<char>>::display( ocl_deviceIds[j], CL_DEVICE_PROFILE, __To_String(CL_DEVICE_PROFILE));
                InfoDevice< ArrayType<char>>::display( ocl_deviceIds[j], CL_DEVICE_VERSION, __To_String(CL_DEVICE_VERSION));

                InfoDevice<cl_device_type>::display( ocl_deviceIds[j], CL_DEVICE_TYPE, __To_String(CL_DEVICE_TYPE));

                std::cout << std::endl;

                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_VENDOR_ID, __To_String(CL_DEVICE_VENDOR_ID));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_MAX_COMPUTE_UNITS, __To_String(CL_DEVICE_MAX_COMPUTE_UNITS));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, __To_String(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS));
                InfoDevice<ArrayType<size_t>>::display( ocl_deviceIds[j], CL_DEVICE_MAX_WORK_ITEM_SIZES, __To_String(CL_DEVICE_MAX_WORK_ITEM_SIZES));
                InfoDevice<size_t>::display( ocl_deviceIds[j], CL_DEVICE_MAX_WORK_GROUP_SIZE, __To_String(CL_DEVICE_MAX_WORK_GROUP_SIZE));
                
                std::cout << std::endl;

                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR,   __To_String(CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT,  __To_String(CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT,    __To_String(CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG,   __To_String(CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,  __To_String(CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, __To_String(CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF,   __To_String(CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF));

                std::cout << std::endl;

                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR,   __To_String(CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT,  __To_String(CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_INT,    __To_String(CL_DEVICE_NATIVE_VECTOR_WIDTH_INT));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG,   __To_String(CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT,  __To_String(CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, __To_String(CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF,   __To_String(CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF));

                std::cout << std::endl;

                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_MAX_CLOCK_FREQUENCY, __To_String(CL_DEVICE_MAX_CLOCK_FREQUENCY));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_ADDRESS_BITS, __To_String(CL_DEVICE_ADDRESS_BITS));
                InfoDevice<cl_ulong>::display( ocl_deviceIds[j], CL_DEVICE_MAX_MEM_ALLOC_SIZE, __To_String(CL_DEVICE_MAX_MEM_ALLOC_SIZE));

                std::cout << std::endl;

                InfoDevice<cl_bool>::display( ocl_deviceIds[j], CL_DEVICE_IMAGE_SUPPORT, __To_String(CL_DEVICE_IMAGE_SUPPORT));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_MAX_READ_IMAGE_ARGS, __To_String(CL_DEVICE_MAX_READ_IMAGE_ARGS));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_MAX_WRITE_IMAGE_ARGS, __To_String(CL_DEVICE_MAX_WRITE_IMAGE_ARGS));
                InfoDevice<size_t>::display( ocl_deviceIds[j], CL_DEVICE_IMAGE2D_MAX_WIDTH,  __To_String(CL_DEVICE_IMAGE2D_MAX_WIDTH));
                InfoDevice<size_t>::display( ocl_deviceIds[j], CL_DEVICE_IMAGE2D_MAX_HEIGHT, __To_String(CL_DEVICE_IMAGE2D_MAX_HEIGHT));
                InfoDevice<size_t>::display( ocl_deviceIds[j], CL_DEVICE_IMAGE3D_MAX_WIDTH,  __To_String(CL_DEVICE_IMAGE3D_MAX_WIDTH));
                InfoDevice<size_t>::display( ocl_deviceIds[j], CL_DEVICE_IMAGE3D_MAX_HEIGHT, __To_String(CL_DEVICE_IMAGE3D_MAX_HEIGHT));
                InfoDevice<size_t>::display( ocl_deviceIds[j], CL_DEVICE_IMAGE3D_MAX_DEPTH,  __To_String(CL_DEVICE_IMAGE3D_MAX_DEPTH));

                std::cout << std::endl;

                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_MAX_SAMPLERS, __To_String(CL_DEVICE_MAX_SAMPLERS));
                InfoDevice<size_t>::display( ocl_deviceIds[j], CL_DEVICE_MAX_PARAMETER_SIZE, __To_String(CL_DEVICE_MAX_PARAMETER_SIZE));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_MEM_BASE_ADDR_ALIGN, __To_String(CL_DEVICE_MEM_BASE_ADDR_ALIGN));
                InfoDevice<cl_uint>::display( ocl_deviceIds[j], CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, __To_String(CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE));

                std::cout << std::endl;

                InfoDevice<cl_device_fp_config>::display( ocl_deviceIds[j], CL_DEVICE_SINGLE_FP_CONFIG, __To_String(CL_DEVICE_SINGLE_FP_CONFIG));
                InfoDevice<cl_device_mem_cache_type>::display( ocl_deviceIds[j], CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, __To_String(CL_DEVICE_GLOBAL_MEM_CACHE_TYPE));
                InfoDevice<cl_uint>:: display( ocl_deviceIds[j], CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE,  __To_String(CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE));
                InfoDevice<cl_ulong>::display( ocl_deviceIds[j], CL_DEVICE_GLOBAL_MEM_CACHE_SIZE,      __To_String(CL_DEVICE_GLOBAL_MEM_CACHE_SIZE));
                InfoDevice<cl_ulong>::display( ocl_deviceIds[j], CL_DEVICE_GLOBAL_MEM_SIZE,            __To_String(CL_DEVICE_GLOBAL_MEM_SIZE));
                InfoDevice<cl_ulong>::display( ocl_deviceIds[j], CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE,   __To_String(CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE));
                InfoDevice<cl_uint>:: display( ocl_deviceIds[j], CL_DEVICE_MAX_CONSTANT_ARGS,          __To_String(CL_DEVICE_MAX_CONSTANT_ARGS));
                InfoDevice<cl_device_local_mem_type>::display( ocl_deviceIds[j], CL_DEVICE_LOCAL_MEM_TYPE, __To_String(CL_DEVICE_LOCAL_MEM_TYPE));
                InfoDevice<cl_ulong>::display( ocl_deviceIds[j], CL_DEVICE_LOCAL_MEM_SIZE,             __To_String(CL_DEVICE_LOCAL_MEM_SIZE));
                InfoDevice<cl_bool>:: display( ocl_deviceIds[j], CL_DEVICE_ERROR_CORRECTION_SUPPORT,   __To_String(CL_DEVICE_ERROR_CORRECTION_SUPPORT));
                InfoDevice<cl_bool>:: display( ocl_deviceIds[j], CL_DEVICE_HOST_UNIFIED_MEMORY,        __To_String(CL_DEVICE_HOST_UNIFIED_MEMORY));
                InfoDevice<size_t>::  display( ocl_deviceIds[j], CL_DEVICE_PROFILING_TIMER_RESOLUTION, __To_String(CL_DEVICE_PROFILING_TIMER_RESOLUTION));
                InfoDevice<cl_bool>:: display( ocl_deviceIds[j], CL_DEVICE_ENDIAN_LITTLE,              __To_String(CL_DEVICE_ENDIAN_LITTLE));
                InfoDevice<cl_bool>:: display( ocl_deviceIds[j], CL_DEVICE_AVAILABLE,                  __To_String(CL_DEVICE_AVAILABLE));
                InfoDevice<cl_bool>:: display( ocl_deviceIds[j], CL_DEVICE_COMPILER_AVAILABLE,         __To_String(CL_DEVICE_COMPILER_AVAILABLE));
                InfoDevice<cl_device_exec_capabilities>::display( ocl_deviceIds[j], CL_DEVICE_EXECUTION_CAPABILITIES, __To_String(CL_DEVICE_EXECUTION_CAPABILITIES));
                InfoDevice<cl_command_queue_properties>::display( ocl_deviceIds[j], CL_DEVICE_QUEUE_PROPERTIES,       __To_String(CL_DEVICE_QUEUE_PROPERTIES));
                InfoDevice<cl_platform_id>::display( ocl_deviceIds[j], CL_DEVICE_PLATFORM, __To_String(CL_DEVICE_PLATFORM));

                std::cout << std::endl;

                InfoDevice< ArrayType<char>>::display( ocl_deviceIds[j], CL_DEVICE_EXTENSIONS, __To_String(CL_DEVICE_EXTENSIONS));

                std::cout << "\n\n";
            }
        }

        std::cout << "==========================================" << std::endl;
    }
}

/**
 * @brief DisplayPlatformInfo()
 */
void DisplayPlatformInfo( cl_platform_id id, cl_platform_info name, std::string name_str)
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
