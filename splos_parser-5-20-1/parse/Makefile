include /tmp/conf.mk

all: GET_LIB

lib_name := libparse.so
srcs     := $(wildcard *.c)

$(eval $(call cc_lib))

GET_LIB: $(lib_name)

clean:
	$(RM) $(lib_name) $(addsuffix .o, $(basename $(srcs)))

rebuild : clean all

.PHONY: all clean rebuild GET_LIB
