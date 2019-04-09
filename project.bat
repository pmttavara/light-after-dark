@echo off
setlocal EnableDelayedExpansion

if not defined LIB (call "%programfiles(x86)%/Microsoft Visual Studio/2017/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 >NUL)
if not defined LIB (call "%programfiles(x86)%/Microsoft Visual Studio 15.0/VC/vcvarsall.bat" x64)
if not defined LIB (call "%programfiles(x86)%/Microsoft Visual Studio 14.0/VC/vcvarsall.bat" x64)
if errorlevel 1 goto done

for %%a in (%cd%) do call set name=%%~na

if [%3]==[final] (
set "args_metagen=-DMETAGEN -DMETAGEN_RELEASE"
) else (
set "args_metagen=-DMETAGEN"
)
if [%2]==[release] (
  if [%3]==[final] (
  set "args_cl_build=-O2 -DNDEBUG -DMETAGENERATED"
  set "args_link_build=-SUBSYSTEM:windows -OPT:REF,ICF=1024"
  ) else (
  set "args_cl_build=-O2 -DNDEBUG -Z7"
  set "args_link_build=-DEBUG -SUBSYSTEM:windows -OPT:REF,ICF=1024"
  )
) else (
  set "args_cl_build=-Od -D_DEBUG -Z7"
  set "args_link_build=-DEBUG -SUBSYSTEM:console"
)

set "args_cl=-nologo -W4 -WX -wd4201 -wd4244 -wd4189 -wd4456 -wd4100 -wd4457 -wd4127 -wd4505 -permissive- -utf-8 !args_cl_build! %cd%/first.c* -GS- -Gs999999 -Gm- -GR- -EHa- -Oi -MP -fp:fast"
set sdl_libs="kernel32.lib" "user32.lib" "gdi32.lib" "shell32.lib" "winmm.lib" "imm32.lib" "ole32.lib" "oleaut32.lib" "version.lib" "uuid.lib" "advapi32.lib"
set libs=!sdl_libs! -nodefaultlib "%cd%/lib/*.lib" "Comdlg32.lib" "msvcrt.lib"
set "args_link=!libs! !args_link_build! -INCREMENTAL:NO -stack:0x1000000,0x1000000"
if [%1]==[metagen] (
  cl !args_cl! !args_metagen! -link !args_link! -OUT:"./%name%_metagen.exe"
  if errorlevel 1 goto done
  del %name%_metagen.lib
  del %name%_metagen.exp
  del *.obj
  del vc*.pdb
  "./%name%_metagen.exe"
  if errorlevel 1 goto done
)
if [%1]==[build] (
  if not exist bin mkdir bin
  pushd bin
  cl !args_cl! -link !args_link! -OUT:"./%name%.exe"
  if errorlevel 1 goto done
  del %name%.lib
  del %name%.exp
  del *.obj
  del vc*.pdb
  popd
)
if [%1]==[run] (
  if not exist bin goto done
  pushd bin
  if [%2]==[debugger] (
    devenv "%name%.exe"
  ) else (
    "%name%.exe"
  )
  popd
)
if exist "%cd%/bin/Debug" rd -s -q "%cd%/bin/Debug"
:done
echo.
endlocal
