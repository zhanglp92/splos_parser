CC  := gcc
RM  := rm -f
CP  := cp
AR  := ar rc

INCS    := -I$(join $(SRC_DIR), /include) \
		   -I$(join $(SRC_DIR), /../fs/include)
FS_LIB  := $(join $(SRC_DIR), /../fs/lib/libfilesystem.a)

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
