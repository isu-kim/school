CC = gcc

prod_cons_v1: prod_cons_v1.c
	$(CC) -o $@ $<
	echo "Now use ./prod_cons_v1"
prod_cons_v2: prod_cons_v2.c
	$(CC) -o $@ $<
	echo "Now use ./prod_cons_v2"
clean:
	rm prod_cons_v1 && rm prod_cons_v2

all: prod_cons_v1 prod_cons_v2
