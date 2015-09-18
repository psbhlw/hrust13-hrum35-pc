@echo off
mkdir test

echo /*---------------------------------------------------------------------------*/
echo /* pack source file with different -spd... */
hrust13 -spd 0 hrust13.exe test/src_spd0.bin
hrust13 -spd 1 hrust13.exe test/src_spd1.bin
hrust13 -spd 2 hrust13.exe test/src_spd2.bin
hrust13 -spd 3 hrust13.exe test/src_spd3.bin

echo /*---------------------------------------------------------------------------*/
echo /* pack source file with depacker */
hrust13 -spd 3 -depacker -start 24576 -depackto 49152 -di -sp 24576 -jp 49152 hrust13.exe test/src_full.bin

echo /*---------------------------------------------------------------------------*/
