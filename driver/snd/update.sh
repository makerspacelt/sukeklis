#!/bin/bash

echo 'доброе утро. Что. опять работать!' | text2wave -eval "(voice_msu_ru_nsh_clunits)" > bootup1.wav
aplay bootup1.wav &

echo 'Добрый день. на работу пришел!' | text2wave -eval "(voice_msu_ru_nsh_clunits)" > bootup2.wav
aplay bootup2.wav &

echo 'работать гoтов!' | text2wave -eval "(voice_msu_ru_nsh_clunits)" > bootup3.wav 
aplay bootup3.wav &


echo 'извините товарищ капитан но у нас РАМ кончился.' | text2wave -eval "(voice_msu_ru_nsh_clunits)" > ram1.wav
aplay ram1.wav &

echo 'капитан. мы начали использовать СВAAП.' | text2wave -eval "(voice_msu_ru_nsh_clunits)" > swap1.wav
aplay swap1.wav &

echo 'капитан. процессор перегружен!' | text2wave -eval "(voice_msu_ru_nsh_clunits)" > cpu1.wav
aplay cpu1.wav &



echo "DONE";

