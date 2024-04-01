CC = cc
CFLAGS = -I -Wall
EXEC = saludcli
DEPS = saludcli.h
OBJ = saludcli.o log.o auxiliar.o socketaomi.o socketaraucaria.o secure.o dbsql.o main.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXEC): $(OBJ)
	$(CC) -ldxxx -lsrl -lmysqlclient -L/usr/local/mysql/lib/mysql -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~

