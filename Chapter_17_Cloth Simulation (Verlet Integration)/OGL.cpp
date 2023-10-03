/**
 * @file   OGL.cpp
 * @brief  OpenGL Programmable Pipeline Template
 * @author Vijaykumar Dangi
 * @date   07-Aug-2022
 * 
 * @copyright Copyright (c) 2022
 */

//Headers
#include "OGL.h"

#include <stdio.h>
#include <float.h>
#include <vector>

#include "Keyboard.h"

#include "Camera.h"
#include "Resource.h"

#include "Grid.h"
#include "ExMaths.h"

#include "OpenCLUtil.h"
#include "Cloth_CL.h"

//Library
#pragma comment( lib, "user32.lib")
#pragma comment( lib, "gdi32.lib")
#pragma comment( lib, "kernel32.lib")

#pragma comment( lib, "glew32.lib")
#pragma comment( lib, "OpenGL32.lib")

//macro
#define ENABLE_LOCK_FPS 0
#define DESIRE_FPS 60

#define BOUND_DIMENSION 40.0f

//Global function declaration
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM);

//Global variables
static FILE *gp_log_file = nullptr;
HWND  g_hwnd;
HDC   g_hdc;
HGLRC g_hrc;

DWORD style;
WINDOWPLACEMENT wpPrev = { sizeof(WINDOWPLACEMENT) };

int g_window_width  = 800;
int g_window_height = 600;

bool g_is_active_window = false;
bool g_is_escape_key_pressed = false;
bool g_is_fullscreen = false;

Camera g_camera(1.0f, 0.05f, vmath::vec3(0.0f, 1.0f, 3.0f), vmath::vec3(0.0f, 1.0f, 0.0f));

vmath::mat4 g_projection_matrix;

double deltaTime = 0.0;

ClothSimulation_OpenCL::Cloth red_cloth = nullptr;

GLuint texture_program;
GLint texture_u_model_matrix;
GLint texture_u_view_matrix;
GLint texture_u_projection_matrix;
GLint texture_u_texture;

GLuint sponza_curtain_blue_texture = 0;

GLuint vao_wireframe_cube;
GLuint vbo_wireframe_cube;

bool b_toggle_cloth_update = false;

//WinMain()
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInsatnce, LPSTR szCmdLine, int iCmdShow)
{
    //function declarations
    bool Initialize( void);
    void Display( void);
    void Update( double deltaTime);
    void UnInitialize( void);
    
    //variable declarations
    WNDCLASSEX wndclass;
    HWND  hwnd;
    MSG   msg;
    TCHAR szClassName[] = TEXT("OpenGLPP");
    bool  bDone = false;

    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER Frequency;

    //code
    if(fopen_s( &gp_log_file, "LogFile.txt", "w") != 0)
    {
        MessageBox( NULL, TEXT("Error while creating log file"), TEXT("Error"), MB_OK | MB_TOPMOST | MB_ICONSTOP);
        return( EXIT_FAILURE);
    }
    else
    {
        fprintf( gp_log_file, "Log Started...\n");
        fflush( gp_log_file);
        fclose( gp_log_file);
        gp_log_file = nullptr;
    }
    
    //Initialize window attributes
    wndclass.cbSize         = sizeof(WNDCLASSEX);
    wndclass.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_MYICON));
    wndclass.hIconSm        = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_MYICON));
    wndclass.hCursor        = LoadCursor( NULL, IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH) GetStockObject( BLACK_BRUSH);
    wndclass.lpszClassName  = szClassName;
    wndclass.lpszMenuName   = NULL;
    wndclass.lpfnWndProc    = WndProc;
    
    if(!RegisterClassEx( &wndclass))
    {
        Log( "Class Not Registered\n");
        return( EXIT_FAILURE);
    }
    
    hwnd = CreateWindowEx(
                WS_EX_APPWINDOW,
                szClassName, TEXT("OpenGL"),
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
                100, 100, g_window_width, g_window_height,
                NULL, NULL,
                hInstance, NULL
            );
    g_hwnd = hwnd;

    ShowWindow( hwnd, iCmdShow);
    SetForegroundWindow(hwnd);
    SetFocus( hwnd);
    
    if( Initialize() == false)
    {
        bDone = true;
    }
    
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
            if(g_is_active_window == true)
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
    
    UnInitialize();

    UnregisterClass( szClassName, hInstance);

    return( (int)msg.wParam);
}

//WndProc()
LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //function declaration
    void Resize( int, int);
    void ToggleFullScreen( void);
    void InitializeDessolveEffect();
    
    //variables
	static int mousePosX, mousePosY;
	static int iAccumDelta, zDelta;

	int mouseX, mouseY;
	int mouseDx, mouseDy;

	int newX = -1;
	int newY = -1;

	POINT pt;
    
    //code
    switch(message)
    {
        case WM_SETFOCUS:
            g_is_active_window = true;
        break;

        case WM_KILLFOCUS:
            g_is_active_window = false;
        break;
        
        case WM_ERASEBKGND:
        return(0);
        
        case WM_SIZE:
            g_window_width = LOWORD(lParam);
            g_window_height = HIWORD(lParam);
            
            Resize( g_window_width, g_window_height);
        break;
        
        case WM_MOUSEWHEEL:
        {
            if(zDelta == 0)
                break;

            mouseX = LOWORD(lParam);
            mouseY = HIWORD(lParam);
            
            iAccumDelta = iAccumDelta + (short)HIWORD(wParam);

            while( iAccumDelta >= zDelta)
                iAccumDelta = iAccumDelta - zDelta;
            
            while( iAccumDelta <= -zDelta)
                iAccumDelta = iAccumDelta + zDelta;
        }
        break;
        
        case WM_MOUSEMOVE:
        {
            mouseX = LOWORD(lParam);
            mouseY = HIWORD(lParam);

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

            if( wParam & MK_LBUTTON)   //calculate camera pitch
            {
                float pitchChange = mouseDy * 0.1f;
                float yawChange = -mouseDx * 0.1f;
				g_camera.rotate(pitchChange, yawChange);
            }


            mousePosX = mouseX;
            mousePosY = mouseY;

            //set current mouse position
            if( ( wParam & MK_LBUTTON ) || ( wParam & MK_RBUTTON ) || ( wParam & MK_MBUTTON ))
            {
                newX = WrapInt( mouseX, 22, g_window_width - 22);
                newY = WrapInt( mouseY, 22, g_window_height - 22);

                if((newX != mouseX) || (newY != mouseY))
                {
                    pt.x = newX;
                    pt.y = newY;
                    
                    ClientToScreen( hwnd, &pt);
                    SetCursorPos( pt.x, pt.y);
                    
                    mousePosX = newX;
                    mousePosY = newY;
                }
            }
        }
        break;

        case WM_LBUTTONDOWN:
            SetCapture( hwnd);
        break;

        case WM_LBUTTONUP:
            ReleaseCapture();
        break;
        
        case WM_CLOSE:
        break;
        
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
    }
    
    return( DefWindowProc(hwnd, message, wParam, lParam));
}

//ToggleFullScreen()
void ToggleFullScreen( void)
{
    //variable declarations
    MONITORINFO mi = { sizeof(MONITORINFO) };
    
    //code
    if(g_is_fullscreen == false)
    {
        style = GetWindowLong(g_hwnd, GWL_STYLE);
        if(style & WS_OVERLAPPEDWINDOW)
        {
            if(
                GetWindowPlacement(g_hwnd, &wpPrev) &&
                GetMonitorInfo(
                    MonitorFromWindow(g_hwnd, MONITORINFOF_PRIMARY),
                    &mi
                )
            )
            {
                SetWindowLong(g_hwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(
                    g_hwnd,
                    HWND_TOP,
                    mi.rcMonitor.left,
                    mi.rcMonitor.top,
                    mi.rcMonitor.right - mi.rcMonitor.left,
                    mi.rcMonitor.bottom - mi.rcMonitor.top,
                    SWP_NOZORDER | SWP_FRAMECHANGED
                );
            }
        }
        
        //ShowCursor( FALSE);
    }
    else
    {
        SetWindowLong( g_hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement( g_hwnd, &wpPrev);
        SetWindowPos(
            g_hwnd,
            HWND_TOP,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED
        );
        
        //ShowCursor( TRUE);
    }
}

//Error Log
void PrintLog( int lineNo, char *fileName, char *functionName, char *format, ...)
{
    // code
    fopen_s( &gp_log_file, "LogFile.txt", "a");

    if( gp_log_file)
    {
        va_list argList;

        va_start( argList, format);

            fprintf( gp_log_file, "[%s\\%s() : %d]: ", fileName, functionName, lineNo);
            vfprintf( gp_log_file, format, argList);
            fprintf( gp_log_file, "\n");
            fflush( gp_log_file);
        
        va_end( argList);

        fclose( gp_log_file);
        gp_log_file = nullptr;
    }
}

//Initialize()
bool Initialize(void)
{
    //function declarations
    void Resize( int, int);
    bool InitializeOpenGL( HWND hwnd, HDC *pHDC, HGLRC *pHGLRC);
    void OpenGLLog( void);
    
    //variable declaration
    RECT rc;

    //code
    GetClientRect( g_hwnd, &rc);
    g_window_width = rc.right - rc.left;
    g_window_height = rc.bottom - rc.top;

    /** ___________________ OpenGL Initialize _______________________**/
    if( InitializeOpenGL( g_hwnd, &g_hdc, &g_hrc) == false)
    {
        return false;
    }

        //Initialize GLEW
    GLenum glew_error = glewInit();
    if(glew_error != GLEW_OK)
    {
        return false;
    }

    /**_________________ OPENGL INFORMATION _____________________**/
    OpenGLLog();

    /* =============================== GRID Program =============================== */
    if( InitializeGrid() == false)
    {
        Log( "InitializeGrid() Failed");
        return false;
    }

    Log("");

    if( OpenCLUtil::Initialize() == false)
    {
        Log("OpenCLUtil::Initialize() Failed.");
        return false;
    }

    Log("");

    if( ClothSimulation_OpenCL::Initialize() == false)
    {
        Log( "ClothSimulation_OpenCL::Initialize() Failed.");
        return false;
    }

    Log("");

    red_cloth = ClothSimulation_OpenCL::CreateCloth( 30, 30, 512, 512, 0.0f, 5.0f);
    if( red_cloth == nullptr)
    {
        Log( "CreateCloth() Failed.");
        return false;
    }

    Log("");

        /////// texture shader
    SHADERS_INFO shaders_info[] = {
        { GL_VERTEX_SHADER, SHADER_INFO_LOAD_FROM_FILE, "shaders/texture/vertex_shader.glsl", 0},
        { GL_FRAGMENT_SHADER, SHADER_INFO_LOAD_FROM_FILE, "shaders/texture/fragment_shader.glsl", 0}
    };

    BIND_ATTRIBUTES_INFO bind_attrib[] = {
        "a_position", ATTRIBUTE_INDEX::POSITION,
        "a_normal", ATTRIBUTE_INDEX::NORMAL,
        "a_texcoord", ATTRIBUTE_INDEX::TEXCOORD2D
    };

    texture_program = CreateProgram( shaders_info, _ARRAYSIZE(shaders_info), bind_attrib, _ARRAYSIZE(bind_attrib), nullptr);
    if( texture_program == 0)
    {
        Log("CreateProgram() Failed.");
        return false;
    }

    Log( "u_model_matrix:: %d", texture_u_model_matrix = glGetUniformLocation( texture_program, "u_model_matrix"));
    Log( "u_view_matrix:: %d", texture_u_view_matrix = glGetUniformLocation( texture_program, "u_view_matrix"));
    Log( "u_projection_matrix:: %d", texture_u_projection_matrix = glGetUniformLocation( texture_program, "u_projection_matrix"));
    Log( "u_texture:: %d", texture_u_texture = glGetUniformLocation( texture_program, "u_texture"));

    Log("");

        // texture loading
    LoadTexture( &sponza_curtain_blue_texture, "texture/sponza_curtain_blue_diff.png");

    Log("");

        // Wireframe Cube
    float wire_frame_cube[] =
    {
        //Front
            //position
             BOUND_DIMENSION,  BOUND_DIMENSION, BOUND_DIMENSION,
            -BOUND_DIMENSION,  BOUND_DIMENSION, BOUND_DIMENSION,

            -BOUND_DIMENSION,  BOUND_DIMENSION, BOUND_DIMENSION,
            -BOUND_DIMENSION, -BOUND_DIMENSION, BOUND_DIMENSION,

            -BOUND_DIMENSION, -BOUND_DIMENSION, BOUND_DIMENSION,
             BOUND_DIMENSION, -BOUND_DIMENSION, BOUND_DIMENSION,

             BOUND_DIMENSION, -BOUND_DIMENSION, BOUND_DIMENSION,
             BOUND_DIMENSION,  BOUND_DIMENSION, BOUND_DIMENSION,
        
        //Right
            //position
            BOUND_DIMENSION,  BOUND_DIMENSION, -BOUND_DIMENSION,
            BOUND_DIMENSION,  BOUND_DIMENSION,  BOUND_DIMENSION,

            BOUND_DIMENSION,  BOUND_DIMENSION,  BOUND_DIMENSION,
            BOUND_DIMENSION, -BOUND_DIMENSION,  BOUND_DIMENSION,

            BOUND_DIMENSION, -BOUND_DIMENSION,  BOUND_DIMENSION,
            BOUND_DIMENSION, -BOUND_DIMENSION, -BOUND_DIMENSION,

            BOUND_DIMENSION, -BOUND_DIMENSION, -BOUND_DIMENSION,
            BOUND_DIMENSION,  BOUND_DIMENSION, -BOUND_DIMENSION,
        
        //Back
            //position
            -BOUND_DIMENSION,  BOUND_DIMENSION, -BOUND_DIMENSION,
             BOUND_DIMENSION,  BOUND_DIMENSION, -BOUND_DIMENSION,

             BOUND_DIMENSION,  BOUND_DIMENSION, -BOUND_DIMENSION,
             BOUND_DIMENSION, -BOUND_DIMENSION, -BOUND_DIMENSION,

             BOUND_DIMENSION, -BOUND_DIMENSION, -BOUND_DIMENSION,
            -BOUND_DIMENSION, -BOUND_DIMENSION, -BOUND_DIMENSION,

            -BOUND_DIMENSION, -BOUND_DIMENSION, -BOUND_DIMENSION,
            -BOUND_DIMENSION,  BOUND_DIMENSION, -BOUND_DIMENSION,
        
        //Left
            //position
            -BOUND_DIMENSION,  BOUND_DIMENSION,  BOUND_DIMENSION,
            -BOUND_DIMENSION,  BOUND_DIMENSION, -BOUND_DIMENSION,

            -BOUND_DIMENSION,  BOUND_DIMENSION, -BOUND_DIMENSION,
            -BOUND_DIMENSION, -BOUND_DIMENSION, -BOUND_DIMENSION,

            -BOUND_DIMENSION, -BOUND_DIMENSION, -BOUND_DIMENSION,
            -BOUND_DIMENSION, -BOUND_DIMENSION,  BOUND_DIMENSION,

            -BOUND_DIMENSION, -BOUND_DIMENSION,  BOUND_DIMENSION,
            -BOUND_DIMENSION,  BOUND_DIMENSION,  BOUND_DIMENSION,
        
        //Top
            //position
             BOUND_DIMENSION, BOUND_DIMENSION, -BOUND_DIMENSION,
            -BOUND_DIMENSION, BOUND_DIMENSION, -BOUND_DIMENSION,

            -BOUND_DIMENSION, BOUND_DIMENSION, -BOUND_DIMENSION,
            -BOUND_DIMENSION, BOUND_DIMENSION,  BOUND_DIMENSION,

            -BOUND_DIMENSION, BOUND_DIMENSION,  BOUND_DIMENSION,
             BOUND_DIMENSION, BOUND_DIMENSION,  BOUND_DIMENSION,

             BOUND_DIMENSION, BOUND_DIMENSION,  BOUND_DIMENSION,
             BOUND_DIMENSION, BOUND_DIMENSION, -BOUND_DIMENSION,
        
        //Bottom
            //position
             BOUND_DIMENSION, -BOUND_DIMENSION, -BOUND_DIMENSION,
             BOUND_DIMENSION, -BOUND_DIMENSION,  BOUND_DIMENSION,

             BOUND_DIMENSION, -BOUND_DIMENSION,  BOUND_DIMENSION,
            -BOUND_DIMENSION, -BOUND_DIMENSION,  BOUND_DIMENSION,

            -BOUND_DIMENSION, -BOUND_DIMENSION,  BOUND_DIMENSION,
            -BOUND_DIMENSION, -BOUND_DIMENSION, -BOUND_DIMENSION,

            -BOUND_DIMENSION, -BOUND_DIMENSION, -BOUND_DIMENSION,
             BOUND_DIMENSION, -BOUND_DIMENSION, -BOUND_DIMENSION,
    };

    glGenVertexArrays( 1, &vao_wireframe_cube);
    glBindVertexArray( vao_wireframe_cube);
        glGenBuffers( 1, &vbo_wireframe_cube);
        glBindBuffer( GL_ARRAY_BUFFER, vbo_wireframe_cube);
            glBufferData( GL_ARRAY_BUFFER, sizeof( wire_frame_cube), wire_frame_cube, GL_STATIC_DRAW);

            glVertexAttribPointer( ATTRIBUTE_INDEX::POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray( ATTRIBUTE_INDEX::POSITION);
        glBindBuffer( GL_ARRAY_BUFFER, 0);
    glBindVertexArray( 0);


    /** ______________ OPENGL STATE SET ________________ **/
	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);
    glEnable( GL_DEPTH_TEST);
    
    //glEnable( GL_CULL_FACE);
    
    //background color
    glClearColor( 0.5f, 0.5f, 0.5f, 1.0f);
    
    //warm-up Resize call
    Resize( g_window_width, g_window_height);

    return true;
}

//InitializeOpenGL()
bool InitializeOpenGL( HWND hwnd, HDC *pHDC, HGLRC *pHGLRC)
{
    //variable declarations
    PIXELFORMATDESCRIPTOR pfd;
    int iPixelFormatIndex;
    HDC hdc;
    HGLRC hglrc;

    //code
    if(hwnd == NULL)
    {
        return (false);
    }

    *pHDC = NULL;
    *pHGLRC = NULL;

    ZeroMemory( &pfd, sizeof(PIXELFORMATDESCRIPTOR));
    
        //Initialize PIXELFORMATDESCRIPTOR
    pfd.nSize       = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion    = 1;
    pfd.dwFlags     = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType  = PFD_TYPE_RGBA;
    pfd.cColorBits  = 32;
    pfd.cRedBits    = 8;
    pfd.cGreenBits  = 8;
    pfd.cBlueBits   = 8;
    pfd.cAlphaBits  = 8;
    pfd.cDepthBits  = 32;
    
    hdc = GetDC( hwnd);
    
    //choose pixel format
    iPixelFormatIndex = ChoosePixelFormat( hdc, &pfd);
    if(iPixelFormatIndex == 0)
    {
        ReleaseDC( hwnd, hdc);
        hdc = NULL;

        DestroyWindow( hwnd);
        return (false);
    }
    
    //set pixel format
    if(SetPixelFormat( hdc, iPixelFormatIndex, &pfd) == false)
    {
        ReleaseDC( hwnd, hdc);
        hdc = NULL;

        DestroyWindow( hwnd);
        return (false);
    }
    
    //get rendering context
    hglrc = wglCreateContext( hdc);
    if(hglrc == NULL)
    {
        ReleaseDC( hwnd, hdc);
        hdc = NULL;

        DestroyWindow( hwnd);
        return (false);
    }
    
    if(wglMakeCurrent(hdc, hglrc) == false)
    {
        wglDeleteContext( hglrc);
        hglrc = NULL;

        ReleaseDC(hwnd, hdc);
        hdc = NULL;
        
        DestroyWindow( hwnd);
        
        return (false);
    }

    *pHDC = hdc;
    *pHGLRC = hglrc;
   
    return( true);
}

//OpenGLLog()
void OpenGLLog( void)
{
    //variable delcaration
    FILE *pOpenGLInfoLog = NULL;

    //code
    fopen_s(&pOpenGLInfoLog, "OpenGLInformation.log", "w");
    if(pOpenGLInfoLog == NULL)
    {
        MessageBox( NULL, TEXT("Error while create/open file \"OpenGLInformation.log\""), TEXT("Error"), MB_OK | MB_ICONSTOP );
        
        DestroyWindow(g_hwnd);
    }
    
    fprintf(pOpenGLInfoLog, "OpenGL Vendor   : %s\n", glGetString( GL_VENDOR));
    fprintf(pOpenGLInfoLog, "OpenGL Renderer : %s\n", glGetString( GL_RENDERER));
    fprintf(pOpenGLInfoLog, "OpenGL Version  : %s\n", glGetString( GL_VERSION));
    fprintf(pOpenGLInfoLog, "Shading Language (GLSL) Version : %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION));
    
        //OpenGL Enabled Extensions
    GLint numExtension;
    glGetIntegerv( GL_NUM_EXTENSIONS, &numExtension);
    
    fprintf(pOpenGLInfoLog, "Number of Enabled Extensions : %d\n", numExtension);
    fprintf(pOpenGLInfoLog, "Enable Extension : \n");
    for(int i = 0; i < numExtension; i++)
    {
        fprintf(pOpenGLInfoLog, "\t%s\n", glGetStringi(GL_EXTENSIONS, i));
    }
    
    fclose(pOpenGLInfoLog);
    pOpenGLInfoLog = NULL;
}

//Resize()
void Resize( int width, int height)
{
    //code
    if(height == 0)
        height = 1;


    ResizeGrid( width, height);

    glViewport( 0, 0, (GLfloat)width, (GLfloat)height);

        //Apply Perspective
    g_projection_matrix = vmath::perspective(
        45.0f,
        (GLfloat)width/(GLfloat)height,
        0.1f,
        100.0f
    );
}


//Display()
void Display( void)
{
    //function declaration

    //variable declarations
	vmath::mat4 view_matrix = g_camera.getViewMatrix();
    vmath::mat4 model_matrix = vmath::mat4::identity();

    //code
    glViewport( 0, 0, g_window_width, g_window_height);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glUseProgram( texture_program);

    glUniformMatrix4fv( texture_u_model_matrix, 1, GL_FALSE, vmath::mat4::identity());
    glUniformMatrix4fv( texture_u_view_matrix, 1, GL_FALSE, view_matrix);
    glUniformMatrix4fv( texture_u_projection_matrix, 1, GL_FALSE, g_projection_matrix);

    glActiveTexture( GL_TEXTURE0);
    glBindTexture( GL_TEXTURE_2D, sponza_curtain_blue_texture);
    glUniform1i( texture_u_texture, 0);

    ClothSimulation_OpenCL::Render( red_cloth);

    // wire frame cube
    float line_width;
    glGetFloatv( GL_LINE_WIDTH, &line_width);

    glBindVertexArray( vao_wireframe_cube);
        glLineWidth( 4.0f);
        glDrawArrays( GL_LINES, 0, 48);
    glBindVertexArray( 0);

    glLineWidth( line_width);

    glBindTexture( GL_TEXTURE_2D, 0);

    glUseProgram( 0);

    RenderGrid( g_projection_matrix, view_matrix);

    SwapBuffers( g_hdc);
}


//Update()
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
        ToggleFullScreen();
        g_is_fullscreen = !g_is_fullscreen;
    }
    
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

    if( KeyboardInput::IsKeyPressed( 'T'))
    {
        b_toggle_cloth_update = !b_toggle_cloth_update;
    }

    if( KeyboardInput::IsKeyPressed( 'L'))
    {
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
    }

    if( KeyboardInput::IsKeyPressed( 'O'))
    {
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
    }

    //update
    if( b_toggle_cloth_update)
    {
        ClothSimulation_OpenCL::Update( red_cloth, deltaTime, vmath::vec3( BOUND_DIMENSION));
    }
    
    UpdateGrid( deltaTime);
}


//UnInitialize()
void UnInitialize( void)
{
    //code
    if(g_is_fullscreen == true)
    {
        SetWindowLong( g_hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement( g_hwnd, &wpPrev);
        SetWindowPos(
            g_hwnd,
            HWND_TOP,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED
        );
        
        ShowCursor(TRUE);
    }

    UninitializeGrid();
    OpenCLUtil::Unintialize();
    
    ClothSimulation_OpenCL::DeleteCloth( red_cloth);
    red_cloth = nullptr;

    ClothSimulation_OpenCL::Uninitialize();

    DELETE_TEXTURE(sponza_curtain_blue_texture);

    DELETE_VERTEX_ARRAY( vao_wireframe_cube);
    DELETE_BUFFER( vbo_wireframe_cube);

    if( texture_program)
    {
        DeleteProgram( texture_program);
        texture_program = 0;
    }


    wglMakeCurrent( NULL, NULL);
    
    if(g_hrc)
    {
        wglDeleteContext(g_hrc);
        g_hrc = NULL;
    }
    
    if(g_hdc)
    {
        ReleaseDC( g_hwnd, g_hdc);
        g_hdc = NULL;
    }
    
    Log("Log Ended...");
}

