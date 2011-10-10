#!/bin/sh
diff="diff.txt"
cp tests/* .
cp disk-scheduler/* .
cp lib/* .

OLD="old"
NEW="new"
mkdir $OLD $NEW

test() {
  g++ thread$1.cc -c -o thread$1.a -m32
  for i in 1 2 3 4
  do
    g++ thread$1.a libinterrupt.a test$i.cpp -o test$i$1.o -ldl -m32
    ./test$i$1.o > $2/test$i.out
  done
  g++ thread$1.a libinterrupt.a diskold.cc -o disk$1.o -ldl -m32
  for i in 1 2 3 4 5
  do
    ./disk$1.o $i disk.in0 disk.in1 disk.in2 disk.in3 disk.in4 > $2/disk$i.out
  done
}

test $OLD $OLD
test "" $NEW

touch $diff
diff new old > $diff
rm test*.o test*.cpp threadold.cc thread.a threadold.a
rm disk.* diskold.*
rm interrupt.h libinterrupt.a
rm -r $OLD $NEW
more $diff