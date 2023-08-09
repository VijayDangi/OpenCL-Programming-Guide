/**
 * @author : Vijaykumar Dangi
 * @date   : 07-Aug-2023
 */

#include <Windows.h>

#include <iostream>
#include <stdio.h>

#include <fstream>
#include <sstream>

#include "../../Common/glew/include/GL/glew.h"
#include <GL/gl.h>

#include <CL/cl.h>
#include <CL/cl_gl.h>

#define VERTEX_COUNT 256
#define TEXTURE_SIZE 256

#define RELEASE_CL_OBJECT( obj, release_func) \
    if(obj) \
    {   \
        release_func(obj);    \
        obj = nullptr;  \
    }


// Global function declaration
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM);

// Global variable declaration
    //Window
HWND g_hwnd;
HDC g_hdc;
HGLRC g_hrc;

DWORD style = 0;
WINDOWPLACEMENT wpPrev = { sizeof( WINDOWPLACEMENT) };

int g_window_width = 800;
int g_window_height = 600;

bool g_b_active_window = false;
bool g_b_fullscreen = false;

GLuint gl_program;

GLuint gl_vao_line;
GLuint gl_vbo_line;

GLuint gl_vao_quad;
GLuint gl_vbo_quad;

GLuint gl_texture;


cl_context ocl_context = nullptr;
cl_command_queue ocl_command_queue = nullptr;
cl_device_id ocl_device_id = nullptr;
cl_program ocl_program = nullptr;
cl_kernel ocl_sin_kernel = nullptr;
cl_kernel ocl_texture_kernel = nullptr;
cl_mem ocl_gl_vbo_line = nullptr;
cl_mem ocl_gl_texture = nullptr;

bool b_done = false;

/**
 * @brief main()
 */
int main( int argc, char *argv[])
{
    // function declaration
    bool InitializeWindow( HWND *hwnd);
    bool InitializeOpenGL( HWND hwnd, HDC *hdc, HGLRC *hrc);
    bool InitializeOpenCL();
    bool Initialize();

    void Render();
    void Update();

    void Uninitialize();

    // variable declaration
    MSG msg;

    // code
    std::cout << __LINE__ << std::endl;
    if( InitializeWindow( &g_hwnd) == false)
    {
        std::cerr << "InitializeWindow() Failed." << std::endl;
        Uninitialize();
        return 1;
    }

    std::cout << __LINE__ << std::endl;

    if( InitializeOpenGL( g_hwnd, &g_hdc, &g_hrc) == false)
    {
        std::cerr << "InitializeOpenGL() Failed." << std::endl;
        Uninitialize();
        return 1;
    }

    std::cout << __LINE__ << std::endl;

    if( InitializeOpenCL() == false)
    {
        std::cerr << "InitializeOpenCL() Failed." << std::endl;
        Uninitialize();
        return 1;
    }

    std::cout << __LINE__ << std::endl;

    if( Initialize() == false)
    {
        std::cerr << "Initialize() Failed." << std::endl;
        Uninitialize();
        return 1;
    }

    std::cout << __LINE__ << std::endl;

    // Game Loop
    while( b_done == false)
    {
        if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE))
        {
            if( msg.message == WM_QUIT)
            {
                b_done = true;
            }
            else
            {
                TranslateMessage( &msg);
                DispatchMessage( &msg);
            }
        }
        else
        {
            Render();
            Update();
        }
    }

    Uninitialize();

    return 0;
}

/**
 * @brief WndProc()
 */
LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // function declaration
    void Resize( int, int);
    void ToggleFullScreen( void);

    // code
    switch( message)
    {
        case WM_SETFOCUS:
            g_b_active_window = true;
        break;

        case WM_KILLFOCUS:
            g_b_active_window = false;
        break;

        case WM_ERASEBKGND:
        return 0;

        case WM_SIZE:
        {
            g_window_width = LOWORD( lParam);
            g_window_height = HIWORD( lParam);

            Resize( g_window_width, g_window_height);
        }
        break;

        case WM_KEYUP:
        {
            switch( wParam)
            {
                case 'F':
                    ToggleFullScreen();
                break;
            }
        }
        break;

        case WM_DESTROY:
            PostQuitMessage( 0);
        break;
    }

    return DefWindowProc( hwnd, message, wParam, lParam);
}

/**
 * @brief Resize()
 */
void Resize( int width, int height)
{
    // code
    glViewport( 0, 0, width, height);
}

/**
 * @brief ToggleFullScreen()
 */
void ToggleFullScreen( void)
{
    // variable declaration
    MONITORINFO mi = { sizeof( MONITORINFO) };

    // code
    if( g_b_fullscreen)
    {
        SetWindowLong( g_hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement( g_hwnd, &wpPrev);
        SetWindowPos( g_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    else
    {
        style = GetWindowLong( g_hwnd, GWL_STYLE);
        if( style & WS_OVERLAPPEDWINDOW)
        {
            if( GetWindowPlacement( g_hwnd, &wpPrev) && GetMonitorInfo( MonitorFromWindow( g_hwnd, MONITORINFOF_PRIMARY), &mi))
            {
                SetWindowLong( g_hwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos( g_hwnd, HWND_TOP,
                        mi.rcMonitor.left,
                        mi.rcMonitor.top,
                        mi.rcMonitor.right - mi.rcMonitor.left,
                        mi.rcMonitor.bottom - mi.rcMonitor.top,
                        SWP_NOZORDER | SWP_FRAMECHANGED
                );
            }
        }
    }

    g_b_fullscreen = !g_b_fullscreen;
}

/**
 * @brief InitializeWindow()
 */
bool InitializeWindow( HWND *hwnd)
{
    // variable declaration
    WNDCLASSEX wndclass;
    MSG msg;
    TCHAR szClassName[] = TEXT("OpenGL_CL_Interop");
    HINSTANCE hInstance = GetModuleHandle( nullptr);

    // code
    if( !hwnd)
    {
        std::cerr << "Null pointer pass." << std::endl;
        return false;
    }

    std::cout << __LINE__ << std::endl;

    wndclass.cbSize = sizeof( WNDCLASSEX);
    wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance  = hInstance;
    wndclass.hIcon = LoadIcon( nullptr, IDI_APPLICATION);
    wndclass.hIconSm = LoadIcon( nullptr, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor( nullptr, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) GetStockObject( BLACK_BRUSH);
    wndclass.lpszClassName = szClassName;
    wndclass.lpszMenuName = nullptr;
    wndclass.lpfnWndProc = WndProc;

    std::cout << __LINE__ << std::endl;

    if( !RegisterClassEx( &wndclass))
    {
        std::cerr << "Window Class not register." << std::endl;
        return false;
    }

    std::cout << __LINE__ << std::endl;

    *hwnd = CreateWindowEx(
                WS_EX_APPWINDOW,
                szClassName, TEXT("OpenCL-OpenGL Interoperation"),
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
                100, 100, g_window_width, g_window_height,
                nullptr, nullptr, hInstance, nullptr
    );

    std::cout << __LINE__ << std::endl;

    SetForegroundWindow( *hwnd);
    SetFocus( *hwnd);

    std::cout << __LINE__ << std::endl;

    ShowWindow( *hwnd, SW_NORMAL);

    std::cout << __LINE__ << std::endl;
    return true;
}

/**
 * @brief InitializeOpenGL()
 */
bool InitializeOpenGL( HWND hwnd, HDC *p_hdc, HGLRC *p_hglrc)
{
    // variable declaration
    PIXELFORMATDESCRIPTOR pfd;
    int iPixelFormatIndex;
    HDC hdc;
    HGLRC hglrc;

    // code
    if( !p_hdc || !p_hglrc || !hwnd)
    {
        std::cerr << "Invalid Argument." << std::endl;
        return false;
    }

    *p_hdc = nullptr;
    *p_hglrc = nullptr;

    ZeroMemory( &pfd, sizeof( pfd));

        // Initialize PIXELFORMATDESCRIPTION
    pfd.nSize = sizeof( PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cRedBits = 8;
    pfd.cGreenBits = 8;
    pfd.cBlueBits = 8;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 32;

    hdc = GetDC( hwnd);

        // choose pixel format
    iPixelFormatIndex = ChoosePixelFormat( hdc, &pfd);
    if( iPixelFormatIndex == 0)
    {
        std::cerr << "ChoosePixelFormat() Failed." << std::endl;

        ReleaseDC( hwnd, hdc);
        hdc = nullptr;

        return false;
    }

        // Set pixel format
    if( SetPixelFormat( hdc, iPixelFormatIndex, &pfd) == false)
    {
        std::cerr << "SetPixelFormat() Failed." << std::endl;

        ReleaseDC( hwnd, hdc);
        hdc = nullptr;

        return false;
    }

        // get rendering context
    hglrc = wglCreateContext( hdc);
    if( hglrc == nullptr)
    {
        std::cerr << "wglCreateContext() Failed." << std::endl;

        ReleaseDC( hwnd, hdc);
        hdc = nullptr;

        return false;
    }

    if( wglMakeCurrent( hdc, hglrc) == false)
    {
        std::cerr << "wglMakeCurrent() Failed." << std::endl;

        ReleaseDC( hwnd, hdc);
        hdc = nullptr;

        wglDeleteContext( hglrc);
        hglrc = nullptr;

        return false;
    }

        // Initialize GLEW
    GLenum glew_err = glewInit();
    if( glew_err != GLEW_OK)
    {
        std::cerr << "glewInit() Failed." << std::endl;

        ReleaseDC( hwnd, hdc);
        hdc = nullptr;

        wglDeleteContext( hglrc);
        hglrc = nullptr;

        return false;
    }

    *p_hdc = hdc;
    *p_hglrc = hglrc;

    return true;
}

/**
 * @brief InitializeOpenCL()
 */
bool InitializeOpenCL()
{
    // function declaration
    cl_context CreateContext();
    cl_command_queue CreateCommandQueue( cl_context ocl_context, cl_device_id *ocl_device);

    // code
        // create context
    ocl_context = CreateContext();
    if( ocl_context == nullptr)
    {
        std::cerr << "Failed to create OpenCL Context." << std::endl;
        return false;
    }

        // create command-queue
    ocl_command_queue = CreateCommandQueue( ocl_context, &ocl_device_id);
    if( ocl_context == nullptr)
    {
        std::cerr << "CreateCommandQueue()." << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief read_file()
 */
std::string read_file( const char *file_name)
{
    // code
    std::ifstream file( file_name, std::ios::in);
    if( file.is_open() == false)
    {
        std::cerr << "Failed to open file." << std::endl;
        return std::string("");
    }

    std::ostringstream oss;
    oss << file.rdbuf();

    file.close();

    return oss.str();
}


/**
 * @brief CreateContext()
 */
cl_context CreateContext()
{
    // variable declaration
    cl_int ocl_err;
    cl_uint ocl_nuum_platforms;
    cl_platform_id ocl_first_plarform_id;
    cl_context ocl_context = nullptr;

    // code
        // get first platform
    ocl_err = clGetPlatformIDs( 1, &ocl_first_plarform_id, &ocl_nuum_platforms);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clGetPlafotmIDs() Failed ( " << ocl_err << ")." << std::endl;
        return nullptr;
    }

        // OpenCL context properties compatible with OpenGL context
    cl_context_properties ocl_properties[] =
    {
        CL_CONTEXT_PLATFORM, ( cl_context_properties) ocl_first_plarform_id,
        CL_GL_CONTEXT_KHR, ( cl_context_properties) wglGetCurrentContext(),
        CL_WGL_HDC_KHR, ( cl_context_properties) wglGetCurrentDC(),
        0
    };

    ocl_context = clCreateContextFromType( ocl_properties, CL_DEVICE_TYPE_GPU, nullptr, nullptr, &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cout << "Could not create GPU context, trying CPU..." << std::endl;

        ocl_context = clCreateContextFromType( ocl_properties, CL_DEVICE_TYPE_CPU, nullptr, nullptr, &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            std::cerr << "Failed to create an OpenCL GPU or CPU context." << std::endl;
            return nullptr;
        }
    }

    return ocl_context;
}

/**
 * @brief CreateCommandQueue()
 */
cl_command_queue CreateCommandQueue( cl_context ocl_context, cl_device_id *ocl_device)
{
    // variable declaration
    cl_int ocl_err;
    cl_device_id *ocl_devices = nullptr;
    cl_command_queue ocl_command_queue = nullptr;
    size_t device_buffer_size = -1;

    // code
    ocl_err = clGetContextInfo( ocl_context, CL_CONTEXT_DEVICES, 0, nullptr, &device_buffer_size);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clGetContextInfO() Failed ( " << ocl_err << " )." << std::endl;
        return nullptr;
    }

    if( device_buffer_size <= 0)
    {
        std::cerr << "No devices available." << std::endl;
        return nullptr;
    }

        // Allocate memory for the devices buffer
    ocl_devices = new cl_device_id[ device_buffer_size / sizeof( cl_device_id)];
    ocl_err = clGetContextInfo( ocl_context, CL_CONTEXT_DEVICES, device_buffer_size, ocl_devices, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clGetContextInfO() Failed ( " << ocl_err << " )." << std::endl;

        delete ocl_devices;
        ocl_devices = nullptr;

        return nullptr;
    }

        // create command queue
    ocl_command_queue = clCreateCommandQueue( ocl_context, ocl_devices[0], 0, &ocl_err);
    if( (ocl_err != CL_SUCCESS) || (ocl_command_queue == nullptr))
    {
        std::cerr << "clCreateCommandQueue() Failed ( " << ocl_err << " )." << std::endl;

        delete ocl_devices;
        ocl_devices = nullptr;

        return nullptr;
    }

    *ocl_device = ocl_devices[0];

    delete ocl_devices;
    ocl_devices = nullptr;

    return ocl_command_queue;
}

/**
 * @brief CreateOpenCLProgram()
 */
cl_program CreateOpenCLProgram( cl_context ocl_context, cl_device_id ocl_device, const char *file_name)
{
    // variable declaration
    cl_int ocl_err;
    cl_program ocl_program = nullptr;

    // code
    std::string program_string = read_file( file_name);
    if( program_string.empty())
    {
        std::cerr << "OpenCL program read failed." << std::endl;
        return nullptr;
    }

    const char *program_source = program_string.c_str();

    ocl_program = clCreateProgramWithSource( ocl_context, 1, (const char **)&program_source, nullptr, nullptr);
    if( ocl_program == nullptr)
    {
        std::cerr << "clCreateProgramWithSource() Failed." << std::endl;
        return nullptr;
    }

    ocl_err = clBuildProgram( ocl_program, 0, nullptr, nullptr, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        // Determine the reason for the error
        size_t log_size = 0;
        clGetProgramBuildInfo( ocl_program, ocl_device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);

        if( log_size > 0)
        {
            char *build_log = new char[log_size + 1];
            
            clGetProgramBuildInfo( ocl_program, ocl_device, CL_PROGRAM_BUILD_LOG, sizeof( build_log), build_log, nullptr);
            std::cerr << "Error in Program: " << std::endl;
            std::cerr << build_log;

            delete build_log;
        }
        else
        {
            std::cerr << "Error in Program" << std::endl;
        }

        RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);

        return nullptr;
    }

    return ocl_program;
}

/**
 * @brief Initialize()
 */
bool Initialize()
{
    // code
        // Create OpenGL program
    std::string vertex_shader_source = read_file("display_shader/vertex_shader.glsl");
    if( vertex_shader_source.empty())
    {
        std::cerr << "Vertex Shader File read failed." << std::endl;
        return false;
    }

    std::string fragment_shader_source = read_file("display_shader/fragment_shader.glsl");
    if( fragment_shader_source.empty())
    {
        std::cerr << "Fragment Shader File read failed." << std::endl;
        return false;
    }

    GLint status;

        // Vertex Shader
    GLuint vertex_shader_object = glCreateShader( GL_VERTEX_SHADER);
    const char *vertex_source = vertex_shader_source.c_str();
    glShaderSource( vertex_shader_object, 1, (const char **) &vertex_source, nullptr);
    glCompileShader( vertex_shader_object);
    
    glGetShaderiv( vertex_shader_object, GL_COMPILE_STATUS, &status);
    if( status == GL_FALSE)
    {
        GLint log_length = 0;
        glGetShaderiv( vertex_shader_object, GL_INFO_LOG_LENGTH, &log_length);
        if( log_length > 0)
        {
            char *log = new char[log_length];
            glGetShaderInfoLog( vertex_shader_object, log_length, nullptr, log);

            std::cerr << "Vertex Shader Compile Log: " << log << std::endl;

            delete log;

            glDeleteShader( vertex_shader_object);
            vertex_shader_object = 0;
        }
        std::cerr << "Vertex Shader Compile Error" << std::endl;

        return false;
    }

        // Fragment Shader
    GLuint fragment_shader_object = glCreateShader( GL_FRAGMENT_SHADER);
    const char *fragment_source = fragment_shader_source.c_str();
    glShaderSource( fragment_shader_object, 1, (const char **) &fragment_source, nullptr);
    glCompileShader( fragment_shader_object);
    
    glGetShaderiv( fragment_shader_object, GL_COMPILE_STATUS, &status);
    if( status == GL_FALSE)
    {
        GLint log_length = 0;
        glGetShaderiv( fragment_shader_object, GL_INFO_LOG_LENGTH, &log_length);
        if( log_length > 0)
        {
            char *log = new char[log_length];
            glGetShaderInfoLog( fragment_shader_object, log_length, nullptr, log);

            std::cerr << "Fragment Shader Compile Log: " << log << std::endl;

            delete log;

            glDeleteShader( vertex_shader_object);
            vertex_shader_object = 0;

            glDeleteShader( fragment_shader_object);
            fragment_shader_object = 0;
        }
        std::cerr << "Fragment Shader Compile Error" << std::endl;

        return false;
    }


        // Program
    gl_program = glCreateProgram();
    glAttachShader( gl_program, vertex_shader_object);
    glAttachShader( gl_program, fragment_shader_object);

        // Link program
    glLinkProgram( gl_program);

    glGetProgramiv( gl_program, GL_LINK_STATUS, &status);
    if( status == GL_FALSE)
    {
        GLint log_length = 0;
        glGetProgramiv( gl_program, GL_INFO_LOG_LENGTH, &log_length);
        if( log_length > 0)
        {
            char *log = new char[log_length];
            glGetProgramInfoLog( gl_program, log_length, nullptr, log);

            std::cerr << "Program Link Log: " << log << std::endl;

            delete log;

            glDetachShader( gl_program, vertex_shader_object);
            glDetachShader( gl_program, fragment_shader_object);

            glDeleteProgram( gl_program);
            gl_program = 0;

            glDeleteShader( vertex_shader_object);
            vertex_shader_object = 0;
            
            glDeleteShader( fragment_shader_object);
            fragment_shader_object = 0;
        }
        std::cerr << "Program Link Error." << std::endl;

        return false;
    }

        // wave buffer
    glGenVertexArrays( 1, &gl_vao_line);
    glGenBuffers( 1, &gl_vbo_line);

    glBindVertexArray( gl_vao_line);
    glBindBuffer( GL_ARRAY_BUFFER, gl_vbo_line);
    glBufferData( GL_ARRAY_BUFFER, VERTEX_COUNT * sizeof( float) * 2, nullptr, GL_STREAM_DRAW);

    GLint buffer_size;
    glGetBufferParameteriv( GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffer_size);
    if( buffer_size != (VERTEX_COUNT * sizeof( float) * 2))
    {
        std::cerr << "Vertex buffer object has incorrect size." << std::endl;
        return false;
    }

    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray( 0);
    glBindBuffer( GL_ARRAY_BUFFER, 0);
    glBindVertexArray( 0);

        // quad buffer
    GLfloat quad_position[] = 
    {
         1.0f,  1.0f,
        -1.0f,  1.0f,
        -1.0f, -1.0f,

         1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f, -1.0f,
    };

    glGenVertexArrays( 1, &gl_vao_quad);
    glGenBuffers( 1, &gl_vbo_quad);

    glBindVertexArray( gl_vao_quad);
    glBindBuffer( GL_ARRAY_BUFFER, gl_vbo_quad);
    glBufferData( GL_ARRAY_BUFFER, sizeof( quad_position), quad_position, GL_STATIC_DRAW);

    buffer_size = 0;
    glGetBufferParameteriv( GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffer_size);
    if( buffer_size != sizeof( quad_position))
    {
        std::cerr << "Vertex buffer object has incorrect size." << std::endl;
        return false;
    }
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray( 0);
    glBindBuffer( GL_ARRAY_BUFFER, 0);
    glBindVertexArray( 0);

        // texture
    glGenTextures( 1, &gl_texture);
    glBindTexture( GL_TEXTURE_2D, gl_texture);
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_SIZE, TEXTURE_SIZE, 0, GL_RGBA, GL_FLOAT, nullptr);
        glGenerateMipmap( GL_TEXTURE_2D);
        std::cout << glGetError() << std::endl;
    glBindTexture( GL_TEXTURE_2D, 0);

    /***********************************************************************/
    cl_int ocl_err;

        // OpenCL program
    ocl_program = CreateOpenCLProgram( ocl_context, ocl_device_id, "CLProgram.cl");
    if( ocl_program == nullptr)
    {
        std::cerr << "CreateOpenCLProgram() Failed." << std::endl;
        return false;
    }

        // OpenCL sinwave kernel
    ocl_sin_kernel = clCreateKernel( ocl_program, "init_vbo_kernel", nullptr);
    if( ocl_sin_kernel == nullptr)
    {
        std::cerr << "clCreateKernel() Failed." << std::endl;
        return false;
    }

        // OpenCL texture kernel
    ocl_texture_kernel = clCreateKernel( ocl_program, "init_texture_kernel", nullptr);
    if( ocl_texture_kernel == nullptr)
    {
        std::cerr << "clCreateKernel() Failed." << std::endl;
        return false;
    }

        // OpenCL member buffer
    ocl_gl_vbo_line = clCreateFromGLBuffer( ocl_context, CL_MEM_READ_WRITE, gl_vao_line, &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clCreateFromGLBuffer() Failed." << std::endl;
        return false;
    }

    ocl_gl_texture = clCreateFromGLTexture2D( ocl_context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, gl_texture, &ocl_err);
    if( (ocl_err != CL_SUCCESS) || (ocl_gl_texture == nullptr))
    {
        std::cerr << "clCreateFromGLTexture2D() Failed." << std::endl;
        return false;
    }

        /**************************************/
        // perform queries
    std::cout << "Performing queries on OpenGL objects:" << std::endl;

    cl_gl_object_type ocl_gl_object_type;
    GLuint obj_name;

    ocl_err = clGetGLObjectInfo( ocl_gl_texture, &ocl_gl_object_type, &obj_name);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Failed to get object information" << std::endl;
    }
    else if( ocl_gl_object_type == CL_GL_OBJECT_TEXTURE2D)
    {
        std::cout << "Queried a texture object successfully." << std::endl;
        std::cout << "Object name is : " << obj_name << std::endl;
    }


    GLenum param;
    size_t param_ret_size;

    ocl_err = clGetGLTextureInfo( ocl_gl_texture, CL_GL_TEXTURE_TARGET, sizeof( param), &param, &param_ret_size);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Failed to get texture information" << std::endl;
    }
    else if( param == GL_TEXTURE_2D)
    {
        std::cout << "Texture rectangle is being used." << std::endl;
    }

        //

    return true;
}

/**
 * @brief Initialize()
 */
void Render()
{
    // code
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f);
    glClear( GL_COLOR_BUFFER_BIT );

    glUseProgram( gl_program);
        glUniform4f( glGetUniformLocation( gl_program, "u_color"), 0.0f, 0.0f, 1.0f, 1.0f);

        glActiveTexture( GL_TEXTURE0);
        glBindTexture( GL_TEXTURE_2D, gl_texture);
        glUniform1i( glGetUniformLocation( gl_program, "u_texture_d"), 0);

        glUniform1i( glGetUniformLocation( gl_program, "u_use_texture"), 1);
        
        glBindVertexArray( gl_vao_quad);
            glDrawArrays( GL_TRIANGLES, 0, 6);
        glBindVertexArray( 0);

        glBindTexture( GL_TEXTURE_2D, 0);


        glUniform1i( glGetUniformLocation( gl_program, "u_use_texture"), 0);
        glUniform4f( glGetUniformLocation( gl_program, "u_color"), 0.0f, 1.0f, 1.0f, 1.0f);
        glBindVertexArray( gl_vao_line);
            glDrawArrays( GL_LINE_STRIP, 0, VERTEX_COUNT);
        glBindVertexArray( 0);
    glUseProgram( 0);

    SwapBuffers( g_hdc);
}

/**
 * @brief ComputeVBOLine()
 */
void ComputeVBOLine()
{
    // variable declaration
    cl_int ocl_err;
    static cl_int seq = 0;

    // code
    seq = ( seq + 1) % VERTEX_COUNT;

    cl_int size = VERTEX_COUNT;

    ocl_err = clSetKernelArg( ocl_sin_kernel, 0, sizeof( cl_mem), &ocl_gl_vbo_line);
    ocl_err |= clSetKernelArg( ocl_sin_kernel, 1, sizeof( cl_int), &size);
    ocl_err |= clSetKernelArg( ocl_sin_kernel, 2, sizeof( cl_int), &size);
    ocl_err |= clSetKernelArg( ocl_sin_kernel, 3, sizeof( cl_int), &seq);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Error Setting kernel arguments." << __LINE__ << std::endl;

        b_done = true;

        return;
    }

    size_t global_work_size[1] = { VERTEX_COUNT};
    size_t local_work_size[1] = { 16};

    // Acquire the GL object
    // Note, we should ensure GL is complete with any commands that might affect this VBO before we issue OpenCL commands.
    glFinish();

    ocl_err = clEnqueueAcquireGLObjects( ocl_command_queue, 1, &ocl_gl_vbo_line, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueAcquireGLObjects() Failed." << std::endl;
        b_done = true;

        return;
    }

    //execute curbel
    ocl_err = clEnqueueNDRangeKernel( ocl_command_queue, ocl_sin_kernel, 1, nullptr, global_work_size, local_work_size, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueNDRangeKernel() Failed." << std::endl;
        b_done = true;

        return;
    }

    // Acquire the GL object
    // Note, we should ensure OpenCL is finished with any commands that might affect the VBO
    ocl_err = clEnqueueReleaseGLObjects( ocl_command_queue, 1, &ocl_gl_vbo_line, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueReleaseGLObjects() Failed." << std::endl;
        b_done = true;

        return;
    }

    clFinish( ocl_command_queue);
}

/**
 * @brief ComputeTexture()
 */
void ComputeTexture()
{
    // variable declaration
    cl_int ocl_err;
    static cl_int seq = 0;

    // code
    seq = ( seq + 1) % ( TEXTURE_SIZE * 2);

    cl_int size = TEXTURE_SIZE;

    ocl_err = clSetKernelArg( ocl_texture_kernel, 0, sizeof( cl_mem), &ocl_gl_texture);
    ocl_err |= clSetKernelArg( ocl_texture_kernel, 1, sizeof( cl_int), &size);
    ocl_err |= clSetKernelArg( ocl_texture_kernel, 2, sizeof( cl_int), &size);
    ocl_err |= clSetKernelArg( ocl_texture_kernel, 3, sizeof( cl_int), &seq);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Error Setting kernel arguments." << __LINE__ << std::endl;

        b_done = true;

        return;
    }

    size_t global_work_size[2] = { TEXTURE_SIZE, TEXTURE_SIZE};
    size_t local_work_size[2] = { 32, 4};

    // Acquire the GL object
    // Note, we should ensure GL is complete with any commands that might affect this VBO before we issue OpenCL commands.
    glFinish();

    ocl_err = clEnqueueAcquireGLObjects( ocl_command_queue, 1, &ocl_gl_texture, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueAcquireGLObjects() Failed." << std::endl;
        b_done = true;

        return;
    }

    //execute curbel
    ocl_err = clEnqueueNDRangeKernel( ocl_command_queue, ocl_texture_kernel, 2, nullptr, global_work_size, local_work_size, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueNDRangeKernel() Failed." << std::endl;
        b_done = true;

        return;
    }

    // Acquire the GL object
    // Note, we should ensure OpenCL is finished with any commands that might affect the VBO
    ocl_err = clEnqueueReleaseGLObjects( ocl_command_queue, 1, &ocl_gl_texture, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueReleaseGLObjects() Failed." << std::endl;
        b_done = true;

        return;
    }

    clFinish( ocl_command_queue);
}

/**
 * @brief Initialize()
 */
void Update()
{
    // code
    ComputeTexture();
    ComputeVBOLine();
}

/**
 * Uninitialize()
 */
void Uninitialize()
{
    // code
        // *************************** OpenCL
    RELEASE_CL_OBJECT( ocl_context, clReleaseContext);
    RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
    RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
    RELEASE_CL_OBJECT( ocl_sin_kernel, clReleaseKernel);
    RELEASE_CL_OBJECT( ocl_texture_kernel, clReleaseKernel);
    RELEASE_CL_OBJECT( ocl_gl_vbo_line, clReleaseMemObject);
    RELEASE_CL_OBJECT( ocl_gl_texture, clReleaseMemObject);

        // *************************** OpenGL
    if( gl_program)
    {
        glUseProgram( gl_program);
            GLsizei shader_count;
            GLsizei actual_shader_count;

            glGetProgramiv( gl_program, GL_ATTACHED_SHADERS, &shader_count);
            GLuint *p_shader = new GLuint[ shader_count];

            glGetAttachedShaders( gl_program, shader_count, &actual_shader_count, p_shader);

            for( int i = 0; i < shader_count; ++i)
            {
                glDetachShader( gl_program, p_shader[i]);
                glDeleteShader( p_shader[i]);
            }

            glDeleteProgram( gl_program);
            gl_program = 0;
        glUseProgram( 0);

        delete p_shader;
        p_shader = nullptr;
    }

    glDeleteVertexArrays( 1, &gl_vao_line);
    gl_vao_line = 0;

    glDeleteBuffers( 1, &gl_vbo_line);
    gl_vbo_line = 0;

    glDeleteVertexArrays( 1, &gl_vao_quad);
    gl_vao_quad = 0;

    glDeleteBuffers( 1, &gl_vbo_quad);
    gl_vbo_quad = 0;

    glDeleteTextures( 1, &gl_texture);
    gl_texture = 0;


    wglMakeCurrent( nullptr, nullptr);
    
    if( g_hrc)
    {
        wglDeleteContext( g_hrc);
        g_hrc = nullptr;
    }

        // ************************** Window
    if( g_b_fullscreen)
    {
        ToggleFullScreen();
    }
    
    if( g_hdc)
    {
        ReleaseDC( g_hwnd, g_hdc);
        g_hdc = nullptr;
    }

    if( g_hwnd)
    {
        DestroyWindow( g_hwnd);
        g_hwnd = nullptr;
    }
}
