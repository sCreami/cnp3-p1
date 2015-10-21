#!/bin/sh
set -e
export LD_LIBRARY_PATH=$HOME/local/lib:$LD_LIBRARY_PATH

echo "#####################################"
echo "# LINGI 1341 Project Testing Script #"
echo "#       -- 21 October 2015 --       #"
echo "#####################################"

# Check if OpenSSL is installed 
openssl sha1 /dev/stdout > /dev/null
if [ "$?" != 0 ]; then
	echo ">> OpenSSL not available"
	echo "-- Test failed --"
	exit 1
fi

# Thus running sender-receiver
# for their checksum.
echo ">> Building CUnit tests"
make cunit > /dev/null
echo ">> Running CUnit for packets"
./test
echo ">> Testing for sender-receiver"
echo ">> Generating a 100MB file"
dd if=/dev/urandom bs=400000 count=250 of=in.dat 2> /dev/null
CHECKSUM=`openssl sha1 in.dat`

echo ">> Launching receiver"
./receiver :: 64341 -f out.dat &
PIDRECV=$!

sleep 1
echo ">> Launching sender and transfering"
./sender ::1 64341 -f in.dat

if [ "$?" != 0 ]; then
	echo ">> sender failed with exit code $?"
	echo "-- Test failed --"
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
rm -f int.dat out.dat
echo ">> Generating a 1MB file"
dd if=/dev/urandom bs=40000 count=25 of=in.dat 2> /dev/null
CHECKSUM=`openssl sha1 in.dat`

cd tests/linksim/ && make > /dev/null
if [ "$?" != 0 ]; then
	echo "!! Failed to build link_sim"
	exit 1;
fi

./link_sim -p 1234 -P 4321 -d 500 -j 500 -e 5 -c 5 -l 5 > /dev/null &
cd ../.. # I fckg know thnk u

echo ">> Launching receiver"
./receiver :: 4321 -f out.dat &
PIDRECV=$!

sleep 1
echo ">> Launching sender and transfering"
./sender ::1 1234 -f in.dat

if [ "$?" != 0 ]; then
	echo ">> sender failed with exit code $?"
	echo "-- Test failed --"
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

echo ">> SUCCEEDED ! Well done, you passed the test."
echo ">> Cleaning behind"
rm -f in.dat && rm -f out.dat
exit 0
