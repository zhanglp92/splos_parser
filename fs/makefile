C_SOURCES = $(filter-out main.c, $(wildcard *.c))
C_OBJECTS = $(patsubst %.c,%.o,$(C_SOURCES))

CFLAGS  = -DDEBUG -c -Wall -g 
PROGRAM = filesystem

all:$(C_OBJECTS)
		@echo link...
		gcc  -c $(C_OBJECTS) -o $(PROGRAM)
		make libra

c.o:
		gcc -c $(CFLAGS) $< -o $@

.PHONY:clean
clean:
		rm -rf $(C_OBJECTS) $(PROGRAM) libfilesystem.a
		rm -rf /tmp/file.txt
libra:
		ar -r libfilesystem.a *.o
