init_fs =  init_memory.c
C_SOURCES = $(filter-out $(init_fs), $(wildcard *.c))
C_OBJECTS = $(patsubst %.c,%.o,$(C_SOURCES))

CFLAGS  = -DDEBUG -c -Wall -g 
PROGRAM = filesystem
LIB = ../lib/libfilesystem.a
BIN = ../bin/init_fs

$(shell mkdir -p ../bin ../lib)

		#gcc -shared -o ../lib/libfilesystem.so $^
all : $(C_OBJECTS)
	ar rcv $(LIB) $^

bin: $(basename $(init_fs)).o
	gcc -o $(BIN) $^  $(LIB)
test: mv2local.o
	gcc -o mv2local $^ $(LIB)

.c.o:
	gcc -fPIC -o $(<:.c=.o) -c $<
	echo "hello"
	@echo $<

show:
	@echo $(wildcard *.c)

.PHONY:clean
clean:
		rm -rf $(C_OBJECTS) $(PROGRAM) $(LIB) $(BIN)
		rm -rf /tmp/file.txt
