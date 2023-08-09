/**
 * @author : Vijaykumar Dangi
 * @date   : 21-July-2023
 */

#include <iostream>
#include <string>

#include <CL/cl.h>

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
    void DisplayDeviceInfo( cl_device_id, cl_device_info, std::string);

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
                DisplayDeviceInfo( ocl_deviceIds[j], CL_DEVICE_NAME, "CL_DEVICE_NAME");
                DisplayDeviceInfo( ocl_deviceIds[j], CL_DEVICE_VENDOR, "CL_DEVICE_VENDOR");
                DisplayDeviceInfo( ocl_deviceIds[j], CL_DRIVER_VERSION, "CL_DRIVER_VERSION");
                DisplayDeviceInfo( ocl_deviceIds[j], CL_DEVICE_PROFILE, "CL_DEVICE_PROFILE");
                DisplayDeviceInfo( ocl_deviceIds[j], CL_DEVICE_VERSION, "CL_DEVICE_VERSION");
                DisplayDeviceInfo( ocl_deviceIds[j], CL_DEVICE_EXTENSIONS, "CL_DEVICE_EXTENSIONS");

                std::cout << std::endl;
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

/**
 * @brief DisplayDeviceInfo()
 */
void DisplayDeviceInfo( cl_device_id id, cl_device_info name, std::string name_str)
{
    // variable declaration
    cl_int ocl_err;
    std::size_t param_value_size;

    // code
    ocl_err = clGetDeviceInfo( id, name, 0, nullptr, &param_value_size);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Failed to find OpenCL Device " << name_str << "." << std::endl;
        return;
    }

    char *info = (char *) alloca( sizeof( char) * param_value_size);
    ocl_err = clGetDeviceInfo( id, name, param_value_size, info, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Failed to find OpenCL Device " << name_str << "." << std::endl;
        return;
    }

    std::cout << "\t\t" << name_str << ":\t" << info << std::endl;
}
