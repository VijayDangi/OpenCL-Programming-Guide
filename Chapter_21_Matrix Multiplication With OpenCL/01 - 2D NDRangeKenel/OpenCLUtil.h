
#include <cl/cl.h>

cl_context CreateContext( int platform_used, cl_device_type device_type);
cl_command_queue CreateCommandQueue( cl_context, cl_command_queue_properties command_queue_prop, cl_device_id* );
cl_program CreateProgram( cl_context, cl_device_id, const char* );
