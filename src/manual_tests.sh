#!/bin/bash

echo "z80 tests. errores de flags normalmente en: BIT n,(HL). errores de memptr en ninguno"
sleep 4
./zesarux --noconfigfile extras/media/spectrum/tests/z80tests.tap

echo "z80doc tests. van todos"
sleep 4
./zesarux --noconfigfile ./extras/media/spectrum/tests/z80doc.tap

echo "fusetest. en 48k, 128k. falla Floating bus. skipped read 3ffd, 7ffd. en +2a falla ademas LDIR, Floating bus skipped, bffd read failed. en 128k dado que lee puerto
paginacion, se pone pantalla en negro"
sleep 4
./zesarux --noconfigfile ./extras/media/spectrum/tests/fusetest.tap
./zesarux --noconfigfile --machine 128k ./extras/media/spectrum/tests/fusetest.tap

echo "Timing_Tests-48k_v1.0.sna. errores normalmente en test 4 add etc, test 25 add etc, test 36 in a etc, test 37 in a etc"
sleep 4
./zesarux --noconfigfile ./extras/media/spectrum/tests/Timing_Tests-48k_v1.0.sna

echo "Probar ClckFreq.p"
sleep 4
./zesarux --noconfigfile --machine zx81 extras/media/zx81/tests/ClckFreq.p

echo "Probar chroma81"
sleep 4
./zesarux --noconfigfile --machine zx81 --realvideo --chroma81 extras/media/zx81/Chroma_81/ALMINCOL.P
./zesarux --noconfigfile --machine zx81 --realvideo --chroma81 extras/media/zx81/Chroma_81/GalaxCol.P
./zesarux --noconfigfile --machine zx81 --realvideo --chroma81 extras/media/zx81/Chroma_81/MazogsColour.P
./zesarux --noconfigfile --machine zx81 --realvideo --chroma81 extras/media/zx81/Chroma_81/proftime.p
./zesarux --noconfigfile --machine zx81 --realvideo --chroma81 extras/media/zx81/Chroma_81/MISSICOL.P
./zesarux --noconfigfile --machine zx81 --realvideo --chroma81 extras/media/zx81/Chroma_81/stellar.p
./zesarux --noconfigfile --machine zx81 --realvideo --chroma81 extras/media/zx81/Chroma_81/wall-expanded.p
./zesarux --noconfigfile --machine zx81 --realvideo --chroma81 extras/media/zx81/Chroma_81/rebound/Rebound.p



echo "Probar spool file"
sleep 4
TEMPFILE=`mktemp`

#./txt_to_basic_lines.sh archivo tipo si_numlinea si_rem si_delayline
#-probar spool file: en spectrum 48k, 128k, zx80, zx81, z88, con modo turbo y sin. usar script txt_to_basic_lines.sh
#tipo puede ser: 1: 48k, zx81. 2: zx80. 3: 128k, z88, o cualquier otro sistema que use 'REM' con caracteres y no tokens

./txt_to_basic_lines.sh README 1 si si si > $TEMPFILE
./zesarux --noconfigfile --machine 48k --keyboardspoolfile $TEMPFILE 

./txt_to_basic_lines.sh README 1 si si si > $TEMPFILE
./zesarux --noconfigfile --machine zx81 --keyboardspoolfile $TEMPFILE 

./txt_to_basic_lines.sh README 2 si si si > $TEMPFILE
./zesarux --noconfigfile --machine zx80 --keyboardspoolfile $TEMPFILE 

./txt_to_basic_lines.sh README 3 si si si > $TEMPFILE
./zesarux --noconfigfile --machine 128k --keyboardspoolfile $TEMPFILE 

./txt_to_basic_lines.sh README 3 si si no > $TEMPFILE
./zesarux --noconfigfile --machine z88 --keyboardspoolfile $TEMPFILE 

./txt_to_basic_lines.sh README 3 si si no > $TEMPFILE
./zesarux --noconfigfile --machine cpc464 --keyboardspoolfile $TEMPFILE 
