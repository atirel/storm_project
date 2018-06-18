#PASS_DIR=`pwd`/../build/LoadStore
#PASSLIB=LLVMLoadStore.so
#PASS=-LoadStore


#PASS_DIR=`pwd`/../build/HelloWorld
#PASSLIB=LLVMHelloWorld.so
#PASS=-HelloWorld


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

EXEC=example dumb

all: $(EXEC)

example: example.c
	$(CC) -g -emit-llvm -c -o example.bc $^
	$(OPT) -load $(PASS_DIR)/$(PASSLIB) $(REQUIREDPASS) $(PASS) example.bc 
	$(LLVMDIS) -o example.ll example.bc
	$(LLC) example.bc
	$(CC) -o $@ example.s
	@sleep 5

dumb: dumb.c
	$(CC) -g -emit-llvm -c -o dumb.bc $^
	$(OPT) -load $(PASS_DIR)/$(PASSLIB) $(REQUIREDPASS) $(PASS) dumb.bc 
	$(LLVMDIS) -o dumb.ll dumb.bc
	$(LLC) dumb.bc
	$(CC) -o $@ dumb.s

example-pass: example.c
	$(CC) -S -emit-llvm -c -o example.ll $^
	$(OPT) -S -loops example.ll 
	$(OPT) -S -simplifycfg -dot-cfg -o example-after-print.ll example.ll 

clean: 
	rm -rf *.o *.bc
	rm -rf *.ll *.s
	rm -rf *.dot
	rm -rf $(EXEC)

