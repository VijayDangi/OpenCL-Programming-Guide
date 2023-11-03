/**
 * @file   OGL.cpp
 * @author Vijaykumar Dangi
 * @date   10-Oct-2023
 * 
 * @copyright Copyright (c) 2023
 * 
 * Ref: https://www.youtube.com/watch?v=PH9q0HNBjT4&ab_channel=Acerola
 */

//Headers
#include "Common.h"
#include "third_party/glew/include/GL/wglew.h"

#include <stdio.h>
#include <float.h>
#include <vector>

#include "system/Keyboard.h"

#include "Camera.h"
#include "Resource.h"

#include "Grid.h"
#include "ExMaths.h"

#include "system/GLView.h"
#include "system/Logger.h"

#include "framework/Framebuffer.h"
#include "framework/Texture2D.h"
#include "framework/Texture2DMultisample.h"
#include "framework/TextureCubeMap.h"

#include "post_process/Atmosphere.h"

#include "utility\OpenCLUtil.h"

#include "SceneWaves.h"

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

//Global variables
static GLView *glView = nullptr;

unsigned int g_window_width  = 1270;
unsigned int g_window_height = 720;

bool g_is_escape_key_pressed = false;

Camera g_camera(1.0f, 1.0f, vmath::vec3(0.0f, 60.0f, 0.0f), vmath::vec3(0.0f, 1.0f, 0.0f));

vmath::mat4 g_projection_matrix;

double deltaTime = 0.0;

int g_disable_camera_control = 0;
int g_polygon_mode = GL_FILL;
int g_cull_back_face = 0;
int g_show_grid = 0;

float g_clear_color[]{0.095f, 0.360f, 0.525f};

static const int MSAA_SAMPLE_COUNT = 8;
static Framebuffer *msaa_framebuffer = nullptr;
static Texture2DMultisample *msaa_color_attachment = nullptr;
static Texture2DMultisample *msaa_depth_attachment = nullptr;

static Framebuffer *default_framebuffer = nullptr;
static Texture2D *default_color_attachment = nullptr;
static Texture2D *default_depth_attachment = nullptr;

static TextureCubeMap *cloud_cube_map = nullptr;


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

    //GAME LOOP
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
    //variables
	static int mousePosX, mousePosY;

	int mouseDx, mouseDy;

	int newX = -1;
	int newY = -1;

	POINT pt;

    // code
    mouseDx = mousePosX - mouseX;
    mouseDy = mousePosY - mouseY;

    if( abs(mouseDx) >= g_window_width/2)
    {
        mouseDx = 0;
    }

    if( abs(mouseDy) >= g_window_height/2)
    {
        mouseDy = 0;
    }

    if( (button & MK_LBUTTON) && !g_disable_camera_control)   //calculate camera pitch
    {
        float pitchChange = mouseDy * 0.1f;
        float yawChange = -mouseDx * 0.1f;
        g_camera.rotate(pitchChange, yawChange);
    }


    mousePosX = mouseX;
    mousePosY = mouseY;

    //set current mouse position
    if( !g_disable_camera_control && (( button & MK_LBUTTON ) || ( button & MK_RBUTTON ) || ( button & MK_MBUTTON )))
    {
        newX = WrapInt( mouseX, 22, g_window_width - 22);
        newY = WrapInt( mouseY, 22, g_window_height - 22);

        if((newX != mouseX) || (newY != mouseY))
        {
            pt.x = newX;
            pt.y = newY;

            ClientToScreen( glView->GetHandle(), &pt);
            SetCursorPos( pt.x, pt.y);
            
            mousePosX = newX;
            mousePosY = newY;
        }
    }
}

/**
 * @brief EventCallback()
 */
void EventCallback( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //code
    if( glView)
    {
        ImGui_ImplWin32_WndProcHandler( hwnd, message, wParam, lParam);
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
    bool InitializeImGUI( HWND hwnd);
    void OpenGLLog( void);
    
    //variable declaration
    RECT rc;

    //code
    GetClientRect( hwnd, &rc);
    g_window_width = rc.right - rc.left;
    g_window_height = rc.bottom - rc.top;

    if( InitializeImGUI( hwnd) == false)
    {
        Log( "InitializeImGUI() Failed.");
        return false;
    }

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

    /* =============================== GRID Program =============================== */
    if( OpenCLUtil::Initialize() == false)
    {
        Log( "OpeCLUtil::Initialize() Failed.");
        return false;
    }

    if( InitializeGrid() == false)
    {
        Log( "InitializeGrid() Failed");
        return false;
    }

    if( SceneWaves::Initialize() == false)
    {
        Log( "SceneWaves::Initialize() Failed.");
        return false;
    }

    if( AtmospherePostProcess::Initialize() == false)
    {
        Log( "AtmospherePostProcess::Initialize() Failed.");
        return false;
    }


        // load cube map
    cloud_cube_map = TextureCubeMap::CreateFromFiles(
            "resource/texture/cloudy/cube_map/bluecloud/bluecloud_rt.png", "resource/texture/cloudy/cube_map/bluecloud/bluecloud_lf.png",
            "resource/texture/cloudy/cube_map/bluecloud/bluecloud_up.png", "resource/texture/cloudy/cube_map/bluecloud/bluecloud_dn.png",
            "resource/texture/cloudy/cube_map/bluecloud/bluecloud_bk.png", "resource/texture/cloudy/cube_map/bluecloud/bluecloud_ft.png",
            false
    );
    if( cloud_cube_map == nullptr)
    {
        Log("Error: Failed to create CubeMap");
        return nullptr;
    }

    cloud_cube_map->SetWrapParameter( GL_REPEAT, GL_REPEAT);
    cloud_cube_map->GenerateMipmap();


        // framebuffer
    msaa_framebuffer = new Framebuffer;
    default_framebuffer = new Framebuffer;

    /** ______________ OPENGL STATE SET ________________ **/
	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);
    glEnable( GL_DEPTH_TEST);
    
    //glEnable( GL_CULL_FACE);
    
    //warm-up Resize call
    OnResize( g_window_width, g_window_height);

    return true;
}

/**
 * @brief InitializeImGUI()
 */
bool InitializeImGUI( HWND hwnd)
{
    // code
    IMGUI_CHECKVERSION();

    if( ImGui::CreateContext() == nullptr)
    {
        Log( "ImGui::CreateContext() Failed.\n");
        return false;
    }

    if( ImGui_ImplWin32_Init( hwnd) == false)
    {
        Log( "ImGui_ImplWin32_Init() Failed.\n");
        return false;
    }

    const char *glsl_version = "#version 130";
    if( ImGui_ImplOpenGL3_Init() == false)
    {
        Log( "ImGui_ImplOpenGL_Init() Failed.\n");
        return false;
    }

        // style
    ImGui::StyleColorsDark();
    
    ImGuiStyle &style = ImGui::GetStyle();
    ImVec4 *colors = style.Colors;

        // change background color
    colors[ImGuiCol_WindowBg].w = 0.4f;
    colors[ImGuiCol_ChildBg].w = 0.4f;
    colors[ImGuiCol_TitleBg].w = 0.4f;

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


    if( msaa_framebuffer)
    {
        if( msaa_color_attachment)
        {
            msaa_color_attachment->Release();
            delete msaa_color_attachment;
            msaa_color_attachment = nullptr;
        }

        if( msaa_depth_attachment)
        {
            msaa_depth_attachment->Release();
            delete msaa_depth_attachment;
            msaa_depth_attachment = nullptr;
        }

        msaa_color_attachment = new Texture2DMultisample( width, height, GL_RGBA, MSAA_SAMPLE_COUNT, GL_FALSE);
        msaa_depth_attachment = new Texture2DMultisample( width, height, GL_DEPTH_COMPONENT24, MSAA_SAMPLE_COUNT, GL_FALSE);


        msaa_framebuffer->Bind( GL_FRAMEBUFFER);
            msaa_framebuffer->AttachTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, msaa_color_attachment, 0);
            msaa_framebuffer->AttachTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, msaa_depth_attachment, 0);

            GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0};
            msaa_framebuffer->SetDrawBuffers( _ARRAYSIZE( draw_buffers), draw_buffers);

            if( glCheckFramebufferStatus( GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                Log("Msaa FBO Failed.");
            }
        msaa_framebuffer->Unbind( GL_FRAMEBUFFER);
    }

    if( default_framebuffer)
    {
        if( default_color_attachment)
        {
            default_color_attachment->Release();
            delete default_color_attachment;
            default_color_attachment = nullptr;
        }

        if( default_depth_attachment)
        {
            default_depth_attachment->Release();
            delete default_depth_attachment;
            default_depth_attachment = nullptr;
        }

        default_color_attachment = new Texture2D( width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, nullptr);
        default_depth_attachment = new Texture2D( width, height, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, nullptr);


        default_framebuffer->Bind( GL_FRAMEBUFFER);
            default_framebuffer->AttachTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, default_color_attachment, 0);
            default_framebuffer->AttachTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, default_depth_attachment, 0);

            GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0};
            default_framebuffer->SetDrawBuffers( _ARRAYSIZE( draw_buffers), draw_buffers);

            if( glCheckFramebufferStatus( GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                Log("Default FBO Failed.");
            }
        default_framebuffer->Unbind( GL_FRAMEBUFFER);
    }

    ResizeGrid( width, height);

    glViewport( 0, 0, (GLfloat)width, (GLfloat)height);

        //Apply Perspective
    g_projection_matrix = vmath::perspective(
        45.0f,
        (GLfloat)width/(GLfloat)height,
        0.1f,
        5000.0f
    );
}

/**
 * @brief RenderImGUI()
 */
void RenderImGUI()
{
    // code
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();

    ImGuiIO &io = ImGui::GetIO();

    ImGui::Begin("Controls");
        ImGui::Text( "Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        ImGui::RadioButton("Use ImGUI", &g_disable_camera_control, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Use Camera", &g_disable_camera_control, 0);

        ImGui::NewLine();

        ImGui::Text( "Camera Position: [ %f, %f, %f]", g_camera.vPosition[0], g_camera.vPosition[1], g_camera.vPosition[2]);
        ImGui::Text( "Camera Pitch %f, Yaw: %f", g_camera.fPitch, g_camera.fYaw);
        ImGui::NewLine();

        if( ImGui::TreeNode("Camera"))
        {
            ImGui::DragFloat( "Camera Movement Speed", &g_camera.movementSpeed, 0.01f, 0.0f, 30.0f);
            ImGui::DragFloat( "Camera Rotate Speed", &g_camera.mouseSensitivity, 0.01f, 0.0f, 20.0f);
            ImGui::TreePop();
        }

        if( ImGui::TreeNode("PolygonMode"))
        {
            bool bChanged = false;

            bChanged |= ImGui::RadioButton( "Lines", &g_polygon_mode, GL_LINE);
            bChanged |= ImGui::RadioButton( "Points", &g_polygon_mode, GL_POINT);
            bChanged |= ImGui::RadioButton( "Fill", &g_polygon_mode, GL_FILL);

            if( bChanged)
            {
                glPolygonMode( GL_FRONT_AND_BACK, g_polygon_mode);
            }

            ImGui::TreePop();
        }

        if( ImGui::TreeNode("Backface Culling"))
        {
            bool bChanged = false;

            bChanged |= ImGui::RadioButton( "ON", &g_cull_back_face, 1);
            bChanged |= ImGui::RadioButton( "OFF", &g_cull_back_face, 0);

            if( bChanged)
            {
                if( g_cull_back_face)
                {
                    glEnable( GL_CULL_FACE);
                }
                else
                {
                    glDisable( GL_CULL_FACE);
                }
            }

            ImGui::TreePop();
        }

        ImGui::Text("Show Grid: ");
        ImGui::SameLine();
        ImGui::RadioButton( "ON", &g_show_grid, 1);
        ImGui::SameLine();
        ImGui::RadioButton( "OFF", &g_show_grid, 0);


            // clear color
        ImGui::ColorEdit3( "Clear Color", g_clear_color, ImGuiColorEditFlags_::ImGuiColorEditFlags_Float);

        SceneWaves::RenderImGUI();

        AtmospherePostProcess::RenderImGui();

    ImGui::End();

    ImGui::Render();

    // show on screen
    ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData());
}

/**
 * @brief Render()
 */
void Render( vmath::mat4& view_matrix)
{
    //function declaration

    //variable declarations
    vmath::mat4 model_matrix = vmath::mat4::identity();

    //code
        //background color
    if( g_show_grid)
    {
        RenderGrid( g_projection_matrix, view_matrix);
    }

    SceneWaves::Render(
        view_matrix, g_projection_matrix, g_camera.vPosition,
        AtmospherePostProcess::u_sun_direction, AtmospherePostProcess::u_sun_color,
        cloud_cube_map
    );
}

/**
 * @brief Display()
 */
void Display( void)
{
    vmath::mat4 view_matrix = g_camera.getViewMatrix();

    msaa_framebuffer->Bind(GL_FRAMEBUFFER);

        glClearColor( g_clear_color[0], g_clear_color[1], g_clear_color[2], 1.0f);
        glViewport( 0, 0, g_window_width, g_window_height);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Render(view_matrix);

    msaa_framebuffer->Unbind(GL_FRAMEBUFFER);


        // resolve framebuffer
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, default_framebuffer->GetID());
    glBindFramebuffer( GL_READ_FRAMEBUFFER, msaa_framebuffer->GetID());
        glBlitFramebuffer(
            0, 0, g_window_width, g_window_height,
            0, 0, g_window_width, g_window_height,
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
            GL_NEAREST
        );

        // bind default framebuffer
    glBindFramebuffer( GL_FRAMEBUFFER, 0);

    glClearColor( 0.0, 0.0, 0.0, 1.0f);
    glViewport( 0, 0, g_window_width, g_window_height);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // post process Atmosphere
    if( g_polygon_mode != GL_FILL)
    {
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
    }
    AtmospherePostProcess::Process(
        default_color_attachment, default_depth_attachment,
        cloud_cube_map, 0.1f, 5000.0f,
        g_camera.vPosition, view_matrix, g_projection_matrix);

    if( g_polygon_mode != GL_FILL)
    {
        glPolygonMode( GL_FRONT_AND_BACK, g_polygon_mode);
    }

    RenderImGUI();

        // copy buffer
    // glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0);
    // glBindFramebuffer( GL_READ_FRAMEBUFFER, default_framebuffer->GetID());
    //     glBlitFramebuffer(
    //         0, 0, g_window_width, g_window_height,
    //         0, 0, g_window_width, g_window_height,
    //         GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
    //         GL_NEAREST
    //     );
    // glBindFramebuffer( GL_FRAMEBUFFER, 0);

//    glFlush();
    SwapBuffers( glView->GetDeviceContext());
}

/**
 * @brief Update()
 */
void Update( double deltaTime)
{
    //update keyboard input
    KeyboardInput::Update();


    //Handle Input
    if( KeyboardInput::IsKeyPressed( VK_ESCAPE))
    {
        g_is_escape_key_pressed = true;
    }

    if( KeyboardInput::IsKeyPressed( 'F'))
    {
        if( glView)
        {
            glView->ToggleFullScreen();
        }
    }

     if( KeyboardInput::IsKeyPressed(VK_F2))
    {
        g_disable_camera_control = !g_disable_camera_control;
    }
    
    if( g_disable_camera_control == false)
    {
            // Camera forward/backward movement
        if( KeyboardInput::IsKeyPressed( 'W') || KeyboardInput::IsKeyHeld( 'W') ||
            KeyboardInput::IsKeyPressed( VK_UP) || KeyboardInput::IsKeyHeld( VK_UP))
        {
            g_camera.moveForward(1.0f);// (deltaTime);
        }
        
        if( KeyboardInput::IsKeyPressed( 'S') || KeyboardInput::IsKeyHeld( 'S') ||
            KeyboardInput::IsKeyPressed( VK_DOWN) || KeyboardInput::IsKeyHeld( VK_DOWN))
        {
            g_camera.moveForward( -1.0f);// (deltaTime);
        }

            // Camera right/left movemene
        if( KeyboardInput::IsKeyPressed( 'D') || KeyboardInput::IsKeyHeld( 'D') ||
            KeyboardInput::IsKeyPressed( VK_RIGHT) || KeyboardInput::IsKeyHeld( VK_RIGHT))
        {
            g_camera.moveRight(1.0f);// (deltaTime);
        }
        
        if( KeyboardInput::IsKeyPressed( 'A') || KeyboardInput::IsKeyHeld( 'A') ||
            KeyboardInput::IsKeyPressed( VK_LEFT) || KeyboardInput::IsKeyHeld( VK_LEFT))
        {
            g_camera.moveRight(-1.0f);// (deltaTime);
        }

            // Camera up/down movemene
        if( KeyboardInput::IsKeyPressed( 'E') || KeyboardInput::IsKeyHeld( 'E'))
        {
            g_camera.moveUp(1.0f);// (deltaTime);
        }
        
        if( KeyboardInput::IsKeyPressed( 'Q') || KeyboardInput::IsKeyHeld( 'Q'))
        {
            g_camera.moveUp(-1.0f);// (deltaTime);
        }
    }

    UpdateGrid( deltaTime);
    SceneWaves::Update( deltaTime);
    AtmospherePostProcess::Update( deltaTime);
}

/**
 * @brief UninitializeImGUI()
 */
void UninitializeImGUI()
{
    //code
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

/**
 * @brief UnInitialize()
 */
void UnInitialize( void)
{
    //code
    OpenCLUtil::Unintialize();
    UninitializeGrid();
    SceneWaves::Uninitialize();
    AtmospherePostProcess::Uninitialize();

    if( msaa_framebuffer)
    {
        msaa_framebuffer->Release();
        delete msaa_framebuffer;
        msaa_framebuffer = nullptr;
    }

    if( msaa_color_attachment)
    {
        msaa_color_attachment->Release();
        delete msaa_color_attachment;
        msaa_color_attachment = nullptr;
    }

    if( msaa_depth_attachment)
    {
        msaa_depth_attachment->Release();
        delete msaa_depth_attachment;
        msaa_depth_attachment = nullptr;
    }


    if( default_framebuffer)
    {
        default_framebuffer->Release();
        delete default_framebuffer;
        default_framebuffer = nullptr;
    }

    if( default_color_attachment)
    {
        default_color_attachment->Release();
        delete default_color_attachment;
        default_color_attachment = nullptr;
    }

    if( default_depth_attachment)
    {
        default_depth_attachment->Release();
        delete default_depth_attachment;
        default_depth_attachment = nullptr;
    }

    if( cloud_cube_map)
    {
        delete cloud_cube_map;
        cloud_cube_map = nullptr;
    }
    
    if( glView)
    {
        delete glView;
        glView = nullptr;
    }

    Log("Log Ended...");
}

