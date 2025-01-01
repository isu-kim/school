CC = gcc
OBJS = main.o sish.o utils.o
CFLAGS = -c -o $@ $<

sish: $(OBJS)
	$(CC) -o sish $(OBJS)
	echo "Now use ./sish"
main.o: main.c utils.h sish.h
	$(CC) $(CFLAGS)
utils.o: utils.c utils.h
	$(CC) $(CFLAGS)
sish.o: sish.c sish.h
	$(CC) $(CFLAGS)
clean:
	rm *.o && rm sish
