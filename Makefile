.PHONY: clean

bgrep: bgrep.o
	gcc bgrep.o -o bgrep

bgrep.o: src/bgrep.c
	gcc -c src/bgrep.c

clean:
	rm -f *.o bgrep
