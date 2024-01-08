default: empty part1

empty.o: empty.c
        gcc -c empty.c -o empty.o

part1.o: part1.c
        gcc -c part1.c -o part1.o

empty: empty.o
        gcc empty.o -o empty

part1: part1.o
        gcc part1.o -o part1

clean:
        -rm -f empty.o
        -rm -f empty
        -rm -f part1.o
        -rm -f part1
