
#include <cl/cl.h>
#include <cl/cl_gl.h>

#define CL_OBJECT_RELEASE( cl_object, release_function) \
        if( cl_object) \
        {   \
            release_function(cl_object);   \
            cl_object = nullptr;   \
        }

namespace OpenCLUtil
{
    bool Initialize();
    void Unintialize();

    bool IsInitialized();

    cl_program CreateProgram( const char* );

    cl_context GetContext();
    cl_command_queue GetCommandQueue();
}
