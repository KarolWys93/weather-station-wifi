@ECHO OFF
"C:\Program Files\Git\mingw64\bin\git.exe" describe --abbrev=5 --dirty --always --tags > g_ver.tmp
SET /p git_hver=<g_ver.tmp
del g_ver.tmp

echo GIT VERSION TAG: %git_hver%
echo /*> ..\Core\Inc\git_version.h
echo  * Auto-generated file>> ..\Core\Inc\git_version.h
echo  */>> ..\Core\Inc\git_version.h
echo #ifndef GIT_VERSION_H_>> ..\Core\Inc\git_version.h
echo #define GIT_VERSION_H_>> ..\Core\Inc\git_version.h
echo.>> ..\Core\Inc\git_version.h
echo #define __GIT_VERSION__ "%git_hver%">> ..\Core\Inc\git_version.h
echo.>> ..\Core\Inc\git_version.h
echo #endif /* GIT_VERSION_H_ */>> ..\Core\Inc\git_version.h
