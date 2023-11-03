#pragma once

#include "MainWindow.h"
#include "../Common.h"

class GLView : public MainWindow
{
    public:
        ~GLView();

        static GLView *CreateGLView(
            HINSTANCE hInstance,
            unsigned int view_width, unsigned int view_height,
            int gl_major_version, int gl_minor_version, int context_flags = 0,
            TCHAR *icon_resource = IDI_APPLICATION);

        void MakeCurrentContext();
        HGLRC GetRenderingContext() { return m_pRC; }
        HDC GetDeviceContext() { return m_pDC; }

    private:
        GLView(
            TCHAR *className, TCHAR *title,
            HINSTANCE hInstance,
            unsigned int width, unsigned int height,
            TCHAR *icon_resource = IDI_APPLICATION
        );

        static HGLRC create_dummy_context( HWND hwnd, HDC hDC);
        static BOOL setup_pixel_format( MainWindow *window, HDC hDC);
        static BOOL set_opengl_context(
                MainWindow *window, HDC hDC, HGLRC *p_ret_context,
                int major_version = 4, int minor_version = 6,
                int context_profile = WGL_CONTEXT_CORE_PROFILE_BIT_ARB);

        HGLRC m_pRC{};
        HDC m_pDC{};
};
