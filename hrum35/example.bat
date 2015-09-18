@echo off
mkdir test

echo /*---------------------------------------------------------------------------*/
echo /* pack source file */
hrum35 -start 32768 -depackto 32768 -ei hrum35.exe test/src_8000.bin
hrum35 -start 49152 -depackto 25000 -ei -sp 25000 -jp 25000 hrum35.exe test/src_c000.bin

echo /*---------------------------------------------------------------------------*/
