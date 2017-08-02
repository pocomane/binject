#!/bin/sh

#############################################################
# Prepare directory

TEST_DIR="$(readlink -f "$(dirname "$0")")/tmp"

rm -fR "$TEST_DIR"
mkdir "$TEST_DIR"
cd "$TEST_DIR"

#############################################################
# Compile static

gcc -o ./array_static.exe ../../*.c
strip ./array_static.exe

gcc -D'BINJECT_ARRAY_SIZE=3' -o ./tail_static.exe ../../*.c
strip ./tail_static.exe

cp ./tail_static.exe ./tail_static_bis.exe

#############################################################
# Compile shared

gcc -shared -fPIC -std=c99 -o ./libbinject_array.so ../../binject.c || exit -1
gcc -o ./array_shared.exe ../../example.c -L ./ -lbinject_array || exit -1
strip ./libbinject_array.so
strip ./array_shared.exe

cp ./array_shared.exe ./array_shared_bis.exe

export LD_LIBRARY_PATH="./"

#############################################################
# Test working

test_working_sequence(){
  TEXT="hello world $1"

  # create text to embed
  echo "$TEXT" > ./"$1".txt || exit -1

  # embed the text
  ./"$1" ./"$1".txt || exit -1
  mv "$1".txt.exe ./"$1".emb || exit -1

  # run the app with the embedded text
  ./"$1".emb > ./"$1".rpt || exit -1

  # check output
  LEN="+$TEXT"
  LEN="${#LEN}"
  RES=$(cat ./"$1".rpt | tail -n 2 | head -n 1 | sed 's:^ A .. ::')
  if [ "$RES" != "A $LEN byte script was found (dump:)[$TEXT" ] ; then
    echo "SOMETHING WRONG"
    exit -1
  fi
}

test_working_sequence array_static.exe
test_working_sequence tail_static.exe
test_working_sequence tail_static_bis.exe
test_working_sequence array_shared.exe
test_working_sequence array_shared_bis.exe

#############################################################
# Test Array / Tail method trhough exe size

ls -l > my_size_info.txt
grep '.exe.emb$' my_size_info.txt

ARRAY=$(grep 'array_shared.exe.emb$' my_size_info.txt | awk '{print $5}')
ARRAYBIS=$(grep 'array_shared_bis.exe.emb$' my_size_info.txt | awk '{print $5}')

# If the array method was chosen, the size of the exe is always the same
if [ "$ARRAY" != "$ARRAYBIS" ] ; then
  echo "SOMETHING WRONG"
  exit -1
fi

TAIL=$(grep 'tail_static.exe.emb$' my_size_info.txt | awk '{print $5}')
TAILBIS=$(grep 'tail_static_bis.exe.emb$' my_size_info.txt | awk '{print $5}')

# If the tail method was chosen, the size of the exe depend on the embeded script
if [ "$TAIL" = "$TAILBIS" ] ; then
  echo "SOMETHING WRONG"
  exit -1
fi

#############################################################
# Test shared

rm *.so
./array_shared.exe.emb > array_shared.exe.empty.rpt 2> ./array_shared.exe.error.rpt

RES=$(cat array_shared.exe.empty.rpt)
if [ "$RES" != "" ] ; then
  echo "SOMETHING WRONG"
  exit -1
fi

#############################################################
# Print succesfull summary

echo "ALL RIGHT"

