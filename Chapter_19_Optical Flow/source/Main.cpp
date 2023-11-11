/**
 * @file   OGL.cpp
 * @author Vijaykumar Dangi
 * 
 * @copyright Copyright (c) 2023
 *
 */

//Headers
#include "Common.h"
#include "third_party/glew/include/GL/wglew.h"

#include <stdio.h>
#include <float.h>
#include <vector>

#include "Resource.h"

#include "ExMaths.h"

#include "system/GLView.h"
#include "system/Logger.h"
#include "system/Keyboard.h"

#include "framework/Texture2D.h"
#include "framework/ShaderProgram.h"
#include "framework/Buffer.h"

#include "utility\OpenCLUtil.h"

#include "escapi.h"

//Library
#pragma comment( lib, "user32.lib")
#pragma comment( lib, "gdi32.lib")
#pragma comment( lib, "kernel32.lib")

// #if defined(DEBUG) || defined(_DEBUG)
//     #pragma comment( lib, "glew32d.lib")
// #else
    #pragma comment( lib, "glew32.lib")
// #endif

#pragma comment( lib, "OpenGL32.lib")

//macro
#define ENABLE_LOCK_FPS 1
#define DESIRE_FPS 60

const int CAPTURE_IMAGE_WIDTH = 512;
const int CAPTURE_IMAGE_HEIGHT = 512;

//Global variables
static GLView *glView = nullptr;

unsigned int g_window_width  = CAPTURE_IMAGE_WIDTH;
unsigned int g_window_height = CAPTURE_IMAGE_HEIGHT;

bool g_is_escape_key_pressed = false;

double deltaTime = 0.0;

    // OpenGL
ShaderProgram *pTexture_display_program = nullptr;
Texture2D *pCamTexture[2] = {nullptr};
Texture2D *pVectorLineTexture = nullptr;
VertexArray *pVao = nullptr;

int curr_cap_image = 0;


    // Web Cam
struct SimpleCapParams capture_params;
int capture_device;

    // OpenCL
cl_program ocl_image_processing_program;
cl_kernel ocl_subtract_image_kernel;
cl_kernel ocl_velocity_map_kernel;
cl_kernel ocl_vector_line_kernel;

cl_mem ocl_gl_cam_image[2];     // CL_RGBA  (unsigned char)
cl_mem ocl_subtract_image;      // CL_RED   (unsigned char)
cl_mem ocl_flow_vector_image;   // CL_RG    (float)
cl_mem ocl_gl_vector_image;     // CL_RED   (unsigned char)

const int search_size = 7;
const int patch_size = 9;


#if defined(CONSOLE) || defined(_CONSOLE)
    #pragma comment( linker, "/subsystem:console")
int main( int argc, char *argv[])
{
    return WinMain(
        GetModuleHandle(NULL),
        NULL,
        GetCommandLineA(),
        SW_SHOWDEFAULT);
}
#else
    #pragma comment( linker, "/subsystem:windows")
#endif


/**
 * @brief WinMain()
 */
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInsatnce, LPSTR szCmdLine, int iCmdShow)
{
    //function declarations
    bool Initialize( HWND hwnd);
    void OnResize( unsigned int width, unsigned int height);
    void OnMouseMove( int mouseX, int mouseY, int button);
    void EventCallback( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    int RunGameLoop( MainWindow *window);
    void UnInitialize( void);
    
    //variable declarations
    bool bSuccess = false;
    int ret_val = 0;

    //code
    if(Logger::Initialize( "LogFile.txt") != false)
    {
        MessageBox( NULL, TEXT("Error while creating log file"), TEXT("Error"), MB_OK | MB_TOPMOST | MB_ICONSTOP);
        return( EXIT_FAILURE);
    }
    else
    {
        Log( "Log Started...\n");
    }

        // Initialize ESCAPI (Extremelty Simple Capture API)
    int capture_device_count = setupESCAPI();
    if( capture_device_count < 1)
    {
        Log("Unable to Initialize() ESCAPI.");
        return 1;
    }
        // Get first capture device
    capture_device = 0;

        // setup ESCAPI to capture webcam image
    capture_params.mWidth = CAPTURE_IMAGE_WIDTH;
    capture_params.mHeight = CAPTURE_IMAGE_HEIGHT;
    capture_params.mTargetBuf = new int[ CAPTURE_IMAGE_WIDTH * CAPTURE_IMAGE_HEIGHT];
    initCapture( capture_device, &capture_params);

    Log( "ESCAPI Initialize()");


        // Create OpenGL Window
    glView = GLView::CreateGLView( hInstance, g_window_width, g_window_height, 4, 6, 0, MAKEINTRESOURCE( IDI_MYICON));
    if( !glView)
    {
        Log("GLView::CreateGLView() Creation Failed.");
        return 1;
    }

        // register callback
    glView->RegisterOnMouseMoveCallback( OnMouseMove);
    glView->RegisterOnResizeCallback( OnResize);
    glView->RegisterOnWindowCallback( EventCallback);

        // show window
    glView->ShowWindow();
    glView->MakeCurrentContext();
    bSuccess = Initialize( glView->GetHandle());


    if( bSuccess)
    {
        ret_val = RunGameLoop( glView);
    }


    if( capture_params.mTargetBuf)
    {
        delete[] capture_params.mTargetBuf;
        capture_params.mTargetBuf = nullptr;
    }
    UnInitialize();

    return( ret_val);
}

/**
 * @brief RunGameLoop()
 */
int RunGameLoop( MainWindow *window)
{
    // function declaration
    void Display( void);
    void Update( double deltaTime);

    // variable declaration
    MSG   msg;
    bool  bDone = false;

    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER Frequency;

    // code
    QueryPerformanceCounter( &StartTime);
    QueryPerformanceFrequency( &Frequency);
    double oneDivideByFrequency = 1.0 / (double)Frequency.QuadPart;

#if ENABLE_LOCK_FPS
    double timePerFrame = 1.0f / (float)DESIRE_FPS;
#endif

    // GAME LOOP
    while( bDone == false)
    {
        if(PeekMessage( &msg, NULL, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
                bDone = true;
            else
            {
                TranslateMessage( &msg);
                DispatchMessage( &msg);
            }
        }
        else
        {
           if( window->IsWindowActive())
            {
                if(g_is_escape_key_pressed == true)
                    bDone = true;

                Display();

                Update(deltaTime);

                QueryPerformanceCounter( &EndTime);
                deltaTime = (double)(EndTime.QuadPart - StartTime.QuadPart) * oneDivideByFrequency;

#if ENABLE_LOCK_FPS
                while( deltaTime < timePerFrame)
                {
                    QueryPerformanceCounter( &EndTime);
                    deltaTime = (double)(EndTime.QuadPart - StartTime.QuadPart) * oneDivideByFrequency;
                }

                StartTime = EndTime;
#endif
            }
        }
    }

    return (int)msg.wParam;
}

/**
 * @brief OnMouseMove()
 */
void OnMouseMove( int mouseX, int mouseY, int button)
{
}

/**
 * @brief EventCallback()
 */
void EventCallback( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // code
    if( message == WM_DESTROY)
    {
        PostQuitMessage(0);
    }
}


#if defined(DEBUG) || defined(_DEBUG)
/**
 * @brief 
 * @param source 
 * @param type 
 * @param id 
 * @param severity 
 * @param length 
 * @param message 
 * @param userParam 
 * @return 
 */
void GLAPIENTRY GLDebugCallbackFunction( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    // variable declaration
    char *source_str = nullptr;
    char *type_str = nullptr;
    char *severity_str = nullptr;

    // code
    switch( source)
    {
        case GL_DEBUG_SOURCE_API:
            source_str = TO_STRING(API);
        break;

        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            source_str = TO_STRING(WINDOW_SYSTEM);
        break;

        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            source_str = TO_STRING(SHADER_COMPILER);
        break;

        case GL_DEBUG_SOURCE_THIRD_PARTY:
            source_str = TO_STRING(THIRD_PARTY);
        break;

        case GL_DEBUG_SOURCE_APPLICATION:
            source_str = TO_STRING(APPLICATION);
        break;

        case GL_DEBUG_SOURCE_OTHER:
            source_str = TO_STRING(OTHER);
        break;

        default:
            source_str = "UNKNOWN";
        break;
    }


    switch(type)
    {
        case GL_DEBUG_TYPE_ERROR:
            type_str = TO_STRING(ERROR);
        break;

        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            type_str = TO_STRING(DEPRECATED_BEHAVIOR);
        break;

        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            type_str = TO_STRING(UNDEFINED_BEHAVIOR);
        break;

        case GL_DEBUG_TYPE_PERFORMANCE:
            type_str = TO_STRING(PERFORMANCE);
        break;

        case GL_DEBUG_TYPE_PORTABILITY:
            type_str = TO_STRING(PORTABILITY);
        break;

        case GL_DEBUG_TYPE_MARKER:
            type_str = TO_STRING(MARKER);
        break;

        case GL_DEBUG_TYPE_PUSH_GROUP:
            type_str = TO_STRING(PUSH_GROUP);
        break;

        case GL_DEBUG_TYPE_POP_GROUP:
            type_str = TO_STRING(POP_GROUP);
        break;

        case GL_DEBUG_TYPE_OTHER:
            type_str = TO_STRING(OTHER);
        break;

        default:
            type_str = "UNKNOWN";
        break;
    }


    switch(severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            severity_str = TO_STRING( HIGH);
        break;

        case GL_DEBUG_SEVERITY_MEDIUM:
            severity_str = TO_STRING( MEDIUM);
        break;

        case GL_DEBUG_SEVERITY_LOW:
            severity_str = TO_STRING( LOW);
        break;

        case GL_DEBUG_SEVERITY_NOTIFICATION:
            severity_str = TO_STRING( NOTIFICATION);
        break;

        default:
            severity_str = "UNKNOWN";
        break;
    }

    if( severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        return;
    }

    Log(
        "GL Debug Message:\nId: 0x%08X\nSource: %s\nType: %s\nSeverity: %s\nMessage: \"%s\"\n",
        id, source_str, type_str, severity_str, message
    );
}
#endif

/**
 * @brief Initialize()
 */
bool Initialize( HWND hwnd)
{
    //function declarations
    void OnResize( unsigned int, unsigned int);
    void OpenGLLog( void);
    
    //variable declaration
    RECT rc;

    //code
    GetClientRect( hwnd, &rc);
    g_window_width = rc.right - rc.left;
    g_window_height = rc.bottom - rc.top;

    /**_________________ OPENGL INFORMATION _____________________**/
    OpenGLLog();

    int double_buffer_support = 0;
    glGetIntegerv( GL_DOUBLEBUFFER, &double_buffer_support);
    Log("GL_DOUBLEBUFFER: %d", double_buffer_support);

#if defined(DEBUG) || defined(_DEBUG)
    // enable OpenGL debugging
    glEnable( GL_DEBUG_OUTPUT);
    // glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback( GLDebugCallbackFunction, nullptr);
    glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

    /* =============================== OpenCL Initialize =============================== */
    if( OpenCLUtil::Initialize() == false)
    {
        Log( "OpeCLUtil::Initialize() Failed.");
        return false;
    }

    /////////////////////////////////////////////
    pTexture_display_program = new ShaderProgram();
    if( ! pTexture_display_program->AddShaderFromFile( "resource/shaders/texture_display/vertex_shader.glsl", GL_VERTEX_SHADER))
    {
        Log("Vertex Shader Failed.");
        return false;
    }
    
    if( ! pTexture_display_program->AddShaderFromFile( "resource/shaders/texture_display/fragment_shader.glsl", GL_FRAGMENT_SHADER))
    {
        Log("Fragment Shader Failed.");
        return false;
    }

    if( ! pTexture_display_program->Build())
    {
        Log("Program Build() Failed.");
        return false;
    }

    ////////////////////////////////////////////
    pCamTexture[0] = new Texture2D( CAPTURE_IMAGE_WIDTH, CAPTURE_IMAGE_HEIGHT, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE);
    pCamTexture[1] = new Texture2D( CAPTURE_IMAGE_WIDTH, CAPTURE_IMAGE_HEIGHT, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE);
    
    pVectorLineTexture = new Texture2D( CAPTURE_IMAGE_WIDTH, CAPTURE_IMAGE_HEIGHT, GL_RED, GL_RED, GL_UNSIGNED_BYTE);

    pVao = new VertexArray();

    ////////////////////////////////////////////
    ////////////////////////////////////////////
    ////////////////////////////////////////////
    cl_int ocl_err;

    ocl_image_processing_program = OpenCLUtil::CreateProgram("resource/opencl_kernel/image_processing.cl");
    if( ocl_image_processing_program == nullptr)
    {
        Log("OpenCLUtil::CreateProgram() Failed.");
        return false;
    }

    ocl_subtract_image_kernel = clCreateKernel( ocl_image_processing_program, "subtract_image", &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        Log("clCreateKernel(): %d", ocl_err);
        return false;
    }

    ocl_velocity_map_kernel = clCreateKernel( ocl_image_processing_program, "velocity_map", &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        Log("clCreateKernel(): %d", ocl_err);
        return false;
    }

    ocl_vector_line_kernel = clCreateKernel( ocl_image_processing_program, "vector_line", &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        Log("clCreateKernel(): %d", ocl_err);
        return false;
    }

        //////////////
    ocl_gl_cam_image[0] = clCreateFromGLTexture2D( OpenCLUtil::GetContext(), CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, pCamTexture[0]->GetID(), &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        Log("clCreateFromGLTexture2D(): %d", ocl_err);
        return false;
    }

    ocl_gl_cam_image[1] = clCreateFromGLTexture2D( OpenCLUtil::GetContext(), CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, pCamTexture[1]->GetID(), &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        Log("clCreateFromGLTexture2D(): %d", ocl_err);
        return false;
    }

    ocl_gl_vector_image = clCreateFromGLTexture2D( OpenCLUtil::GetContext(), CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, pVectorLineTexture->GetID(), &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        Log("clCreateFromGLTexture2D(): %d", ocl_err);
        return false;
    }


    cl_image_format ocl_image_format;

    ocl_image_format.image_channel_order = CL_R;
    ocl_image_format.image_channel_data_type = CL_UNSIGNED_INT8;

    ocl_subtract_image = clCreateImage2D( OpenCLUtil::GetContext(), CL_MEM_READ_WRITE, &ocl_image_format, CAPTURE_IMAGE_WIDTH, CAPTURE_IMAGE_HEIGHT, 0, nullptr, &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        Log("clCreateFromGLTexture2D(): %d", ocl_err);
        return false;
    }

    ocl_image_format.image_channel_order = CL_RG;
    ocl_image_format.image_channel_data_type = CL_FLOAT;

    ocl_flow_vector_image = clCreateImage2D( OpenCLUtil::GetContext(), CL_MEM_READ_WRITE, &ocl_image_format, CAPTURE_IMAGE_WIDTH, CAPTURE_IMAGE_HEIGHT, 0, nullptr, &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        Log("clCreateFromGLTexture2D(): %d", ocl_err);
        return false;
    }


    /** ______________ OPENGL STATE SET ________________ **/
	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);
    glEnable( GL_DEPTH_TEST);
    
    //glEnable( GL_CULL_FACE);
    
    //warm-up Resize call
    OnResize( g_window_width, g_window_height);

        // Start Capturing Video
    doCapture( capture_device);

    return true;
}

/**
 * @brief OpenGLLog()
 */
void OpenGLLog( void)
{
    //variable delcaration
    FILE *pOpenGLInfoLog = NULL;

    //code
    fopen_s(&pOpenGLInfoLog, "OpenGLInformation.log", "w");
    if(pOpenGLInfoLog == NULL)
    {
        MessageBox( NULL, TEXT("Error while create/open file \"OpenGLInformation.log\""), TEXT("Error"), MB_OK | MB_ICONSTOP );
        g_is_escape_key_pressed = true;
        return;
    }
    
    fprintf(pOpenGLInfoLog, "OpenGL Vendor   : %s\n", glGetString( GL_VENDOR));
    fprintf(pOpenGLInfoLog, "OpenGL Renderer : %s\n", glGetString( GL_RENDERER));
    fprintf(pOpenGLInfoLog, "OpenGL Version  : %s\n", glGetString( GL_VERSION));

    GLint val;
    glGetIntegerv( GL_MAJOR_VERSION, &val);
    fprintf(pOpenGLInfoLog, "OpenGL Major Version Version  : %d\n", val);

    glGetIntegerv( GL_MINOR_VERSION, &val);
    fprintf(pOpenGLInfoLog, "OpenGL Minor Version Version  : %d\n", val);

    fprintf(pOpenGLInfoLog, "Shading Language (GLSL) Version : %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION));
    
        // SSBO
    glGetIntegerv( GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &val);
    fprintf( pOpenGLInfoLog, "\nGL_MAX_SHADER_STORAGE_BUFFER_BINDINGS = %d\n", val);

    glGetIntegerv( GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &val);
    fprintf( pOpenGLInfoLog, "GL_MAX_SHADER_STORAGE_BLOCK_SIZE = %d\n", val);

    glGetIntegerv( GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &val);
    fprintf( pOpenGLInfoLog, "GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS = %d\n", val);

    glGetIntegerv( GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &val);
    fprintf( pOpenGLInfoLog, "GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS = %d\n", val);

    glGetIntegerv( GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS, &val);
    fprintf( pOpenGLInfoLog, "GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS = %d\n", val);

    glGetIntegerv( GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS, &val);
    fprintf( pOpenGLInfoLog, "GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS = %d\n", val);

    glGetIntegerv( GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS, &val);
    fprintf( pOpenGLInfoLog, "GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS = %d\n", val);
    
    glGetIntegerv( GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &val);
    fprintf( pOpenGLInfoLog, "GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS = %d\n", val);

    glGetIntegerv( GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &val);
    fprintf( pOpenGLInfoLog, "GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS = %d\n", val);

        //OpenGL Enabled Extensions
    GLint numExtension;
    glGetIntegerv( GL_NUM_EXTENSIONS, &numExtension);
    
    fprintf(pOpenGLInfoLog, "\nNumber of Enabled Extensions : %d\n", numExtension);
    fprintf(pOpenGLInfoLog, "Enable Extension : \n");
    for(int i = 0; i < numExtension; i++)
    {
        fprintf(pOpenGLInfoLog, "\t%s\n", glGetStringi(GL_EXTENSIONS, i));
    }
    
    fclose(pOpenGLInfoLog);
    pOpenGLInfoLog = NULL;
}

/**
 * @brief OnResize()
 */
void OnResize( unsigned int width, unsigned int height)
{
    //code
    if(height == 0)
        height = 1;


    g_window_width = width;
    g_window_height = height;

    glViewport( 0, 0, (GLfloat)width, (GLfloat)height);
}

/**
 * @brief Display()
 */
void Display( void)
{

    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f);
    glViewport( 0, 0, g_window_width, g_window_height);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable( GL_BLEND);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    pTexture_display_program->Bind();
        pTexture_display_program->SetUniformInt( "u_texture_buffer", 0);
        pTexture_display_program->SetUniformInt( "u_flip_image", 1);

        pVao->Bind();

        pCamTexture[curr_cap_image]->Bind(0);
        pTexture_display_program->SetUniformInt( "u_use_vector_image", 0);
            glDrawArrays( GL_TRIANGLES, 0, 6);
        pCamTexture[curr_cap_image]->Unbind(0);


        pVectorLineTexture->Bind(0);
        pTexture_display_program->SetUniformInt( "u_use_vector_image", 1);
            glDrawArrays( GL_TRIANGLES, 0, 6);
        pVectorLineTexture->Unbind(0);

        pVao->Unbind();
    pTexture_display_program->Unbind();

    if( isCaptureDone( capture_device))
    {
            // swapping
        curr_cap_image = !curr_cap_image;

        pCamTexture[curr_cap_image]->Bind(0);
            glTexSubImage2D(
                GL_TEXTURE_2D, 0,
                0, 0, pCamTexture[curr_cap_image]->GetWidth(), pCamTexture[curr_cap_image]->GetHeight(),
                pCamTexture[curr_cap_image]->GetFormat(), GL_UNSIGNED_BYTE, capture_params.mTargetBuf
            );
        pCamTexture[curr_cap_image]->Unbind(0);


        ///////////////////////////////////////////////////
#define CL_CHECK_ERROR(err) \
        if( err != CL_SUCCESS)  \
        {   \
            Log("CL Error Code: (%d).", err);   \
            return; \
        }

        int args_index = 0;
        cl_int ocl_err;

        cl_command_queue ocl_command_queue = OpenCLUtil::GetCommandQueue();


            // Aquire GL resources
        ocl_err = clEnqueueAcquireGLObjects( ocl_command_queue, 2, ocl_gl_cam_image, 0, nullptr, nullptr);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clEnqueueAcquireGLObjects( ocl_command_queue, 1, &ocl_gl_vector_image, 0, nullptr, nullptr);
        CL_CHECK_ERROR(ocl_err);


            // clean vector line image
        size_t origin[] = { 0, 0, 0};
        size_t region[] = { CAPTURE_IMAGE_WIDTH, CAPTURE_IMAGE_HEIGHT, 1};
        size_t row_pitch = 0;

        void *vec_image = clEnqueueMapImage( ocl_command_queue, ocl_gl_vector_image, CL_TRUE, CL_MEM_WRITE_ONLY, origin, region, &row_pitch, nullptr, 0, nullptr, nullptr, &ocl_err);
        CL_CHECK_ERROR(ocl_err);

        memset( vec_image, 0, CAPTURE_IMAGE_HEIGHT * row_pitch);

        ocl_err = clEnqueueUnmapMemObject( ocl_command_queue, ocl_gl_vector_image, vec_image, 0, nullptr, nullptr);
        CL_CHECK_ERROR(ocl_err);



        args_index = 0;

            // subtract images
        ocl_err = clSetKernelArg( ocl_subtract_image_kernel, args_index++, sizeof( cl_mem), &ocl_gl_cam_image[ ! curr_cap_image]);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clSetKernelArg( ocl_subtract_image_kernel, args_index++, sizeof( cl_mem), &ocl_gl_cam_image[ curr_cap_image]);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clSetKernelArg( ocl_subtract_image_kernel, args_index++, sizeof( cl_mem), &ocl_subtract_image);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clSetKernelArg( ocl_subtract_image_kernel, args_index++, sizeof( cl_int), &CAPTURE_IMAGE_WIDTH);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clSetKernelArg( ocl_subtract_image_kernel, args_index++, sizeof( cl_int), &CAPTURE_IMAGE_HEIGHT);
        CL_CHECK_ERROR(ocl_err);

        {
            size_t global_work_size[] = { CAPTURE_IMAGE_WIDTH, CAPTURE_IMAGE_HEIGHT};
            ocl_err = clEnqueueNDRangeKernel( ocl_command_queue, ocl_subtract_image_kernel, 2, nullptr, global_work_size, nullptr, 0, nullptr, nullptr);
            CL_CHECK_ERROR(ocl_err);
        }


            // Calculate Optic Flow
        args_index = 0;

        ocl_err = clSetKernelArg( ocl_velocity_map_kernel, args_index++, sizeof( cl_mem), &ocl_gl_cam_image[ ! curr_cap_image]);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clSetKernelArg( ocl_velocity_map_kernel, args_index++, sizeof( cl_mem), &ocl_gl_cam_image[ curr_cap_image]);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clSetKernelArg( ocl_velocity_map_kernel, args_index++, sizeof( cl_mem), &ocl_subtract_image);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clSetKernelArg( ocl_velocity_map_kernel, args_index++, sizeof( cl_mem), &ocl_flow_vector_image);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clSetKernelArg( ocl_velocity_map_kernel, args_index++, sizeof( cl_int), &search_size);
        CL_CHECK_ERROR(ocl_err);
        
        ocl_err = clSetKernelArg( ocl_velocity_map_kernel, args_index++, sizeof( cl_int), &patch_size);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clSetKernelArg( ocl_velocity_map_kernel, args_index++, sizeof( cl_int), &CAPTURE_IMAGE_WIDTH);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clSetKernelArg( ocl_velocity_map_kernel, args_index++, sizeof( cl_int), &CAPTURE_IMAGE_HEIGHT);
        CL_CHECK_ERROR(ocl_err);

        {
            size_t global_work_size[] = { CAPTURE_IMAGE_WIDTH, CAPTURE_IMAGE_HEIGHT};
            ocl_err = clEnqueueNDRangeKernel( ocl_command_queue, ocl_velocity_map_kernel, 2, nullptr, global_work_size, nullptr, 0, nullptr, nullptr);
            CL_CHECK_ERROR(ocl_err);
        }


            // Draw Vector Lines
        args_index = 0;

        ocl_err = clSetKernelArg( ocl_vector_line_kernel, args_index++, sizeof( cl_mem), &ocl_flow_vector_image);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clSetKernelArg( ocl_vector_line_kernel, args_index++, sizeof( cl_mem), &ocl_gl_vector_image);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clSetKernelArg( ocl_vector_line_kernel, args_index++, sizeof( cl_int), &search_size);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clSetKernelArg( ocl_vector_line_kernel, args_index++, sizeof( cl_int), &CAPTURE_IMAGE_WIDTH);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clSetKernelArg( ocl_vector_line_kernel, args_index++, sizeof( cl_int), &CAPTURE_IMAGE_HEIGHT);
        CL_CHECK_ERROR(ocl_err);

        {
            size_t global_work_size[] = { CAPTURE_IMAGE_WIDTH, CAPTURE_IMAGE_HEIGHT};
            ocl_err = clEnqueueNDRangeKernel( ocl_command_queue, ocl_vector_line_kernel, 2, nullptr, global_work_size, nullptr, 0, nullptr, nullptr);
            CL_CHECK_ERROR(ocl_err);
        }



            // Require GL resources
        ocl_err = clEnqueueReleaseGLObjects( ocl_command_queue, 2, ocl_gl_cam_image, 0, nullptr, nullptr);
        CL_CHECK_ERROR(ocl_err);

        ocl_err = clEnqueueReleaseGLObjects( ocl_command_queue, 1, &ocl_gl_vector_image, 0, nullptr, nullptr);
        CL_CHECK_ERROR(ocl_err);




            // capture more
        doCapture(capture_device);

#undef CL_CHECK_ERROR
    }

    SwapBuffers( glView->GetDeviceContext());
}

/**
 * @brief Update()
 */
void Update( double deltaTime)
{
    //update keyboard input
    KeyboardInput::Update();
}

/**
 * @brief UnInitialize()
 */
void UnInitialize( void)
{
    //code
    CL_OBJECT_RELEASE( ocl_image_processing_program, clReleaseProgram);
    CL_OBJECT_RELEASE( ocl_subtract_image_kernel, clReleaseKernel);
    CL_OBJECT_RELEASE( ocl_velocity_map_kernel, clReleaseKernel);
    CL_OBJECT_RELEASE( ocl_vector_line_kernel, clReleaseKernel);

    CL_OBJECT_RELEASE( ocl_gl_cam_image[0], clReleaseMemObject);
    CL_OBJECT_RELEASE( ocl_gl_cam_image[1], clReleaseMemObject);
    CL_OBJECT_RELEASE( ocl_subtract_image, clReleaseMemObject);
    CL_OBJECT_RELEASE( ocl_flow_vector_image, clReleaseMemObject);
    CL_OBJECT_RELEASE( ocl_gl_vector_image, clReleaseMemObject);

    OpenCLUtil::Unintialize();

    if( pTexture_display_program)
    {
        pTexture_display_program->Release();
        delete pTexture_display_program;
        pTexture_display_program = nullptr;
    }

    if( pCamTexture[0])
    {
        pCamTexture[0]->Release();
        delete pCamTexture[0];
        pCamTexture[0] = nullptr;
    }

    if( pCamTexture[1])
    {
        pCamTexture[1]->Release();
        delete pCamTexture[1];
        pCamTexture[1] = nullptr;
    }

    if( pVectorLineTexture)
    {
        pVectorLineTexture->Release();
        delete pVectorLineTexture;
        pVectorLineTexture = nullptr;
    }

    if( pVao)
    {
        delete pVao;
        pVao = nullptr;
    }


    if( glView)
    {
        delete glView;
        glView = nullptr;
    }

    Log("Log Ended...");
}

