CC = cc
CFLAGS = -I -Wall
EXEC = secure
#DEPS = 
OBJ = secure.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~

