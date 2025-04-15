@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" >nul
rem now launch VS Code in a separate process so that the dev cmd shell is available to use
start "" code .


