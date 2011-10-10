#!/bin/sh
diff="diff.txt"
OLD="old"
NEW="new"

cp tests/* .
cp disk-scheduler/* .
cp lib/* .

mkdir $OLD $NEW

test() {
  echo "Compiling $2 thread library ..."
  g++ thread$1.cc -c -o thread$1.a -m32
  echo "Testing $2 thread library ..."
  for i in 1 2 3 4
  do
    echo " - test "$i
    g++ thread$1.a libinterrupt.a test$i.cpp -o test$i$1.o -ldl -m32
    ./test$i$1.o > $2/test$i.out
  done
  echo "Compiling $2 disk scheduler ..."
  g++ thread$1.a libinterrupt.a diskold.cc -o disk$1.o -ldl -m32
  echo "Testing $2 disk scheduler..."
  for i in 1 2 3 4 5
  do
    echo " - test "$i
    ./disk$1.o $i disk.in0 disk.in1 disk.in2 disk.in3 disk.in4 > $2/disk$i.out
  done
}

test $OLD $OLD
test "" $NEW

touch $diff
diff new old > $diff

echo "Diff:"
more $diff

echo "Clean up"
rm test*.o test*.cpp threadold.cc thread.a threadold.a
rm disk.* diskold.*
rm interrupt.h libinterrupt.a
rm -r $OLD $NEW