CC = gcc
ASM64 = yasm -f elf64 -DYASM -D__x86_64__ -DPIC
CFLAG = -g -Wall -fno-stack-protector -nostdlib

BUILD_SO = libmini.so

all: libmini64.o libmini.o start.o
	ld -share -o $(BUILD_SO) libmini64.o libmini.o
%.o: %.asm
	$(ASM64) $< -o $@
%.o: %.c
	$(CC) -c -fPIC $(CFLAG) $< -o $@

write1:
	$(CC) -c $(CFLAG) -I. -I.. -DUSEMINI write1.c
	ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o write1 write1.o start.o -L. -L.. -lmini
	rm write1.o

alarm1:
	$(CC) -c $(CFLAG) -I. -I.. -DUSEMINI alarm1.c
	ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o alarm1 alarm1.o start.o -L. -L.. -lmini
	rm alarm1.o

alarm2:
	$(CC) -c $(CFLAG) -I. -I.. -DUSEMINI alarm2.c
	ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o alarm2 alarm2.o start.o -L. -L.. -lmini
	rm alarm2.o

alarm3:
	$(CC) -c $(CFLAG) -I. -I.. -DUSEMINI alarm3.c
	ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o alarm3 alarm3.o start.o -L. -L.. -lmini
	rm alarm3.o

jmp1:
	$(CC) -c $(CFLAG) -I. -I.. -DUSEMINI jmp1.c
	ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o jmp1 jmp1.o start.o -L. -L.. -lmini
	rm jmp1.o
	
test:
	make -C testcase

clean:
	rm -rf *.o
	rm -rf $(BUILD_SO)