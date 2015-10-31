#!/bin/sh
set -e

echo "#####################################"
echo "# LINGI 1341 Project Testing Script #"
echo "#       -- 31 October 2015 --       #"
echo "#####################################"

# Running sender-receiver
# for their checksum.
echo "[`date +"%M:%S"`] Generating a 100MB file"
dd if=/dev/urandom bs=400000 count=250 of=in.dat 2> /dev/null
CHECKSUM=`sha256sum in.dat`

echo "[`date +"%M:%S"`] Launching receiver"
timeout 302 ./receiver :: 64341 -f out.dat &
PIDRECV=$!

sleep 2
echo "[`date +"%M:%S"`] Launching sender and transfering"
timeout 300 ./sender ::1 64341 -f in.dat

if [ "$?" != 0 ]; then
    echo "[`date +"%M:%S"`] sender failed with exit code $?"
    echo "-- Test failed --"
    exit 1;
else
    wait $PIDRECV
    if [ "$?" != 0 ]; then
        echo "[`date +"%M:%S"`] receiver failed with exit code $?"
        echo "-- Test failed --"
        exit 1;
    else
        SHA1A=`echo $CHECKSUM | awk -F' ' '{print $1}'`
        SHA1B=`sha256sum out.dat | awk -F' ' '{print $1}'`

        if [  $SHA1A != $SHA1B ]; then
            echo "[`date +"%M:%S"`] The received file did not match the sent one"
            echo "-- Test failed --"
            exit 1;         
        fi
    fi
fi

echo "[`date +"%M:%S"`] OK for checksums verification !"
echo "[`date +"%M:%S"`] Testing with packets losses and corruption"

# Spaghetti duplicati codelini
rm -f in.dat out.dat
echo "[`date +"%M:%S"`] Generating a 1MB file"
dd if=/dev/urandom bs=40000 count=25 of=in.dat 2> /dev/null
CHECKSUM=`sha256sum in.dat`

(cd tests/linksim/ && make) > /dev/null
if [ "$?" != 0 ]; then
    echo "!! Failed to build link_sim"
    exit 1;
fi

timeout 302 ./tests/linksim/link_sim -p 1234 -P 4321 -d 500 -j 500 -e 5 -c 5 -l 5 > /dev/null &

echo "[`date +"%M:%S"`] Launching receiver"
timeout 302 ./receiver :: 4321 -f out.dat &
PIDRECV=$!

sleep 2
echo "[`date +"%M:%S"`] Launching sender and transfering"
timeout 300 ./sender ::1 1234 -f in.dat

if [ "$?" != 0 ]; then
    echo "[`date +"%M:%S"`] sender failed with exit code $?"
    echo "-- Test failed --"
    exit 1;
else
    wait $PIDRECV
    if [ "$?" != 0 ]; then
        echo "[`date +"%M:%S"`] receiver failed with exit code $?"
        echo "-- Test failed --"
        exit 1;
    else
        SHA1A=`echo $CHECKSUM | awk -F' ' '{print $1}'`
        SHA1B=`sha256sum out.dat | awk -F' ' '{print $1}'`

        if [  $SHA1A != $SHA1B ]; then
            echo "[`date +"%M:%S"`] The received file did not match the sent one"
            echo "-- Test failed --"
            exit 1;
        fi
    fi
fi

echo "[`date +"%M:%S"`] OK for checksums verification !"
echo "-- Test succeeded --"
rm -f in.dat out.dat
exit 0;
