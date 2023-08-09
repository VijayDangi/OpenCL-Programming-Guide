CL.exe /EHsc /c /I"%CUDA_PATH%\include" Convolution.cpp

LINK.exe /OUT:Convolution.exe /LIBPATH:"%CUDA_PATH%\lib\x64" opencl.lib Convolution.obj

DEL Convolution.obj
