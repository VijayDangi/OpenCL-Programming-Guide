/**
 * @author : Vijaykumar Dangi
 * @date   : 10-Aug-2023
 */

#include <Windows.h>

#include <iostream>
#include <stdio.h>

#include <fstream>
#include <sstream>

#include <d3d11.h>
#include <d3dcompiler.h>

#include <CL/cl.h>
#include <CL/cl_d3d11.h>

#define VERTEX_COUNT 256
#define TEXTURE_SIZE 256

#define RELEASE_CL_OBJECT( obj, release_func) \
    if(obj) \
    {   \
        release_func(obj);    \
        obj = nullptr;  \
    }

#define COM_INTERFACE_SAFE_RELEASE( Iptr) \
    if( Iptr) \
    {   \
        (Iptr)->Release();  \
        (Iptr) = nullptr;   \
    }

// type
clCreateFromD3D11BufferKHR_fn fnPtr_clCreateFromD3D11Buffer = nullptr;
clCreateFromD3D11Texture2DKHR_fn fnPtr_clCreateFromD3D11Texture2D = nullptr;
clEnqueueAcquireD3D11ObjectsKHR_fn  fnPtr_clEnqueueAcquireD3D11Objects = nullptr;
clEnqueueReleaseD3D11ObjectsKHR_fn  fnPtr_clEnqueueReleaseD3D11Objects = nullptr;

#define clCreateFromD3D11Buffer fnPtr_clCreateFromD3D11Buffer
#define clCreateFromD3D11Texture2D fnPtr_clCreateFromD3D11Texture2D
#define clEnqueueAcquireD3D11Objects fnPtr_clEnqueueAcquireD3D11Objects
#define clEnqueueReleaseD3D11Objects fnPtr_clEnqueueReleaseD3D11Objects


// Global function declaration
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM);

// Global variable declaration
    //Window
HWND g_hwnd;

DWORD style = 0;
WINDOWPLACEMENT wpPrev = { sizeof( WINDOWPLACEMENT) };

int g_window_width = 800;
int g_window_height = 600;

bool g_b_active_window = false;
bool g_b_fullscreen = false;
bool b_done = false;

    // Direct3D 11
IDXGISwapChain *g_pIDXGISwapChain = nullptr;
ID3D11Device *g_pID3D11Device = nullptr;
ID3D11DeviceContext *g_pID3D11DeviceContext = nullptr;

ID3D11RenderTargetView *g_pID3D11RenderTargetView = nullptr;

ID3D11VertexShader *g_pID3D11VertexShader = nullptr;
ID3D11PixelShader *g_pID3D11PixelShader = nullptr;
ID3D11InputLayout *g_pID3D11InputLayout = nullptr;

ID3D11Buffer *g_pID3D11Buffer_SinWave = nullptr;
ID3D11Buffer *g_pID3D11Buffer_Quad = nullptr;
ID3D11Buffer *g_pID3D11Buffer_ConstantBuffer = nullptr;

ID3D11Texture2D *g_pID3D11Texture2D = nullptr;
ID3D11ShaderResourceView *g_pID3D11ShaderResourceView = nullptr;
ID3D11SamplerState *g_pID3D11SamplerState = nullptr;

struct ConstantBuffer
{
    float u_color[4];
    int u_use_texture[4];   // 1 used, 3 unused => to keep multiple of 16
};

    // OpenCL
cl_context ocl_context = nullptr;
cl_command_queue ocl_command_queue = nullptr;
cl_device_id ocl_device_id = nullptr;
cl_program ocl_program = nullptr;
cl_kernel ocl_sin_kernel = nullptr;
cl_kernel ocl_texture_kernel = nullptr;
cl_mem ocl_d3d11_line_buffer = nullptr;
cl_mem ocl_d3d11_texture = nullptr;

/**
 * @brief main()
 */
int main( int argc, char *argv[])
{
    // function declaration
    bool InitializeWindow( HWND *hwnd);
    bool InitializeDirect3D11( HWND hwnd);
    bool InitializeOpenCL();
    bool Initialize();

    void Render();
    void Update();

    void Uninitialize();

    // variable declaration
    MSG msg;

    // code
    if( InitializeWindow( &g_hwnd) == false)
    {
        std::cerr << "InitializeWindow() Failed." << std::endl;
        Uninitialize();
        return 1;
    }


    if( InitializeDirect3D11( g_hwnd) == false)
    {
        std::cerr << "InitializeDirect3D11() Failed." << std::endl;
        Uninitialize();
        return 1;
    }


    if( InitializeOpenCL() == false)
    {
        std::cerr << "InitializeOpenCL() Failed." << std::endl;
        Uninitialize();
        return 1;
    }


    if( Initialize() == false)
    {
        std::cerr << "Initialize() Failed." << std::endl;
        Uninitialize();
        return 1;
    }


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
 * @brief ComErrorDescriptionString()
 */
void ComErrorDescriptionString( HRESULT hr, char *str)
{
	if(!str)
	{
		return;
	}

	char *szErrorMessage = nullptr;

	if( FACILITY_WINDOWS == HRESULT_FACILITY(hr))
	{
		hr = HRESULT_CODE( hr);
	}

	DWORD result = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		hr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR) &szErrorMessage,
		0, 
		NULL
	);
	if( result != 0)
	{
		sprintf( str, "%s", szErrorMessage);
		LocalFree( szErrorMessage);
	}
	else
	{
		sprintf( str, "Unknown COM Error. %#x", hr);
	}
}

/**
 * @brief Resize()
 */
void Resize( int width, int height)
{
    // variable declaration
    HRESULT hr;

    // code
    if( height == 0)
    {
        height = 1;
    }

    if( !g_pIDXGISwapChain)
    {
        return;
    }

    COM_INTERFACE_SAFE_RELEASE( g_pID3D11RenderTargetView);

        // resize color buffre
    hr = g_pIDXGISwapChain->ResizeBuffers( 1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    if( FAILED( hr))
    {
        char str[4096];
        ComErrorDescriptionString( hr, str);
        printf( "ResizeBuffers() Failed %#x: %s.\n", hr, str);
        return;
    }

    ID3D11Texture2D *pBackBuffer = nullptr;
    hr = g_pIDXGISwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D), ( void **)&pBackBuffer);
    if( FAILED( hr))
    {
        std::cout << "GetBuffer() Failed." << std::endl;
        return;
    }

        // create RTV
    hr = g_pID3D11Device->CreateRenderTargetView( pBackBuffer, nullptr, &g_pID3D11RenderTargetView);
    if( FAILED( hr))
    {
        std::cout << "CreateRenderTargetView() Failed." << std::endl;

        COM_INTERFACE_SAFE_RELEASE( pBackBuffer);
        return;
    }

    COM_INTERFACE_SAFE_RELEASE( pBackBuffer);
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
    TCHAR szClassName[] = TEXT("D3D11_CL_Interop");
    HINSTANCE hInstance = GetModuleHandle( nullptr);

    // code
    if( !hwnd)
    {
        std::cerr << "Null pointer pass." << std::endl;
        return false;
    }


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


    if( !RegisterClassEx( &wndclass))
    {
        std::cerr << "Window Class not register." << std::endl;
        return false;
    }


    *hwnd = CreateWindowEx(
                WS_EX_APPWINDOW,
                szClassName, TEXT("OpenCL-Direct3D11 Interoperation"),
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
                100, 100, g_window_width, g_window_height,
                nullptr, nullptr, hInstance, nullptr
    );


    SetForegroundWindow( *hwnd);
    SetFocus( *hwnd);

    ShowWindow( *hwnd, SW_NORMAL);

    return true;
}

/**
 * @brief InitializeWindow()
 */
bool InitializeDirect3D11( HWND hwnd)
{
    // variable declaration
    HRESULT hr;
    RECT window_rect;

    D3D_FEATURE_LEVEL d3d_feature_level_require = D3D_FEATURE_LEVEL_11_0;
    D3D_FEATURE_LEVEL d3d_feature_level_acquire = D3D_FEATURE_LEVEL_10_1;

    D3D_DRIVER_TYPE driver_type[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE
    };

    D3D_DRIVER_TYPE driver_type_selected;

    // code
    if( hwnd == nullptr)
    {
        std::cerr << "Invalid HWND." << std::endl;
        return false;
    }

    GetClientRect( hwnd, &window_rect);

        // Swap Chain
    DXGI_SWAP_CHAIN_DESC dxgi_swap_chain_desc = { 0};
    ZeroMemory( &dxgi_swap_chain_desc, sizeof( dxgi_swap_chain_desc));

    dxgi_swap_chain_desc.BufferCount = 1;

    dxgi_swap_chain_desc.BufferDesc.Width = window_rect.right - window_rect.left;
    dxgi_swap_chain_desc.BufferDesc.Height = window_rect.bottom - window_rect.top;
    dxgi_swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    dxgi_swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
    dxgi_swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;

    dxgi_swap_chain_desc.SampleDesc.Count = 1;
    dxgi_swap_chain_desc.SampleDesc.Quality = 0;

    dxgi_swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgi_swap_chain_desc.OutputWindow = hwnd;
    dxgi_swap_chain_desc.Windowed = TRUE;

    int num_driver_type = _ARRAYSIZE( driver_type);
    for( int index = 0; index < num_driver_type; ++index)
    {
        driver_type_selected = driver_type[ index];

        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            driver_type_selected,
            nullptr,
            0,
            &d3d_feature_level_require,
            1,
            D3D11_SDK_VERSION,
            &dxgi_swap_chain_desc,
            &g_pIDXGISwapChain,
            &g_pID3D11Device,
            &d3d_feature_level_acquire,
            &g_pID3D11DeviceContext
        );

        if( SUCCEEDED( hr))
        {
            break;
        }
    }

    if( FAILED(hr))
    {
        std::cerr << "D3D11CreateDeviceAndSwapChain() Failed." << std::endl;
        return false;
    }

    if( driver_type_selected == D3D_DRIVER_TYPE_HARDWARE)
    {
        std::cerr << "D3D_DRIVER_TYPE_HARDWARE type selected." << std::endl;
    }
    else if( driver_type_selected == D3D_DRIVER_TYPE_WARP)
    {
        std::cerr << "D3D_DRIVER_TYPE_WARP type selected." << std::endl;
    }
    else if( driver_type_selected == D3D_DRIVER_TYPE_REFERENCE)
    {
        std::cerr << "D3D_DRIVER_TYPE_REFERENCE type selected." << std::endl;
    }
    else
    {
        std::cerr << "unknown driver type selected." << std::endl;
    }

    Resize( window_rect.right - window_rect.left, window_rect.bottom - window_rect.top);

    return true;
}

/**
 * @brief InitializeOpenCL()
 */
bool InitializeOpenCL()
{
    // function declaration
    cl_context CreateContext( cl_platform_id *ocl_platform);
    cl_command_queue CreateCommandQueue( cl_context ocl_context, cl_device_id *ocl_device);


    // variable declaration
    cl_platform_id ocl_platform = nullptr;

    // code
        // create context
    ocl_context = CreateContext( &ocl_platform);
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
    

    size_t string_size = 0;
    clGetPlatformInfo( ocl_platform, CL_PLATFORM_NAME, 0, nullptr, &string_size);

    char *str = new char[string_size];
    clGetPlatformInfo( ocl_platform, CL_PLATFORM_NAME, string_size, str, nullptr);

    std::cout << str << std::endl;
    delete str;

        // Initialize Function pointer for D3D11-OpenCL Interop
    fnPtr_clCreateFromD3D11Buffer = (clCreateFromD3D11BufferKHR_fn) clGetExtensionFunctionAddress( "clCreateFromD3D11BufferNV");
    if( fnPtr_clCreateFromD3D11Buffer == nullptr)
    {
        fnPtr_clCreateFromD3D11Buffer = (clCreateFromD3D11BufferKHR_fn) clGetExtensionFunctionAddress( "clCreateFromD3D11BufferKHR");
        if( fnPtr_clCreateFromD3D11Buffer == nullptr)
        {
            std::cerr << "clGetExtensionFunctionAddress() Failed for clCreateFromD3D11Buffer()." << std::endl;
            return false;
        }
    }

    fnPtr_clCreateFromD3D11Texture2D = (clCreateFromD3D11Texture2DKHR_fn) clGetExtensionFunctionAddressForPlatform( ocl_platform, "clCreateFromD3D11Texture2DNV");
    if( fnPtr_clCreateFromD3D11Texture2D == nullptr)
    {
        fnPtr_clCreateFromD3D11Texture2D = (clCreateFromD3D11Texture2DKHR_fn) clGetExtensionFunctionAddressForPlatform( ocl_platform, "clCreateFromD3D11Texture2DKHR");
        if( fnPtr_clCreateFromD3D11Texture2D == nullptr)
        {
            std::cerr << "clGetExtensionFunctionAddressForPlatform() Failed for clCreateFromD3D11Texture2D()." << std::endl;
            return false;
        }
    }

    fnPtr_clEnqueueAcquireD3D11Objects = (clEnqueueAcquireD3D11ObjectsKHR_fn) clGetExtensionFunctionAddressForPlatform( ocl_platform, "clEnqueueAcquireD3D11ObjectsNV");
    if( fnPtr_clEnqueueAcquireD3D11Objects == nullptr)
    {
        fnPtr_clEnqueueAcquireD3D11Objects = (clEnqueueAcquireD3D11ObjectsKHR_fn) clGetExtensionFunctionAddressForPlatform( ocl_platform, "clEnqueueAcquireD3D11ObjectsKHR");
        if( fnPtr_clEnqueueAcquireD3D11Objects == nullptr)
        {
            std::cerr << "clGetExtensionFunctionAddressForPlatform() Failed for clEnqueueAcquireD3D11Objects()." << std::endl;
            return false;
        }
    }

    fnPtr_clEnqueueReleaseD3D11Objects = (clEnqueueReleaseD3D11ObjectsKHR_fn) clGetExtensionFunctionAddressForPlatform( ocl_platform, "clEnqueueReleaseD3D11ObjectsNV");
    if( fnPtr_clEnqueueReleaseD3D11Objects == nullptr)
    {
        fnPtr_clEnqueueReleaseD3D11Objects = (clEnqueueReleaseD3D11ObjectsKHR_fn) clGetExtensionFunctionAddressForPlatform( ocl_platform, "clEnqueueReleaseD3D11ObjectsKHR");
        if( fnPtr_clEnqueueReleaseD3D11Objects == nullptr)
        {
            std::cerr << "clGetExtensionFunctionAddressForPlatform() Failed for clEnqueueReleaseD3D11Objects()." << std::endl;
            return false;
        }
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
cl_context CreateContext( cl_platform_id *ocl_platform)
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
        *ocl_platform = nullptr;
        return nullptr;
    }

        // OpenCL context properties compatible with Direct3D 11 context
    cl_context_properties ocl_properties[] =
    {
        CL_CONTEXT_PLATFORM, ( cl_context_properties) ocl_first_plarform_id,
        CL_CONTEXT_D3D11_DEVICE_KHR, (cl_context_properties) g_pID3D11Device,
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

            *ocl_platform = nullptr;
            return nullptr;
        }
    }

    *ocl_platform = ocl_first_plarform_id;

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
        // Direct 3D 11
    ID3DBlob *pID3DBlob_VertexShaderCode = nullptr;
    ID3DBlob *pID3DBlob_PixelShaderCode = nullptr;
    ID3DBlob *pID3DBlob_Error = nullptr;

    HRESULT hr;

        // vertex shader
    hr = D3DCompileFromFile( L"display_shader/vertex_shader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", 0, 0, &pID3DBlob_VertexShaderCode, &pID3DBlob_Error);
    if( FAILED( hr))
    {
        if( pID3DBlob_Error)
        {
            std::cerr << "D3DCompileFromFile() Vertex Shader Failed: " << (char *) pID3DBlob_Error->GetBufferPointer() << std::endl;
            COM_INTERFACE_SAFE_RELEASE( pID3DBlob_Error);
        }
        else
        {
            std::cerr << "D3DCompileFromFile() Vertex Shader Failed." << std::endl;
        }

        COM_INTERFACE_SAFE_RELEASE( pID3DBlob_VertexShaderCode);
        return false;
    }
    else if( pID3DBlob_Error)
    {
        std::cerr << "Vertex Shader Log: " << (char *) pID3DBlob_Error->GetBufferPointer() << std::endl;

        COM_INTERFACE_SAFE_RELEASE( pID3DBlob_Error);
    }

    hr = g_pID3D11Device->CreateVertexShader( pID3DBlob_VertexShaderCode->GetBufferPointer(), pID3DBlob_VertexShaderCode->GetBufferSize(), nullptr, &g_pID3D11VertexShader);
    if( FAILED(hr))
    {
        std::cerr << "CreateVertexShader() Failed." << std::endl;
        COM_INTERFACE_SAFE_RELEASE( pID3DBlob_VertexShaderCode);

        return false;
    }


        // pixel shader
    hr = D3DCompileFromFile( L"display_shader/pixel_shader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", 0, 0, &pID3DBlob_PixelShaderCode, &pID3DBlob_Error);
    if( FAILED( hr))
    {
        if( pID3DBlob_Error)
        {
            std::cerr << "D3DCompileFromFile() Pixel Shader Failed: " << (char *) pID3DBlob_Error->GetBufferPointer() << std::endl;
            COM_INTERFACE_SAFE_RELEASE( pID3DBlob_Error);
        }
        else
        {
            std::cerr << "D3DCompileFromFile() Pixel Shader Failed." << std::endl;
        }

        COM_INTERFACE_SAFE_RELEASE( pID3DBlob_VertexShaderCode);
        COM_INTERFACE_SAFE_RELEASE( pID3DBlob_PixelShaderCode);
        return false;
    }
    else if( pID3DBlob_Error)
    {
        std::cerr << "Pixel Shader Log: " << (char *) pID3DBlob_Error->GetBufferPointer() << std::endl;

        COM_INTERFACE_SAFE_RELEASE( pID3DBlob_Error);
    }

    hr = g_pID3D11Device->CreatePixelShader( pID3DBlob_PixelShaderCode->GetBufferPointer(), pID3DBlob_PixelShaderCode->GetBufferSize(), nullptr, &g_pID3D11PixelShader);
    if( FAILED(hr))
    {
        std::cerr << "CreateVertexShader() Failed." << std::endl;
        COM_INTERFACE_SAFE_RELEASE( pID3DBlob_VertexShaderCode);
        COM_INTERFACE_SAFE_RELEASE( pID3DBlob_PixelShaderCode);

        return false;
    }
    

        // input layout
    D3D11_INPUT_ELEMENT_DESC d3d_input_layout_desc = { 0};

    d3d_input_layout_desc.SemanticName = "POSITION";
    d3d_input_layout_desc.SemanticIndex = 0;
    d3d_input_layout_desc.Format = DXGI_FORMAT_R32G32_FLOAT;
    d3d_input_layout_desc.AlignedByteOffset = 0;
    d3d_input_layout_desc.InputSlot = 0;
    d3d_input_layout_desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    d3d_input_layout_desc.InstanceDataStepRate = 0;

    hr = g_pID3D11Device->CreateInputLayout( &d3d_input_layout_desc, 1, pID3DBlob_VertexShaderCode->GetBufferPointer(), pID3DBlob_VertexShaderCode->GetBufferSize(), &g_pID3D11InputLayout);
    if( FAILED( hr))
    {
        std::cerr << "CreateInputLayout() Failed." << std::endl;
        COM_INTERFACE_SAFE_RELEASE( pID3DBlob_VertexShaderCode);
        COM_INTERFACE_SAFE_RELEASE( pID3DBlob_PixelShaderCode);

        return false;
    }
    
    COM_INTERFACE_SAFE_RELEASE( pID3DBlob_VertexShaderCode);
    COM_INTERFACE_SAFE_RELEASE( pID3DBlob_PixelShaderCode);

        // constant buffer
    D3D11_BUFFER_DESC d3d_buffer_desc = { 0};

    d3d_buffer_desc.ByteWidth = sizeof( ConstantBuffer);
    d3d_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    d3d_buffer_desc.CPUAccessFlags = 0;
    d3d_buffer_desc.Usage = D3D11_USAGE_DEFAULT;

    hr = g_pID3D11Device->CreateBuffer( &d3d_buffer_desc, nullptr, &g_pID3D11Buffer_ConstantBuffer);
    if( FAILED( hr))
    {
        std::cerr << "CreateBuffer() Failed for Constant Buffer." << std::endl;
        return false;
    }


        // wave line position
    ZeroMemory( &d3d_buffer_desc, sizeof( d3d_buffer_desc));
    d3d_buffer_desc.ByteWidth = VERTEX_COUNT * 2 * sizeof( float);
    d3d_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    d3d_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    d3d_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;

    hr = g_pID3D11Device->CreateBuffer( &d3d_buffer_desc, nullptr, &g_pID3D11Buffer_SinWave);
    if( FAILED( hr))
    {
        std::cerr << "CreateBuffer() Failed for Wave line Buffer." << std::endl;
        return false;
    }

        // quad buffer
    float quad_position[] = 
    {
         1.0f,  1.0f,
        -1.0f, -1.0f,
        -1.0f,  1.0f,

         1.0f,  1.0f,
         1.0f, -1.0f,
         -1.0f, -1.0f
    };

    ZeroMemory( &d3d_buffer_desc, sizeof( d3d_buffer_desc));
    d3d_buffer_desc.ByteWidth = sizeof( quad_position);
    d3d_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    d3d_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    d3d_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;

    D3D11_SUBRESOURCE_DATA d3d_subresource = { 0};
    d3d_subresource.pSysMem = quad_position;

    hr = g_pID3D11Device->CreateBuffer( &d3d_buffer_desc, &d3d_subresource, &g_pID3D11Buffer_Quad);
    if( FAILED( hr))
    {
        std::cerr << "CreateBuffer() Failed for Quad Buffer." << std::endl;
        return false;
    }

        // texture
    D3D11_TEXTURE2D_DESC d3d_texture_2d = { 0};

    d3d_texture_2d.Width = TEXTURE_SIZE;
    d3d_texture_2d.Height = TEXTURE_SIZE;
    d3d_texture_2d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    d3d_texture_2d.Usage = D3D11_USAGE_DEFAULT;
    d3d_texture_2d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    d3d_texture_2d.SampleDesc.Count = 1;
    d3d_texture_2d.SampleDesc.Quality = 0;
    d3d_texture_2d.MipLevels = 1;
    d3d_texture_2d.ArraySize = 1;
    d3d_texture_2d.CPUAccessFlags = 0;
    d3d_texture_2d.MiscFlags = 0;

    hr = g_pID3D11Device->CreateTexture2D( &d3d_texture_2d, nullptr, &g_pID3D11Texture2D);
    if( FAILED( hr))
    {
        std::cerr << "CreateTexture2D() Failed." << std::endl;
        return false;
    }

        // Shader  Resource view
    hr = g_pID3D11Device->CreateShaderResourceView( g_pID3D11Texture2D, nullptr, &g_pID3D11ShaderResourceView);
    if( FAILED( hr))
    {
        std::cerr << "CreateShaderResourceView() Failed." << std::endl;
        return false;
    }

        // sampler
    D3D11_SAMPLER_DESC d3d_sampler_desc;
    ZeroMemory( &d3d_sampler_desc, sizeof( d3d_sampler_desc));

    d3d_sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    d3d_sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    d3d_sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    d3d_sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

    hr = g_pID3D11Device->CreateSamplerState( &d3d_sampler_desc, &g_pID3D11SamplerState);
    if( FAILED( hr))
    {
        std::cerr << "CreateSamplerState() Failed." << std::endl;
        return false;
    }

    /***********************************************************************/
    cl_int ocl_err = CL_SUCCESS;

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

        // OpenCL buffer
    ocl_d3d11_line_buffer = clCreateFromD3D11Buffer( ocl_context, CL_MEM_READ_WRITE, g_pID3D11Buffer_SinWave, &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clCreateFromD3D11Buffer() Failed." << std::endl;
        return false;
    }

    ocl_d3d11_texture = clCreateFromD3D11Texture2D( ocl_context, CL_MEM_READ_WRITE, g_pID3D11Texture2D, 0, &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clCreateFromD3D11Texture2D() Failed." << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief Initialize()
 */
void Render()
{
    // variable declaration
    D3D11_VIEWPORT d3d_viewport = { 0};

    float clear_color[] = { 1.0f, 0.0f, 0.0f, 1.0f};

    ConstantBuffer cbuffer = { 0};

    // code
    d3d_viewport.TopLeftX = 0;
    d3d_viewport.TopLeftY = 0;
    d3d_viewport.Width = ( float) g_window_width;
    d3d_viewport.Height = ( float) g_window_height;
    d3d_viewport.MinDepth = 0.0f;
    d3d_viewport.MaxDepth = 1.0;


    g_pID3D11DeviceContext->OMSetRenderTargets( 1, &g_pID3D11RenderTargetView, nullptr);
    g_pID3D11DeviceContext->ClearRenderTargetView( g_pID3D11RenderTargetView, clear_color);

    g_pID3D11DeviceContext->VSSetShader( g_pID3D11VertexShader, nullptr, 0);
    g_pID3D11DeviceContext->PSSetShader( g_pID3D11PixelShader, nullptr, 0);
    g_pID3D11DeviceContext->IASetInputLayout( g_pID3D11InputLayout);
    g_pID3D11DeviceContext->PSSetConstantBuffers( 0, 1, &g_pID3D11Buffer_ConstantBuffer);
        
        // texture
        cbuffer.u_use_texture[0] = 1;
        g_pID3D11DeviceContext->UpdateSubresource( g_pID3D11Buffer_ConstantBuffer, 0, nullptr, &cbuffer, 0, 0);

        // vertex buffer
        UINT stride = 2 * sizeof( float);
        UINT offset = 0;
        g_pID3D11DeviceContext->IASetVertexBuffers( 0, 1, &g_pID3D11Buffer_Quad, &stride, &offset);
        
        // texture
        g_pID3D11DeviceContext->PSSetShaderResources( 0, 1, &g_pID3D11ShaderResourceView);
        g_pID3D11DeviceContext->PSSetSamplers( 0, 1, &g_pID3D11SamplerState);

        //draw
        g_pID3D11DeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_pID3D11DeviceContext->Draw( 6, 0);




        // wave
        cbuffer.u_color[0] = 1.0f;
        cbuffer.u_color[1] = 1.0f;
        cbuffer.u_color[2] = 1.0f;
        cbuffer.u_color[3] = 1.0f;

        cbuffer.u_use_texture[0] = 0;
        g_pID3D11DeviceContext->UpdateSubresource( g_pID3D11Buffer_ConstantBuffer, 0, nullptr, &cbuffer, 0, 0);

        // vertex buffer
        stride = 2 * sizeof( float);
        offset = 0;
        g_pID3D11DeviceContext->IASetVertexBuffers( 0, 1, &g_pID3D11Buffer_SinWave, &stride, &offset);

        //draw
        g_pID3D11DeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
        g_pID3D11DeviceContext->Draw( VERTEX_COUNT, 0);



    g_pID3D11DeviceContext->VSSetShader( nullptr, nullptr, 0);
    g_pID3D11DeviceContext->PSSetShader( nullptr, nullptr, 0);
    g_pID3D11DeviceContext->IASetInputLayout( nullptr);
    g_pID3D11DeviceContext->PSSetConstantBuffers( 0, 0, nullptr);
    g_pID3D11DeviceContext->IASetVertexBuffers( 0, 0, nullptr, nullptr, nullptr);

    g_pID3D11DeviceContext->RSSetViewports( 1, &d3d_viewport);
    
    g_pIDXGISwapChain->Present( 1, 0);
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

    ocl_err = clSetKernelArg( ocl_sin_kernel, 0, sizeof( cl_mem), &ocl_d3d11_line_buffer);
    ocl_err |= clSetKernelArg( ocl_sin_kernel, 1, sizeof( cl_int), &size);
    ocl_err |= clSetKernelArg( ocl_sin_kernel, 2, sizeof( cl_int), &size);
    ocl_err |= clSetKernelArg( ocl_sin_kernel, 3, sizeof( cl_int), &seq);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clSetKernelArgs() Failed." << std::endl;
        b_done = true;

        return;
    }

    size_t global_work_size[1] = { VERTEX_COUNT};
    size_t local_wok_size[1] = { 16};

    ocl_err = clEnqueueAcquireD3D11Objects( ocl_command_queue, 1, &ocl_d3d11_line_buffer, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueAcquireD3D11Objects() Failed." << std::endl;
        b_done = true;

        return;
    }

    ocl_err = clEnqueueNDRangeKernel( ocl_command_queue, ocl_sin_kernel, 1, nullptr, global_work_size, local_wok_size, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueNDRangeKernel() Failed." << std::endl;
        b_done = true;

        return;
    }

    ocl_err = clEnqueueReleaseD3D11Objects( ocl_command_queue, 1, &ocl_d3d11_line_buffer, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueReleaseD3D11Objects() Failed." << std::endl;
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

    ocl_err = clSetKernelArg( ocl_texture_kernel, 0, sizeof( cl_mem), &ocl_d3d11_texture);
    ocl_err |= clSetKernelArg( ocl_texture_kernel, 1, sizeof( cl_int), &size);
    ocl_err |= clSetKernelArg( ocl_texture_kernel, 2, sizeof( cl_int), &size);
    ocl_err |= clSetKernelArg( ocl_texture_kernel, 3, sizeof( cl_int), &seq);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clSetKernelArgs() Failed." << std::endl;
        b_done = true;

        return;
    }

    size_t global_work_size[2] = { TEXTURE_SIZE, TEXTURE_SIZE};
    size_t local_wok_size[2] = { 32, 4};

    ocl_err = clEnqueueAcquireD3D11Objects( ocl_command_queue, 1, &ocl_d3d11_texture, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueAcquireD3D11Objects() Failed." << std::endl;
        b_done = true;

        return;
    }

    ocl_err = clEnqueueNDRangeKernel( ocl_command_queue, ocl_texture_kernel, 2, nullptr, global_work_size, local_wok_size, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueNDRangeKernel() Failed." << std::endl;
        b_done = true;

        return;
    }

    ocl_err = clEnqueueReleaseD3D11Objects( ocl_command_queue, 1, &ocl_d3d11_texture, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueReleaseD3D11Objects() Failed." << std::endl;
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
    RELEASE_CL_OBJECT( ocl_d3d11_line_buffer, clReleaseMemObject);
    RELEASE_CL_OBJECT( ocl_d3d11_texture, clReleaseMemObject);
    

        // *************************** Direct3D 11
    COM_INTERFACE_SAFE_RELEASE( g_pIDXGISwapChain);
    COM_INTERFACE_SAFE_RELEASE( g_pID3D11Device);
    COM_INTERFACE_SAFE_RELEASE( g_pID3D11DeviceContext);

    COM_INTERFACE_SAFE_RELEASE( g_pID3D11RenderTargetView);

    COM_INTERFACE_SAFE_RELEASE( g_pID3D11VertexShader);
    COM_INTERFACE_SAFE_RELEASE( g_pID3D11PixelShader);
    COM_INTERFACE_SAFE_RELEASE( g_pID3D11InputLayout);

    COM_INTERFACE_SAFE_RELEASE( g_pID3D11Buffer_SinWave);
    COM_INTERFACE_SAFE_RELEASE( g_pID3D11Buffer_Quad);
    COM_INTERFACE_SAFE_RELEASE( g_pID3D11Buffer_ConstantBuffer);

    COM_INTERFACE_SAFE_RELEASE( g_pID3D11Texture2D);
    COM_INTERFACE_SAFE_RELEASE( g_pID3D11ShaderResourceView);
    COM_INTERFACE_SAFE_RELEASE( g_pID3D11SamplerState);
    

        // ************************** Window
    if( g_b_fullscreen)
    {
        ToggleFullScreen();
    }

    if( g_hwnd)
    {
        DestroyWindow( g_hwnd);
        g_hwnd = nullptr;
    }
}
