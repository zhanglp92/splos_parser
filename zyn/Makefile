# Makefile 0.1


C_SRC 	= ego.c scan.c error.c input.c str.c output.c \
		  simp.c



OBJS 	= $(C_SRC:.c=.o)
CC 		= gcc 
CFLAGS 	= -g -D_UCC 

all: $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) 

clean: 
	rm -f *.o a.out
