@echo off
echo 1. C kodlari derleniyor (Assembler ^& Linker)...
mingw32-make clean
mingw32-make all

echo.
echo 2. Knight Rider test programi derleniyor (Assembly)...
if not exist output mkdir output
assembler_bin.exe test_programs/main.s test_programs/utils.s
linker_bin.exe test_programs/main.o test_programs/utils.o -o output/knight_rider --text-base 00000000 --data-base 00000300 --stack-top 00000400

echo.
echo [BASARILI] Tum adimlar tamamlandi. Ciktilar 'output/' klasorunde.
pause
