CL.exe /EHsc /c /I"%CUDA_PATH%\include" Source.cpp

LINK.exe /OUT:Source.exe /LIBPATH:"%CUDA_PATH%\lib\x64" opencl.lib Source.obj

DEL Source.obj
