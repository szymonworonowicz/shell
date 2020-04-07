obj = main.o

all: comp
comp: $(obj)
		gcc $(obj) -o main
.PHONY: clean
clean:
		rm *.o main