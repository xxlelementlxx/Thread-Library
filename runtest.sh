#!/bin/sh
cp tests/* .
cp disk-scheduler/* .
mkdir new old

g++ threadold.cc libinterrupt.a test1.cpp -o test1old.o -ldl -m32
g++ threadold.cc libinterrupt.a test2.cpp -o test2old.o -ldl -m32
g++ threadold.cc libinterrupt.a test3.cpp -o test3old.o -ldl -m32
g++ threadold.cc libinterrupt.a test4.cpp -o test4old.o -ldl -m32
g++ threadold.cc libinterrupt.a diskold.cc -o diskold.o -ldl -m32

./test1old.o > old/test1.out
./test2old.o > old/test2.out
./test3old.o > old/test3.out
./test4old.o > old/test4.out

./diskold.o 1 disk.in0 disk.in1 disk.in2 disk.in3 disk.in4 > old/disk1.out
./diskold.o 2 disk.in0 disk.in1 disk.in2 disk.in3 disk.in4 > old/disk2.out
./diskold.o 3 disk.in0 disk.in1 disk.in2 disk.in3 disk.in4 > old/disk3.out
./diskold.o 4 disk.in0 disk.in1 disk.in2 disk.in3 disk.in4 > old/disk4.out
./diskold.o 5 disk.in0 disk.in1 disk.in2 disk.in3 disk.in4 > old/disk5.out

g++ thread.cc libinterrupt.a test1.cpp -o test1.o -ldl -m32
g++ thread.cc libinterrupt.a test2.cpp -o test2.o -ldl -m32
g++ thread.cc libinterrupt.a test3.cpp -o test3.o -ldl -m32
g++ thread.cc libinterrupt.a test4.cpp -o test4.o -ldl -m32
g++ thread.cc libinterrupt.a disk.cc -o disk.o -ldl -m32

./test1.o > new/test1.out
./test2.o > new/test2.out
./test3.o > new/test3.out
./test4.o > new/test4.out
./disk.o 1 disk.in0 disk.in1 disk.in2 disk.in3 disk.in4 > new/disk1.out
./disk.o 2 disk.in0 disk.in1 disk.in2 disk.in3 disk.in4 > new/disk2.out
./disk.o 3 disk.in0 disk.in1 disk.in2 disk.in3 disk.in4 > new/disk3.out
./disk.o 4 disk.in0 disk.in1 disk.in2 disk.in3 disk.in4 > new/disk4.out
./disk.o 5 disk.in0 disk.in1 disk.in2 disk.in3 disk.in4 > new/disk5.out

touch diff.txt
diff new old > diff.txt
rm test*.o test*.cpp threadold.cc
rm disk.* diskold.*
rm -r new old