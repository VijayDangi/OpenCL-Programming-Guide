CL.exe /EHsc /c /I"%CUDA_PATH%\include" /I"../../Common/glew/include" GLInterop.cpp

LINK.exe /OUT:GLInterop.exe /LIBPATH:"%CUDA_PATH%\lib\x64" /LIBPATH:"../../Common/glew/lib/Release/x64" glew32.lib opencl.lib user32.lib gdi32.lib opengl32.lib GLInterop.obj

DEL GLInterop.obj
