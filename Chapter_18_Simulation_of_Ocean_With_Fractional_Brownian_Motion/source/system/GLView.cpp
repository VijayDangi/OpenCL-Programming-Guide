#include "GLView.h"
#include "Logger.h"

#define GLViewClassName TEXT("GLViewClass")
#define GLViewTitle TEXT("GLView")

/**
 * @brief PrintPixelFormatDescriptor()
 * @param pfd 
 */
static void PrintPixelFormatDescriptor( PIXELFORMATDESCRIPTOR const& pfd)
{
    // code
    /*
        typedef struct tagPIXELFORMATDESCRIPTOR
        {
            WORD  nSize;
            WORD  nVersion;
            DWORD dwFlags;
            BYTE  iPixelType;
            BYTE  cColorBits;
            BYTE  cRedBits;
            BYTE  cRedShift;
            BYTE  cGreenBits;
            BYTE  cGreenShift;
            BYTE  cBlueBits;
            BYTE  cBlueShift;
            BYTE  cAlphaBits;
            BYTE  cAlphaShift;
            BYTE  cAccumBits;
            BYTE  cAccumRedBits;
            BYTE  cAccumGreenBits;
            BYTE  cAccumBlueBits;
            BYTE  cAccumAlphaBits;
            BYTE  cDepthBits;
            BYTE  cStencilBits;
            BYTE  cAuxBuffers;
            BYTE  iLayerType;
            BYTE  bReserved;
            DWORD dwLayerMask;
            DWORD dwVisibleMask;
            DWORD dwDamageMask;
        } PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, FAR *LPPIXELFORMATDESCRIPTOR;
    */

    std::string String;
    String.append( "nSize: ");              String.append( std::to_string( (int)pfd.nSize) + "\n");
    String.append( "nVersion: ");           String.append( std::to_string( (int)pfd.nVersion) + "\n");
    String.append( "dwFlags: ");            String.append( std::to_string( (int)pfd.dwFlags) + "\n");
    String.append( "iPixelType: ");         String.append( std::to_string( (int)pfd.iPixelType) + "\n");
    String.append( "cColorBits: ");         String.append( std::to_string( (int)pfd.cColorBits) + "\n");
    String.append( "cRedBits: ");           String.append( std::to_string( (int)pfd.cRedBits) + "\n");
    String.append( "cRedShift: ");          String.append( std::to_string( (int)pfd.cRedShift) + "\n");
    String.append( "cGreenBits: ");         String.append( std::to_string( (int)pfd.cGreenBits) + "\n");
    String.append( "cGreenShift: ");        String.append( std::to_string( (int)pfd.cGreenShift) + "\n");
    String.append( "cBlueBits: ");          String.append( std::to_string( (int)pfd.cBlueBits) + "\n");
    String.append( "cBlueShift: ");         String.append( std::to_string( (int)pfd.cBlueShift) + "\n");
    String.append( "cAlphaBits: ");         String.append( std::to_string( (int)pfd.cAlphaBits) + "\n");
    String.append( "cAlphaShift: ");        String.append( std::to_string( (int)pfd.cAlphaShift) + "\n");
    String.append( "cAccumBits: ");         String.append( std::to_string( (int)pfd.cAccumBits) + "\n");
    String.append( "cAccumRedBits: ");      String.append( std::to_string( (int)pfd.cAccumRedBits) + "\n");
    String.append( "cAccumGreenBits: ");    String.append( std::to_string( (int)pfd.cAccumGreenBits) + "\n");
    String.append( "cAccumBlueBits: ");     String.append( std::to_string( (int)pfd.cAccumBlueBits) + "\n");
    String.append( "cAccumAlphaBits: ");    String.append( std::to_string( (int)pfd.cAccumAlphaBits) + "\n");
    String.append( "cDepthBits: ");         String.append( std::to_string( (int)pfd.cDepthBits) + "\n");
    String.append( "cStencilBits: ");       String.append( std::to_string( (int)pfd.cStencilBits) + "\n");
    String.append( "cAuxBuffers: ");        String.append( std::to_string( (int)pfd.cAuxBuffers) + "\n");
    String.append( "iLayerType: ");         String.append( std::to_string( (int)pfd.iLayerType) + "\n");
    String.append( "bReserved: ");          String.append( std::to_string( (int)pfd.bReserved) + "\n");
    String.append( "dwLayerMask: ");        String.append( std::to_string( (int)pfd.dwLayerMask) + "\n");
    String.append( "dwVisibleMask: ");      String.append( std::to_string( (int)pfd.dwVisibleMask) + "\n");
    String.append( "dwDamageMask: ");       String.append( std::to_string( (int)pfd.dwDamageMask) + "\n");

    Log("PIXELFORMATDESCRIPTOR\n%s", String.c_str());
}


/*******************/

/**
 * @brief GLView()
 * @param className 
 * @param title 
 * @param hInstance 
 * @param width 
 * @param height 
 * @param icon_resource 
 */
GLView::GLView( TCHAR *className, TCHAR *title, HINSTANCE hInstance, unsigned int width, unsigned int height, TCHAR *icon_resource)
    : MainWindow( className, title, hInstance, width, height, icon_resource)
{

}

/**
 * @brief ~GLView()
 */
GLView::~GLView()
{
    // code
    Log("");

    wglMakeCurrent( nullptr, nullptr);

    if( m_pRC)
    {
        wglDeleteContext( m_pRC);
        m_pRC = nullptr;
    }

    if( m_pDC)
    {
        ReleaseDC( GetHandle(), m_pDC);
        m_pDC = nullptr;
    }
}

/**
 * @brief MakeCurrentContext()
 */
void GLView::MakeCurrentContext()
{
    // code
    wglMakeCurrent( m_pDC, m_pRC);
}

/**
 * The need of creating dummy context:
 *      The function you use to get WGL extensions is, itself an OpenGL extension.
 *  Thus like any OpenGL function, it requires and OpenGL context to call it.
 *  So in order to get the functions we need to create a context, we have to... create a context.
 *  Fortunately, this context does not need to be our final context. All we need to do is 
 *  create a dummy context to get function pointers, then use those functions directly.
 * 
 * WARNING: Unfortunately, Windows platform does not allow the user to change the pixel format of a window.
 *          You get to set it exactly once. Therefore, if you want to use a different pixel format from
 *          the one your fake context used ( for sRGB or multisample frambuffers, or just different bit-depth
 *          of buffers), you must destroy the window entirely and recreate it after we are finished with the dummy
 *          context.
 * 
 * @ref https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)
 */
HGLRC GLView::create_dummy_context( HWND hwnd, HDC hDC)
{
    // code
    // create dummy_window

    PIXELFORMATDESCRIPTOR pfd{};

    int pixel_format = ChoosePixelFormat( hDC, &pfd);

    SetPixelFormat( hDC, pixel_format, &pfd);

    PrintPixelFormatDescriptor( pfd);

    HGLRC hRC = wglCreateContext( hDC);

    if( wglMakeCurrent( hDC, hRC) == TRUE)
    {
        Log( "Created OpenGL dummy context.");
    }
    else
    {
        Log( "Failed to create OpenGL dummy context.");
    }

    return hRC;
};

/**
 * @brief setup_pixel_format()
 * @param window 
 * @return 
 */
BOOL GLView::setup_pixel_format( MainWindow *window, HDC hDC)
{
    // variable declaration
    const int attribList[] = 
    {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        0, // End
    };

    int pixelFormat;
    UINT numFormats;

    // code
    // we retrive pixel format index or pixel format
    if( wglChoosePixelFormatARB( hDC, attribList, nullptr, 1, &pixelFormat, &numFormats) == FALSE)
    {
        Log( "wglChoosePixelFormatARB() Failed.");
        return FALSE;
    }

    PIXELFORMATDESCRIPTOR pfd{};

    // we fill pfd with information for pixelFormat
    int pixel_description = DescribePixelFormat( hDC, pixelFormat, sizeof( pfd), &pfd);
    PrintPixelFormatDescriptor( pfd);

    // we store pfd on to hDC
    return SetPixelFormat( hDC, pixelFormat, &pfd); // ChoosePixelFormat( hDC, &pfd) != 0 ? TRUE : FALSE;
}

/**
 * @brief this will create opengl rendering context and delete dummy_context created in function create_dummy_context()
 * @param window 
 * @param dummy_context 
 * @param p_ret_context 
 * @param major_version 
 * @param minor_version 
 * @param context_profile 
 * @return 
 */
BOOL GLView::set_opengl_context(
    MainWindow *window, HDC hDC, HGLRC *p_ret_context,
    int major_version, int minor_version, int context_profile)
{
    if( wglewIsSupported("WGL_ARB_create_context") != GL_TRUE)
    {
        Log( "OpenGL version 3.2 or above should be supported.");
        return FALSE;
    }

#if defined(DEBUG) || defined(_DEBUG)
    /*
        OpenGL Programming Guide 9th Edition
            Appendix G. Debugging and Profiling OpenGL 
                - Create a Debug Context
     */

    int attribs[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, major_version,
        WGL_CONTEXT_MINOR_VERSION_ARB, minor_version,
        WGL_CONTEXT_PROFILE_MASK_ARB, context_profile,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
        0, 0    // End of the array
    };

    Log( "Setting debugging OpenGL context.");
#else

    int attribs[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, major_version,
        WGL_CONTEXT_MINOR_VERSION_ARB, minor_version,
        WGL_CONTEXT_PROFILE_MASK_ARB, context_profile,
        0, 0    // End of the array
    };

    Log( "Setting non-debugging OpenGL context.");
#endif

    *p_ret_context = wglCreateContextAttribsARB( hDC, 0, attribs);

    return wglMakeCurrent( hDC, *p_ret_context);
}

/**
 * @brief CreateGLView()
 * @param hInstance 
 * @param view_width 
 * @param view_height 
 * @param gl_major_version 
 * @param gl_minor_version 
 * @param context_flags 
 * @param icon_resource 
 * @return 
 */
GLView* GLView::CreateGLView(
    HINSTANCE hInstance, unsigned int view_width, unsigned int view_height,
    int gl_major_version, int gl_minor_version, int context_flags,
    TCHAR *icon_resource)
{
    // variable declaration
    GLView *glView = nullptr;

    // code
    if( (gl_major_version < 1 || gl_minor_version < 0) ||
        (gl_major_version == 1 && gl_minor_version > 5) ||
        (gl_major_version == 2 && gl_minor_version > 1) ||
        (gl_major_version == 3 && gl_minor_version > 3) ||
        (gl_major_version == 4 && gl_minor_version > 6)
    )
    {
        // OpenGL 1.0 is the smallest valid version
        // OpenGL 1.x series ended with version 1.5
        // OpenGL 2.x series ended with version 2.1
        // OpenGL 3.x series ended with version 3.3
        // OpenGL 4.x series ended with version 4.6

        Log("Invalid major or minor version for OpenGL context.");
        return nullptr;
    }

        // create dummy window (This window is used to make dummy rendering context which initialize OpenGL functions)
    WNDCLASSEX wndClass{};
    HWND dummy_window_hwnd;
    HDC dummy_window_hdc;

    wndClass.cbSize = sizeof( WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon( NULL, IDI_APPLICATION);
    wndClass.hIconSm = LoadIcon( NULL, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor( NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH) GetStockObject( BLACK_BRUSH);
    wndClass.lpszClassName = TEXT("_dummy_window_");
    wndClass.lpszMenuName = NULL;
    wndClass.lpfnWndProc = DefWindowProc;

    RegisterClassEx( &wndClass);

        //  create window
    dummy_window_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW, TEXT("_dummy_window_"), TEXT(""),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL
    );

        // get device context
    dummy_window_hdc = GetDC( dummy_window_hwnd);

        // create dummy rendering context
    HGLRC dummy_context = create_dummy_context( dummy_window_hwnd, dummy_window_hdc);

        // Load OpenGL functions
    if( GLEW_OK != glewInit())
    {
        Log("Failed to load OpenGL Library function (glewInit() Failed).");
        
        delete glView;
        glView = nullptr;

        return nullptr;
    }

        // now OpenGL functions are intialized we can now delete the dummy window and dummy context
    wglMakeCurrent( nullptr, nullptr);

    wglDeleteContext( dummy_context);
    dummy_context = NULL;

    ReleaseDC( dummy_window_hwnd, dummy_window_hdc);
    dummy_window_hdc = NULL;

    DestroyWindow( dummy_window_hwnd);
    dummy_window_hwnd = NULL;
    

        // cretae main window on which rendering to be done
    glView = new GLView( GLViewClassName, GLViewTitle, hInstance, view_width, view_height, icon_resource);
    if( glView == nullptr)
    {
        Log("Failed to create GLView.");
        return nullptr;
    }

    glView->ShowWindow();
    glView->m_pDC = GetDC( glView->GetHandle());

    if( setup_pixel_format( glView, glView->m_pDC) != TRUE)
    {
        Log("Failed to setup pixel format.");

        delete glView;
        glView = nullptr;

        return nullptr;
    }

    if( set_opengl_context( glView, glView->m_pDC, &(glView->m_pRC), gl_major_version, gl_minor_version) != TRUE)
    {
        Log("set_opengl_context() Failed.");

        delete glView;
        glView = nullptr;

        return nullptr;
    }

    return glView;
}

