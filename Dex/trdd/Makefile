LIBREX_A_PATH = ../libs/libRex/Linux64
LIBREX_PATH = ../libs/libRex

default: compile

run: clean compile
	./trdd -s 1001 -p 0 -M 1000 100113_SO1	

librex.a:
	cd $(LIBREX_A_PATH) ; make -f Makefile.Release ; cd -

compile: librex.a
	clang *.c -I$(LIBREX_PATH) -L$(LIBREX_A_PATH)/release -lrex -lm -o trdd

clean:
	cd $(LIBREX_A_PATH) ; make -f Makefile.Release clean ; cd -
	rm -rf trdd
