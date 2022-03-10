my-shell: my-shell.o
        gcc -std=c99 -o my-shell my-shell.o

my-shell.o:     my-shell.c
        gcc -std=c99 -c my-shell.c

clean:
        rm -rf my-shell *.o