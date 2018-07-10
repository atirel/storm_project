#!/bin/bash
place=`pwd`/src-test-files
COLS=$(tput cols)
cd $place
function binGenerator {
   echo $1
   clang -emit-llvm -c -o $1.bc $1.c
   opt -load ~/storm/llvm-joujou/test/storm_project/build/DoubleStore/LLVMDoubleStore.so -DoubleStore < $1.bc > $2.bc 
   llvm-dis -o $1.ll $2.bc
   llc $2.bc
   cat $1.ll | grep -v "ModuleID" > $1.txt
}

function binChecker {
   if [[ ! -e ../../expected-results-tst/$1.txt ]]
   then
      echo 2
#      ultimateCleaner `pwd`
      return
   fi
  DIFF=$(diff $1.txt ../../expected-results-tst/$1.txt)
   if [[ $DIFF != "" ]]
   then
      echo 1
      return
   fi
   echo 0
}

SUC=`echo -e "\033[0;92mSUCCESS\033[0m"`
FAIL=`echo -e "\033[0;91mFAILURE\033[0m"`
NOTFOUND=`echo -e "\t\033[1;94m Test Not Found\033[0m"`

function fatalTestor {
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
}

function fatalDisplayer {
   let "j=0"
   let "i=1"
   for file in $(ls)
   do
      if [ ${file: -2} == ".c" ]
      then
   	 echo -n -e "\n$file"
   	 my_file=`echo "$file" | cut -d\. -f1` 
   	 sizename=${#file}
   	 a=`binChecker $my_file`
   	 if [[ "$a" == "0" ]]
   	 then
   	    printf "%*s" $[$COLS/2-sizename] "$SUC"
   	    let "j+=1"
	 elif [[ "$a" == "2" ]]
	 then
	    printf "%*s" $[$COLS/2-sizename] "$NOTFOUND"
	 else
   	    printf "%*s" $[$COLS/2-sizename] "$FAIL"
   	 fi
   	 let "i+=1"
      fi
   done
   let "i-=1"
   echo -e "\n\n\n"
   printf "%*s" $[$COLS/2-10] "Success rate: "
   successRate=$(( j * 10000 / i))
   if [[ "$successRate" -ge "7500" ]]
   then
      echo -en "\033[0;92m"
   elif [[ "$successRate" -ge "5000" ]]
   then
      echo -en "\033[0;93m"
   else
      echo -ne "\033[0;91m"
   fi
   echo -e "$(( successRate / 100)).$(( successRate % 100))%\n\n\n\033[0m"
   rm -f *.bc my_bc *.ll *.s
}

function ultimateCleaner {
   rm -f $1/*.ll $1/*.bc #$1/.txt
}

clear
echo "Do not worry if you see useless stuff for testing purposes, the execution is written over cerrs and as far as my knowledge goes, i don't know a way to hide it while saving it"
echo "for convention, everything will be erased before showing test results"
printf "\n\n\n\n\n%*s\n\n\n\n\n" $[$COLS/2] "press a key to continue"
read continuation
let total=0
cd basic_c_tests
fatalTestor
let "total+=i"
cd ../c_array_tests
fatalTestor
let "total+=i"
cd ../c_cond_tests
fatalTestor
let "total+=i"
clear
printf "\n\n\t\t%*s\n\n" $[$COLS/2] "Testing basic C functions without condition or loop or array"
cd ../basic_c_tests
fatalDisplayer
ultimateCleaner .
cd ../c_array_tests
printf "\n\n\t\t%*s\n\n" $[COLS/2] " Testing more complex C programs with cond and arrays"
fatalDisplayer
ultimateCleaner .
cd ../c_cond_tests
printf "\n\n\t\t%*s\n\n" $[COLS/2] " Testing simple and more complex conditions and loops"
fatalDisplayer
ultimateCleaner .
cd ..
