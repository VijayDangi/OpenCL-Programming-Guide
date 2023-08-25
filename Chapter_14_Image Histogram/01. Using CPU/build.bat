CL.exe /EHsc /c Source.cpp

LINK.exe /OUT:Source.exe "../../Common/FreeImage/x64/FreeImage.lib" Source.obj

DEL Source.obj
