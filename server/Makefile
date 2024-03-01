CC=clang++
CFLAGS=-I. -ggdb
DEPS=
PROG=scaffold_server
PORT=49931

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(PROG): main.o main.cpp
	$(CC) main.o -o $(PROG)

run: $(PROG)
	./$(PROG) $(PORT)

client:
	nc 127.0.0.1 $(PORT)

clean:
	rm $(PROG) *.o; true
