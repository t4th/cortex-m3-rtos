@echo off
set keil_dir=d:\Keil_v5\UV4



if "%~1"=="clean" goto clean
if "%~1"=="c" goto clean

if "%~1"=="re" goto retranslate

if "%~1"=="debug" goto debug
if "%~1"=="d" goto debug

:build
    set param=-b -j0 -o"log.txt"
    set /a show_result=1
    goto run

:clean
    set param=-c -j0 -o"log.txt"
    set /a show_result=1
    goto run

    
:retranslate
    set param=-r -j0 -o"log.txt"
    set /a show_result=1
    goto run

    
:debug
    set param=-d
    set show_result=0
    goto run

:run
@echo on
%keil_dir%\UV4 %param% keil\rtos.uvprojx -t"stm32f103ze"

@echo off
if %show_result%==1 (
    type keil\log.txt
)