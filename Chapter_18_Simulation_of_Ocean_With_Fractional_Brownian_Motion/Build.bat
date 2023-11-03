@echo off 
 
set build_type="RELEASE" 
set exe_type="WINDOWS" 
set macro_defines=/D _RELEASE /D RELEASE 
set subsystem_type_macro=/D _WINDOWS /D WINDOWS 
 
: parse command line argument 
 
for %%x in (%*) do (  
    if /i "%%x" == "DEBUG" ( 
        set macro_defines=/D _DEBUG /D DEBUG /MDd 
        set build_type="DEBUG" 
    ) 
 
    if /i "%%x" == "CONSOLE" ( 
        set subsystem_type_macro=/D _CONSOLE /D CONSOLE 
        set exe_type="CONSOLE" 
    ) 
) 
 
: Set GLEW Path 
@REM if /i "%1" == "DEBUG" (set GLEW_LIB_PATH="source\third_party\glew\lib\Debugd") else (set GLEW_LIB_PATH="source\third_party\glew\lib\Release\x64") 
set GLEW_LIB_PATH="source\third_party\glew\lib\Release\x64" 
set GLEW_INCLUDE_PATH="source\third_party\glew\include" 
 
 echo Deleting App.exe... 

DEL App.exe 
echo: 
 
echo "Compiling Resource file..." 
RC.exe source/Resource.rc 
 
echo: 
 
echo "Compiling Source file..." 
CL.exe /EHsc /c /I%GLEW_INCLUDE_PATH% ^
 /I"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.1\include" ^
 %macro_defines% %subsystem_type_macro% /D UNICODE /D _UNICODE^
 source\ExMaths.cpp^
 source\framework\Buffer.cpp^
 source\framework\Framebuffer.cpp^
 source\framework\ResourceBuffer.cpp^
 source\framework\ShaderProgram.cpp^
 source\framework\Texture2D.cpp^
 source\framework\Texture2DMultisample.cpp^
 source\framework\TextureCubeMap.cpp^
 source\Grid.cpp^
 source\Main.cpp^
 source\post_process\Atmosphere.cpp^
 source\SceneWaves.cpp^
 source\system\FileIO.cpp^
 source\system\GLView.cpp^
 source\system\Keyboard.cpp^
 source\system\Logger.cpp^
 source\system\MainWindow.cpp^
 source\third_party\imgui\imgui.cpp^
 source\third_party\imgui\imgui_demo.cpp^
 source\third_party\imgui\imgui_draw.cpp^
 source\third_party\imgui\imgui_impl_opengl3.cpp^
 source\third_party\imgui\imgui_impl_win32.cpp^
 source\third_party\imgui\imgui_tables.cpp^
 source\third_party\imgui\imgui_widgets.cpp^
 source\utility\OpenCLUtil.cpp


echo: 
if %ERRORLEVEL% == 0 ( 
    echo Compilation Command Success 
) else (  
    goto :error 
) 
 
echo: 
echo Linking Project... 
echo: 
 
LINK.exe /OUT:App.exe /LIBPATH:%GLEW_LIB_PATH% ^
 /LIBPATH:"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.1\lib\x64" ^
 opencl.lib ^
 ExMaths.obj ^
 Buffer.obj ^
 Framebuffer.obj ^
 ResourceBuffer.obj ^
 ShaderProgram.obj ^
 Texture2D.obj ^
 Texture2DMultisample.obj ^
 TextureCubeMap.obj ^
 Grid.obj ^
 Main.obj ^
 Atmosphere.obj ^
 SceneWaves.obj ^
 FileIO.obj ^
 GLView.obj ^
 Keyboard.obj ^
 Logger.obj ^
 MainWindow.obj ^
 imgui.obj ^
 imgui_demo.obj ^
 imgui_draw.obj ^
 imgui_impl_opengl3.obj ^
 imgui_impl_win32.obj ^
 imgui_tables.obj ^
 imgui_widgets.obj ^
 OpenCLUtil.obj ^
 source\Resource.res 

if %ERRORLEVEL% == 0 ( 
    echo Linking Command Success 
) else ( 
    goto :error 
) 

echo: 
echo Cleaning up... 

DEL  ExMaths.obj^
 Buffer.obj^
 Framebuffer.obj^
 ResourceBuffer.obj^
 ShaderProgram.obj^
 Texture2D.obj^
 Texture2DMultisample.obj^
 TextureCubeMap.obj^
 Grid.obj^
 Main.obj^
 Atmosphere.obj^
 SceneWaves.obj^
 FileIO.obj^
 GLView.obj^
 Keyboard.obj^
 Logger.obj^
 MainWindow.obj^
 imgui.obj^
 imgui_demo.obj^
 imgui_draw.obj^
 imgui_impl_opengl3.obj^
 imgui_impl_win32.obj^
 imgui_tables.obj^
 imgui_widgets.obj^
 OpenCLUtil.obj^
 source\Resource.res 


echo: 
if /i %build_type% == "DEBUG" ( 
    echo "DEBUG mode enable" 
) else ( 
    echo "RELEASE mode enable" 
) 


if /i %exe_type% == "CONSOLE" ( 
    echo "Console based exe" 
) else ( 
    echo "Window based exe" 
) 


echo: 
echo Build Successfull 


:error 
if %ERRORLEVEL% NEQ 0 (echo "Build Failed.") 
@echo on 


