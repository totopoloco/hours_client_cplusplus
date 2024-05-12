CC=g++
CFLAGS=-I.
DEPS = main.h
OBJ = main.o 
LIBS=-lcurl

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

hours: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o hours
