#!/bin/sh

#############################################################
# Prepare directory

TEST_DIR="$(readlink -f "$(dirname "$0")")/tmp"

rm -fR "$TEST_DIR"
mkdir "$TEST_DIR"
cd "$TEST_DIR"

#############################################################
# Test script to be embedded

echo "hello world" > my_text_a.txt
echo "hello world bis" > my_text_b.txt

#############################################################
# Compile static

gcc -o ./binject.static.exe ../../*.c
strip ./binject.static.exe

#############################################################
# Embed the script

./binject.static.exe my_text_a.txt
mv my_text_a.txt.exe my_text_a.txt.static.exe

#############################################################
# Test working

./my_text_a.txt.static.exe > my_test.rpt

RES=$(cat my_test.rpt | tail -n 2 | head -n 1)

if [ "$RES" != "A 12 byte script was found (dump:)[hello world" ] ; then
  echo "SOMETHING WRONG"
  exit -1
fi

#############################################################
# Compile shared

gcc -shared -fPIC -std=c99 -o ./libbinject.so ../../binject.c || exit -1
gcc -o ./binject.shared.exe ../../example.c -L ./ -lbinject || exit -1
strip ./libbinject.so
strip ./binject.shared.exe
export LD_LIBRARY_PATH="./"

#############################################################
# Embed the scripts

./binject.shared.exe my_text_a.txt
mv my_text_a.txt.exe my_text_a.txt.shared.exe

./binject.shared.exe my_text_b.txt
mv my_text_b.txt.exe my_text_b.txt.shared.exe

#############################################################
# Size info

ls -l *.exe *.so > my_size_info.txt
cat my_size_info.txt

#############################################################
# Test working

./my_text_a.txt.shared.exe > my_test_a.rpt
./my_text_b.txt.shared.exe > my_test_b.rpt
rm ./libbinject.so
./my_text_b.txt.shared.exe > my_test_c.rpt 2> expected_error.rpt

RES=$(cat my_test_a.rpt | tail -n 2 | head -n 1)

if [ "$RES" != "A 12 byte script was found (dump:)[hello world" ] ; then
  echo "SOMETHING WRONG"
  exit -1
fi

RES=$(cat my_test_b.rpt | tail -n 2 | head -n 1)

if [ "$RES" != "A 16 byte script was found (dump:)[hello world bis" ] ; then
  echo "SOMETHING WRONG"
  exit -1
fi

if [ "$(cat my_test_c.rpt)" != "" ] ; then
  echo "SOMETHING WRONG"
  exit -1
fi

#############################################################
# Print succesfull summary

echo "ALL RIGHT"

