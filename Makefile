all :
	gcc -c wav.c -o wav.o
	gcc -c audio.c -o audio.o
	gcc wav.c audio.o -o synth
clean :
	rm -rf *.o synth
