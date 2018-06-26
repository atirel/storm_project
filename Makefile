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
	$(CC) -emit-llvm -c -o dumb.bc $^
	$(OPT) -load $(PASS_DIR)/$(PASSLIB) $(REQUIREDPASS) $(PASS) < dumb.bc  > ./dumb_path.bc
	$(LLVMDIS) -o dumb.ll dumb_path.bc
	$(LLC) dumb_path.bc
	$(CC) -o $@ dumb_path.s
	@sleep 2

test: exec_test

exec_test:
	cd test && ./test.sh

clean: 
	rm -rf *.o *.bc
	rm -rf *.ll *.s
	rm -rf *.dot
	rm -rf $(EXEC)

