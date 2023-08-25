CL.exe /EHsc /c /I"%CUDA_PATH%\include" Source.cpp OpenCLUtil.cpp

LINK.exe /OUT:Source.exe /LIBPATH:"%CUDA_PATH%\lib\x64" opencl.lib "../../../Common/FreeImage/x64/FreeImage.lib" Source.obj OpenCLUtil.obj

DEL Source.obj OpenCLUtil.obj
