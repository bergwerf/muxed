compile:
	cc -g -o muxed src/main.c -lncurses

debug:
	gdb ./muxed
