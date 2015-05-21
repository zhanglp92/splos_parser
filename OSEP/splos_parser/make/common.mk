CC  := gcc
RM  := rm -f
CP  := cp
AR  := ar rc

INCS    := -I$(join $(SRC_DIR), /include) \
		   -I$(join $(SRC_DIR), /..)
FS_LIB  := $(join $(SRC_DIR), /../fs/libfilesystem.a)

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
