#!/bin/sh

TEST_DIR="$(readlink -f "$(dirname "$0")")/tmp"

rm -fR "$TEST_DIR"
mkdir "$TEST_DIR"
cd "$TEST_DIR"

gcc -std=c99 -o binject.exe ../../*.c
./binject.exe my_script

echo "hello world" > my_text.txt
./binject.exe my_text.txt
rm my_text.txt

./my_text.txt.exe > my_test.rpt

RES=$(cat my_test.rpt | tail -n 2 | head -n 1)

if [ "$RES" = "A 12 byte script was found (dump:)[hello world" ] ; then
  echo "All tests passed"
else
  echo "TEST FAIL"
fi

