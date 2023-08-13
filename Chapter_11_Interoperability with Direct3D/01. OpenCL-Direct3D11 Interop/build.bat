CL.exe /EHsc /c /I"%CUDA_PATH%\include" D3D11Interop.cpp

LINK.exe /OUT:D3D11Interop.exe /LIBPATH:"%CUDA_PATH%\lib\x64" user32.lib gdi32.lib dxgi.lib d3d11.lib d3dcompiler.lib opencl.lib D3D11Interop.obj

DEL D3D11Interop.obj
