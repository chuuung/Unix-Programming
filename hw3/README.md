# Extend the Mini Lib C to Handle Signals

You have to implement the following C library functions in Assembly and C using the syntax supported by yasm x86_64 assembler.

  * setjmp: prepare for long jump by saving the current CPU state. In addition, preserve the signal mask of the current process.
  * longjmp: perform the long jump by restoring a saved CPU state. In addition, restore the preserved signal mask.
  * signal and sigaction: setup the handler of a signal.
  * sigprocmask: can be used to block/unblock signals, and get/set the current signal mask.
  * sigpending: check if there is any pending signal.
  * alarm: setup a timer for the current process.
  * write: write to a file descriptor.
  * pause: wait for signal
  * sleep: sleep for a specified number of seconds
  * exit: cause normal process termination
  * strlen: calculate the length of the string, excluding the terminating null byte ('\0').
  * functions to handle sigset_t data type: sigemptyset, sigfillset, sigaddset, sigdelset, and sigismember.

## Sample Output

### write1
```
$ make write1
gcc -c -g -Wall -fno-stack-protector -nostdlib -I. -I.. -DUSEMINI write1.c
ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o write1 write1.o start.o -L. -L.. -lmini
rm write1.o
$ LD_LIBRARY_PATH=. ./write1
Hello world!
```
### alarm1
The commands to assemble, compile, and link alarm1.c, as well as the corresponding runtime output are pasted below.
```
$ make alarm1
gcc -c -g -Wall -fno-stack-protector -nostdlib -I. -I.. -DUSEMINI alarm1.c
ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o alarm1 alarm1.o start.o -L. -L.. -lmini
rm alarm1.o
$ LD_LIBRARY_PATH=. ./alarm1
(3 seconds later ...)
Alarm clock
```

### alarm2
The commands to assemble, compile, and link alarm2.c, as well as the corresponding runtime output are pasted below.
```
$ make alarm2
gcc -c -g -Wall -fno-stack-protector -nostdlib -I. -I.. -DUSEMINI alarm2.c
ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o alarm2 alarm2.o start.o -L. -L.. -lmini
rm alarm2.o
$ LD_LIBRARY_PATH=. ./alarm2
(5 seconds later ...)
sigalrm is pending.
```
### alarm3
The commands to assemble, compile, and link alarm3.c, as well as the corresponding runtime output are pasted below.
```
$ make alarm3
gcc -c -g -Wall -fno-stack-protector -nostdlib -I. -I.. -DUSEMINI alarm3.c
ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.3 -o alarm3 alarm3.o start.o -L. -L.. -lmini
rm alarm3.o
$ LD_LIBRARY_PATH=. ./alarm3
^Csigalrm is pending.
```

### jmp1
The commands to assemble, compile, and link jmp1.c, as well as the corresponding runtime output are pasted below.
```
$ make jmp1
gcc -o jmp1.o -c -g -Wall -fno-stack-protector -nostdlib -I. -I.. -DUSEMINI jmp1.c
ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o jmp1 jmp1.o start.o -L. -L.. -lmini
rm jmp1.o
$ LD_LIBRARY_PATH=. ./jmp1
This is function a().
This is function b().
This is function c().
This is function d().
This is function e().
This is function f().
This is function g().
This is function h().
This is function i().
This is function j().
$
```
