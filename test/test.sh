#!/bin/bash

cd src-test-files

function binGenerator {
   clang -emit-llvm -c -o $1.bc $1.c
   opt -load ~/storm/llvm-joujou/test/storm_project/build/DoubleStore/LLVMDoubleStore.so -DoubleStore < $1.bc > $2.bc 
   llvm-dis -o $1.ll $2.bc
   cat $1.ll | grep -v "ModuleID" > $1.txt
}

function binChecker {
  DIFF=$(diff $1.txt ../expected-results-tst/$1.txt)
   if [[ $DIFF != "" ]]
   then
      echo 1
      return
   fi
   echo 0
}


let "i=1"
for file in $(ls)
do
   if [ ${file: -2} == ".c" ]
   then
      my_file=`echo "$file" | cut -d\. -f1`
      binGenerator $my_file $i 
   fi
   let "i+=1"
done


for file in $(ls)
do
   if [ ${file: -2} == ".c" ]
   then
      echo -n -e "testing $file\t\t"
      my_file=`echo "$file" | cut -d\. -f1` 
      a=`binChecker $my_file`
      if [[ "$a" -ne "1" ]]
      then
	 echo -e "\033[0;32mSUCCESS\033[0m"
      else
	 echo -e "\033[0;31mFAILED\033[0m"
      fi
   fi
done
#rm -f *.bc *.txt my_bc *.ll
cd ..
