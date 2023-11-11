#pragma once

#include <Windows.h>
#include <vector>
#include <string>

typedef void(* OnMouseMoveEvent)( int x, int y, int button);
typedef void(* OnResizeEvent)( UINT width, UINT height);
typedef void(* WndProcedureCallback)( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

class MainWindow
{
    public:
        void operator=(const MainWindow&) = delete;

        MainWindow( TCHAR *className, TCHAR *title, HINSTANCE hInstance, unsigned int width, unsigned int height, TCHAR *icon_resource = IDI_APPLICATION);
        virtual ~MainWindow();

        void ShowWindow();

        HWND GetHandle();
        int GetClientWidth();
        int GetClientHeight();

        bool IsWindowActive() { return m_bActiveWindow; }

        void SetWindowTitle( TCHAR *title);
        void ToggleFullScreen();

        void RegisterOnMouseMoveCallback( OnMouseMoveEvent mouse_callback);
        void RegisterOnResizeCallback( OnResizeEvent resize_callback);
        void RegisterOnWindowCallback( WndProcedureCallback window_callback);

    private:
        MainWindow( const MainWindow&) = delete;

        static LRESULT CALLBACK MessageCallback( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        void CreateClass( TCHAR *icon_resource);
        void DestroyClass();
        void CreateMainWindow(TCHAR *title, unsigned int width, unsigned int height);
        void DestroyMainWindow();

        void SetClientWidth( unsigned int width);
        void SetClientHeight( unsigned int height);

        HINSTANCE m_hInstance;
        HWND m_window;
        int m_width;
        int m_height;

        ATOM m_windowClass;

#ifndef UNICODE
        std::string m_className;
#else
        std::wstring m_className;
#endif

        WINDOWPLACEMENT m_window_placement;
        bool m_bFullscreen = false;
        bool m_bActiveWindow = true;

        std::vector<OnMouseMoveEvent> m_mouseEventCallbacks;
        std::vector<OnResizeEvent> m_resizeEventCallbacks;
        std::vector<WndProcedureCallback> m_windowEventCallbacks;

};