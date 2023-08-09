CL.exe /EHsc /c /I"%CUDA_PATH%\include" HelloWorld.cpp

LINK.exe /OUT:HelloWorld.exe /LIBPATH:"%CUDA_PATH%\lib\x64" opencl.lib HelloWorld.obj

DEL HelloWorld.obj
