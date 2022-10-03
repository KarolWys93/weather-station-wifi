@ECHO OFF
set server_dir_name=server_test
set server_dir=%server_dir_name%\

if exist %server_dir% (
    ECHO Remove old directory
	rmdir /S /Q %server_dir_name%
)

ECHO Create server directory
mkdir %server_dir_name%
ECHO Copy files
xcopy /E /Q html\* %server_dir%
xcopy /E /Q api_mock\* %server_dir%
ECHO Server start
python.exe -m http.server -d %server_dir%
