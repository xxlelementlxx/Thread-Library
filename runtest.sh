#!/bin/sh

cp tests/* .

g++ threadold.cc libinterrupt.a test1.cpp -o test1old.o -ldl -m32
g++ threadold.cc libinterrupt.a test2.cpp -o test2old.o -ldl -m32
g++ threadold.cc libinterrupt.a test3.cpp -o test3old.o -ldl -m32
g++ threadold.cc libinterrupt.a test4.cpp -o test4old.o -ldl -m32

mkdir old
./test1old.o > old/test1.out
./test2old.o > old/test2.out
./test3old.o > old/test3.out
./test4old.o > old/test4.out

g++ thread.cc libinterrupt.a test1.cpp -o test1.o -ldl -m32
g++ thread.cc libinterrupt.a test2.cpp -o test2.o -ldl -m32
g++ thread.cc libinterrupt.a test3.cpp -o test3.o -ldl -m32
g++ thread.cc libinterrupt.a test4.cpp -o test4.o -ldl -m32

mkdir new
./test1.o > new/test1.out
./test2.o > new/test2.out
./test3.o > new/test3.out
./test4.o > new/test4.out

touch diff.txt
diff new old > diff.txt
rm test*.o test*.cpp threadold.cc
rm -r new old