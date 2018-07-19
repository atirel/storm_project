PASS_DIR=`pwd`/build/DoubleStore
PASSLIB=LLVMDoubleStore.so
PASS=-DoubleStore


REQUIREDPASS=

CC=clang
CXX=clang++
OPT=opt
LLVMDIS=llvm-dis
LLC=llc

CFLAGS=

EXEC=dumb

all: $(EXEC)

dumb:	test/dumb.c
	$(CC) -emit-llvm -g -c -o dumb.bc $^
#	$(OPT) -dse < dumb.bc > dumb_temp.bc
	$(OPT) -load $(PASS_DIR)/$(PASSLIB) $(REQUIREDPASS) $(PASS) < dumb.bc  > ./dumb_path.bc
	$(LLVMDIS) -o dumb.ll dumb_path.bc
	$(LLC) dumb_path.bc
	$(CC) -o $@ dumb_path.s

test: exec_test

init:	test/dumb.c
	$(CC) -emit-llvm -c -o dumb_temp.bc $^
	$(OPT) -load build/Initialize/LLVMInitialize.so -Initialize < dumb_temp.bc > dumb.bc
	$(LLVMDIS) -o dumb.ll dumb.bc

exec_test:
	cd test && ./test.sh

clean: 
	rm -rf *.o *.bc
	rm -rf *.ll *.s
	rm -rf *.dot
	rm -rf $(EXEC)

