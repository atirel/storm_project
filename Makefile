#PASS_DIR=`pwd`/build/0Store
#PASSLIB=LLVM0Store.so
#PASS=-Store0

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

example: example.c
	$(CC) -g -emit-llvm -c -o example.bc $^
	$(OPT) -load $(PASS_DIR)/$(PASSLIB) $(REQUIREDPASS) $(PASS) < example.bc > ./example_path.bc
	$(LLVMDIS) -o example.ll example_path.bc
	$(LLC) example.bc
	$(CC) -o $@ example.s
	@sleep 2

dumb: dumb.c
	$(CC) -g -emit-llvm -c -o dumb.bc $^
	$(OPT) -load $(PASS_DIR)/$(PASSLIB) $(REQUIREDPASS) $(PASS) < dumb.bc  > ./dumb_path.bc
	$(LLVMDIS) -o dumb.ll dumb_path.bc
	$(LLC) dumb.bc
	$(CC) -o $@ dumb.s
	@sleep 2

arrays: arrays.c
	$(CC) -emit-llvm -c -o arrays.bc $^
	$(OPT) -load $(PASS_DIR)/$(PASSLIB) $(REQUIREDPASS) $(PASS) arrays.bc
	$(LLVMDIS) -o arrays.ll arrays.bc
	$(LLC) arrays.bc
	$(CC) -o $@ arrays.s

example-pass: example.c
	$(CC) -S -emit-llvm -c -o example.ll $^
	$(OPT) -S -loops example.ll 
	$(OPT) -S -simplifycfg -dot-cfg -o example-after-print.ll example.ll 

clean: 
	rm -rf *.o *.bc
	rm -rf *.ll *.s
	rm -rf *.dot
	rm -rf $(EXEC)

