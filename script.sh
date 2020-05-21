#!/bin/sh

a=0
echo "Server address: $1"
echo "Server port: $2"
echo "Number of clients: $3"
echo "Number of msgs per client: $4"
while [ $a -lt $3 ]
do
./client_eval $1 $2 $4 & 
#sleep 0.001
#./a.out $a &
#echo "***************Iteration `expr $a + 1`*******************"
a=`expr $a + 1`
done
