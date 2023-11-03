#include "MainWindow.h"

#include "..\Common.h"

MainWindow::MainWindow( TCHAR *className, TCHAR *title, HINSTANCE hInstance, unsigned int width, unsigned int height, TCHAR *icon_resource)
{
    // code
    m_hInstance = hInstance;
    m_className = className;
    CreateClass( icon_resource);
    CreateMainWindow( title, width, height);

    RECT rc;
    GetClientRect( m_window, &rc);
    m_width = rc.right - rc.left;
    m_height = rc.bottom - rc.top;
}

MainWindow::~MainWindow()
{
    // code
    Log("");
    if( m_bFullscreen)
    {
        ToggleFullScreen();
        m_bFullscreen = false;
    }

    DestroyMainWindow();
    DestroyClass();

    m_mouseEventCallbacks.clear();
    m_resizeEventCallbacks.clear();
    m_windowEventCallbacks.clear();
}

void MainWindow::ShowWindow()
{
    // code
    ::ShowWindow( m_window, SW_NORMAL);
    SetForegroundWindow(m_window);
    SetFocus( m_window);
}

HWND MainWindow::GetHandle()
{
    // code
    return m_window;
}

int MainWindow::GetClientWidth()
{
    // code
    return m_width;
}

int MainWindow::GetClientHeight()
{
    // code
    return m_height;
}

void MainWindow::SetWindowTitle( TCHAR *title)
{
    // code
    SetWindowText( m_window, title);
}

void MainWindow::ToggleFullScreen()
{
    // variable declarations
    MONITORINFO mi = { sizeof(MONITORINFO) };
    DWORD style;
    
    // code
    style = (DWORD) GetWindowLongPtr( m_window, GWL_STYLE);

    if( m_bFullscreen == false)
    {
        if(style & WS_OVERLAPPEDWINDOW)
        {
            if(
                GetWindowPlacement( m_window, &m_window_placement) &&
                GetMonitorInfo( MonitorFromWindow(m_window, MONITORINFOF_PRIMARY), &mi)
            )
            {
                SetWindowLong( m_window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);

                SetWindowPos(
                    m_window,
                    HWND_TOP,
                    mi.rcMonitor.left,
                    mi.rcMonitor.top,
                    mi.rcMonitor.right - mi.rcMonitor.left,
                    mi.rcMonitor.bottom - mi.rcMonitor.top,
                    SWP_NOZORDER | SWP_FRAMECHANGED
                );
            }
        }

        m_bFullscreen = true;
    }
    else
    {
        SetWindowLongPtr( m_window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement( m_window, &m_window_placement);
        SetWindowPos(
            m_window,
            HWND_TOP,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED
        );

        m_bFullscreen = false;
    }
}

void MainWindow::RegisterOnMouseMoveCallback( OnMouseMoveEvent mouse_callback)
{
    // code
    if( mouse_callback)
    {
        m_mouseEventCallbacks.push_back( mouse_callback);
    }
}

void MainWindow::RegisterOnResizeCallback( OnResizeEvent resize_callback)
{
    // code
    if( resize_callback)
    {
        m_resizeEventCallbacks.push_back( resize_callback);
    }
}

void MainWindow::RegisterOnWindowCallback( WndProcedureCallback window_callback)
{
    // code
    if( window_callback)
    {
        m_windowEventCallbacks.push_back( window_callback);
    }
}

void MainWindow::CreateClass( TCHAR *icon_resource)
{
    // code
    WNDCLASSEX class_desc = {};

    class_desc.cbSize         = sizeof(WNDCLASSEX);
    class_desc.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    class_desc.cbClsExtra     = 0;
    class_desc.cbWndExtra     = 0;
    class_desc.hInstance      = m_hInstance;
    class_desc.hIcon          = LoadIcon( m_hInstance, icon_resource);
    class_desc.hIconSm        = LoadIcon( m_hInstance, icon_resource);
    class_desc.hCursor        = LoadCursor( NULL, IDC_ARROW);
    class_desc.hbrBackground  = (HBRUSH) GetStockObject( BLACK_BRUSH);
    class_desc.lpszClassName  = m_className.c_str();
    class_desc.lpszMenuName   = NULL;
    class_desc.lpfnWndProc    = MessageCallback;

    m_windowClass = RegisterClassEx( &class_desc);
}

void MainWindow::DestroyClass()
{
    // code
    UnregisterClass( m_className.c_str(), m_hInstance);
}

void MainWindow::CreateMainWindow(TCHAR *title, unsigned int width, unsigned int height)
{
    // code
    m_window = CreateWindowEx(
        WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
        (TCHAR*)m_windowClass, title,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
        (GetSystemMetrics( SM_CXSCREEN) - width) / 2,
        (GetSystemMetrics( SM_CYSCREEN) - height) / 2,
        width, height,
        nullptr, nullptr,
        m_hInstance, nullptr
    );

    // set window use data
    SetWindowLongPtr( m_window, GWLP_USERDATA, (LONG_PTR)this);
}

void MainWindow::DestroyMainWindow()
{
    // code
    DestroyWindow( m_window);
}

void MainWindow::SetClientWidth( unsigned int width)
{
    // code
    m_width = width;
}

void MainWindow::SetClientHeight( unsigned int height)
{
    // code
    m_height = height;
}


LRESULT CALLBACK MainWindow::MessageCallback( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // code
    MainWindow *window = reinterpret_cast<MainWindow *>( GetWindowLongPtr( hwnd, GWLP_USERDATA));

    if( window)
    {
        for( WndProcedureCallback windowProc : window->m_windowEventCallbacks)
        {
            windowProc( hwnd, message, wParam, lParam);
        }
    }

    switch( message)
    {
        case WM_SETFOCUS:
            if( window)
            {
                window->m_bActiveWindow = true;
            }
        break;

        case WM_KILLFOCUS:
            if( window)
            {
                window->m_bActiveWindow = false;
            }
        break;

        case WM_ERASEBKGND:
        return 0;

        case WM_SIZE:
        {
            unsigned int cx = LOWORD( lParam);
            unsigned int cy = HIWORD( lParam);

            if( window)
            {
                window->SetClientWidth( cx);
                window->SetClientHeight( cy);

                for( OnResizeEvent resizeCallback : window->m_resizeEventCallbacks)
                {
                    resizeCallback( cx, cy);
                }
            }
        }
        break;

        case WM_MOUSEMOVE:
        {
            int mouseX = LOWORD( lParam);
            int mouseY = HIWORD( lParam);

            if( window)
            {
                for( OnMouseMoveEvent mouseMoveCallback : window->m_mouseEventCallbacks)
                {
                    mouseMoveCallback( mouseX, mouseY, wParam);
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

        case WM_DESTROY:
            PostQuitMessage(0);
        break;
    }

    return DefWindowProc( hwnd, message, wParam, lParam);

}
