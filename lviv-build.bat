@echo off
rem GTA LVIV BETA: non-interactive build. Usage: lviv-build.bat [Release^|Nightly]
rem Limited parallelism: full -maxCpuCount runs out of memory on 16 GB machines.
cd /d "D:\zborka 41\mtasa-1.6"
set NoDefaultCurrentDirectoryInExePath=
set "PATH=.;%PATH%"
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Release
if not exist Build\MTASA.sln call win-create-projects.bat < nul
for /f "usebackq tokens=1* delims=: " %%i in (`utils\vswhere.exe -latest -requires Microsoft.Component.MSBuild`) do (
    if /i "%%i"=="installationPath" set INSTALLDIR=%%j
)
"%INSTALLDIR%\MSBuild\Current\Bin\MSBuild.exe" Build\MTASA.sln -property:Configuration=%CONFIG% -property:Platform=Win32 -maxCpuCount:3 -property:CL_MPCount=3 -nologo -verbosity:minimal -nodeReuse:false
if errorlevel 1 exit /b 1
if not exist Bin\MTA\netc.dll call win-install-data.bat < nul
exit /b 0