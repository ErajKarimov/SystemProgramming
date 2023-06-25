CFLAGS=-Wall -Wextra -Werror -O3
TARGETS=lab1test lab1kebN32501 libkebN32501.so

.PHONY: all clean

all: $(TARGETS)

clean:
	rm -rf *.o $(TARGETS)

lab1test: lab1test.c plugin_api.h
	gcc $(CFLAGS) -o lab1test lab1test.c -ldl

lab1kebN32501: lab1kebN32501.c plugin_api.h
	gcc $(CFLAGS) -o lab1kebN32501 lab1kebN32501.c -ldl

libkebN32501.so: libkebN32501.c plugin_api.h
	gcc $(CFLAGS) -shared -fPIC -o libkebN32501.so libkebN32501.c -ldl -lm


