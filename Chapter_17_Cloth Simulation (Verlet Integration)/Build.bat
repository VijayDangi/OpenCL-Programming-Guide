
DEL App.exe

RC.exe Resource.rc

CL.exe /EHsc /c /I"%CUDA_PATH%\include" /I"glew\include" OGL.cpp^
 Keyboard.cpp^
 LoadShaders.cpp^
 TextureLoading.cpp^
 Grid.cpp^
 ExMaths.cpp^
 OpenCLUtil.cpp ^
 Cloth_CL.cpp

LINK.exe /OUT:App.exe /LIBPATH:"%CUDA_PATH%\lib\x64" /LIBPATH:"glew\lib\Release\x64" opencl.lib ^
 OGL.obj^
 Keyboard.obj^
 LoadShaders.obj^
 TextureLoading.obj^
 Grid.obj^
 ExMaths.obj^
 OpenCLUtil.obj ^
 Cloth_CL.obj ^
 Resource.res

DEL OGL.obj ^
  Keyboard.obj ^
  LoadShaders.obj ^
  TextureLoading.obj ^
  Grid.obj ^
  ExMaths.obj ^
  OpenCLUtil.obj ^
  Cloth_CL.obj ^
  Resource.res
