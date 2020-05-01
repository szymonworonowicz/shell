obj = main.o

all: comp
comp: $(obj)
		gcc $(obj) -o my_shell
.PHONY: clean
clean:
		rm *.o my_shell