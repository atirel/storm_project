#!/bin/bash
place=`pwd`/src-test-files
cd $place
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
clear
echo "Do not worry if you see useless stuff for testing purposes, the execution is written over cerrs and as far as my knowledge goes, i don't know a way to hide it while saving it"
echo "for convention, everything will be erased before showing test results"
echo -e "\n\n\n\t\t\t\t\t\t\tpress a key to continue"
read continuation

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

clear
let "i=1"
let "j=0"
for file in $(ls)
do
   if [ ${file: -2} == ".c" ]
   then
      echo -n -e "$file\t\t\t\t\t\t"
      my_file=`echo "$file" | cut -d\. -f1` 
      a=`binChecker $my_file`
      if [[ "$i" == "1" || "$i" == "3" || "$i" == "5" || "$i" == "9" ]]
      then
	 echo -en "\t\t"
      fi
      if [[ "$i" == "2" || "$i" == "7" ]]
      then
	 echo -en "\t\t\t" 
      fi
      if [[ "$i" == "4" || "$i" == "6" || "$i" == "10" || "$i" == "11" || "$i" == "12" ]]
      then
	 echo -en "\t"
      fi
      if [[ "$a" -ne "1" ]]
      then
	 echo -e "\033[0;32mSUCCESS\033[0m"
	 let "j+=1"
      else
	 echo -e "\033[0;31m FAILED\033[0m"
      fi
      let "i+=1"
   fi
done
let "i-=1"
echo -ne "\n\n\n\t\t\t\t\t\t\t Success rate: "
succesRate=$(( j * 10000 / i))
if [[ "$succesRate" -ge "7500" ]]
then
   echo -en "\033[0;92m"
elif [[ "$succesRate" -ge "5000" ]]
then
   echo -en "\033[0;93m"
else
   echo -ne "\033[0;91m"
fi
echo -e "$(( succesRate / 100)).$(( succesRate % 100))%\n\n\n\033[0m"
rm -f *.bc my_bc *.ll *.txt
cd ..
