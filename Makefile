#PASS_DIR=`pwd`/build/0Store
#PASSLIB=LLVM0Store.so
#PASS=-Store0

PASS_DIR=`pwd`/build/DoubleStore
PASSLIB=LLVMDoubleStore.so
PASS=-DoubleStore


REQUIREDPASS=
TST=`pwd`/test

CC=clang
CXX=clang++
OPT=opt
LLVMDIS=llvm-dis
LLC=llc

CFLAGS=

EXEC=cheeky

all: $(EXEC)

example: test/example.c
	$(CC) -g -emit-llvm -c -o example.bc $^
	$(OPT) -load $(PASS_DIR)/$(PASSLIB) $(REQUIREDPASS) $(PASS) < example.bc > ./example_path.bc
	$(LLVMDIS) -o example.ll example_path.bc
	$(LLC) example.bc
	$(CC) -o $@ example.s
	@sleep 2

dumb:	test/dumb.c
	$(CC) -g -emit-llvm -c -o dumb.bc $^
	$(OPT) -load $(PASS_DIR)/$(PASSLIB) $(REQUIREDPASS) $(PASS) < dumb.bc  > ./dumb_path.bc
	$(LLVMDIS) -o dumb.ll dumb_path.bc
	$(LLC) dumb.bc
	$(CC) -o $@ dumb.s
	@sleep 2

cheeky: test/cheeky.c
	$(CC) -g -emit-llvm -c -o cheeky.bc $^
	$(OPT) -load $(PASS_DIR)/$(PASSLIB) $(REQUIREDPASS) $(PASS) < cheeky.bc  > ./cheeky_path.bc
	$(LLVMDIS) -o cheeky.ll cheeky_path.bc
	$(LLC) cheeky.bc
	$(CC) -o $@ cheeky.s

example-pass: example.c
	$(CC) -S -emit-llvm -c -o example.ll $^
	$(OPT) -S -loops example.ll 
	$(OPT) -S -simplifycfg -dot-cfg -o example-after-print.ll example.ll 

clean: 
	rm -rf *.o *.bc
	rm -rf *.ll *.s
	rm -rf *.dot
	rm -rf $(EXEC)

