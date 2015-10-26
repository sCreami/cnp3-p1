#!/bin/sh
set -e

echo "#####################################"
echo "# LINGI 1341 Project Testing Script #"
echo "#       -- 22 October 2015 --       #"
echo "#####################################"

# Check if OpenSSL is installed 
openssl sha1 /dev/stdout > /dev/null
if [ "$?" != 0 ]; then
    echo ">> OpenSSL not available"
    echo "-- Test failed --"
    exit 1;
fi

# Thus running sender-receiver
# for their checksum.
echo ">> Generating a 100MB file"
dd if=/dev/urandom bs=400000 count=250 of=in.dat 2> /dev/null
CHECKSUM=`openssl sha1 in.dat`

echo ">> Launching receiver"
timeout 20 ./receiver :: 64341 -f out.dat &
PIDRECV=$!

sleep 2
echo ">> Launching sender and transfering"
timeout 20 ./sender ::1 64341 -f in.dat

if [ "$?" != 0 ]; then
    echo ">> sender failed with exit code $?"
    echo "-- Test failed --"
    exit 1;
else
    wait $PIDRECV
    if [ "$?" != 0 ]; then
        echo ">> receiver failed with exit code $?"
        echo "-- Test failed --"
        exit 1;
    else
        SHA1A=`echo $CHECKSUM | awk -F' ' '{print $2}'`
        SHA1B=`openssl sha1 out.dat | awk -F' ' '{print $2}'`

        if [  $SHA1A != $SHA1B ]; then
            echo ">> The received file did not match the sent one"
            echo "-- Test failed --"
            exit 1;         
        fi
    fi
fi

echo ">> OK for checksums verification !"
echo ">> Testing with packets losses and corruption."

# Spaghetti duplicati codelini
rm -f in.dat out.dat
echo ">> Generating a 1MB file"
dd if=/dev/urandom bs=40000 count=25 of=in.dat 2> /dev/null
CHECKSUM=`openssl sha1 in.dat`

(cd tests/linksim/ && make) > /dev/null
if [ "$?" != 0 ]; then
    echo "!! Failed to build link_sim"
    exit 1;
fi

./tests/linksim/link_sim -p 1234 -P 4321 -d 500 -j 500 -e 5 -c 5 -l 5 > /dev/null &

echo ">> Launching receiver"
timeout 60 ./receiver :: 4321 -f out.dat &
PIDRECV=$!

sleep 2
echo ">> Launching sender and transfering"
timeout 60 ./sender ::1 1234 -f in.dat

if [ "$?" != 0 ]; then
    echo ">> sender failed with exit code $?"
    echo "-- Test failed --"
    exit 1;
else
    wait $PIDRECV
    if [ "$?" != 0 ]; then
        echo ">> receiver failed with exit code $?"
        echo "-- Test failed --"
        exit 1;
    else
        SHA1A=`echo $CHECKSUM | awk -F' ' '{print $2}'`
        SHA1B=`openssl sha1 out.dat | awk -F' ' '{print $2}'`

        if [  $SHA1A != $SHA1B ]; then
            echo ">> The received file did not match the sent one"
            echo "-- Test failed --"
            exit 1;
        fi
    fi
fi

echo ">> OK for checksums verification !"
echo "-- Test succeeded --"
rm -f in.dat out.dat
exit 0;
