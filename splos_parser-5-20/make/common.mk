CC  := gcc
RM  := rm -f
CP  := cp
AR  := ar rc

INCS := -I$(join $(SRC_DIR), /include)

define cc_lib
$(lib_name): $(addsuffix .o, $(basename $(srcs)))
ifeq (.a, $(suffix $(lib_name)))
	$(AR) $$@ $$^
else
	$(CC) -shared -o $$@ $$^
endif
endef

.c.o:
	$(CC) -fPIC -g -o $(<:.c=.o) -c $(INCS) $<
