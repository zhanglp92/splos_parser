conf_filename := /tmp/conf.mk

ifneq ($(conf_filename), $(wildcard $(conf_filename)))
$(shell \
	echo 'SRC_DIR := $(shell pwd)' > $(conf_filename); \
	echo 'include $$(SRC_DIR)/make/common.mk' >> $(conf_filename))
endif

include $(conf_filename)

bin_cc   := pcl.c
bin_name := pcl

bin: 
	make $(bin_name)

$(bin_name): $(addsuffix .o, $(basename $(bin_cc))) 
	make -C ../fs 
	make -C ./types libtypes.a
	make -C ./tools libtools.so
	make -C ./lex   liblex.so
	make -C ./parse libparse.so
	$(CC) -o $@ $^ \
		 $(FS_LIB) \
		./lex/liblex.so \
		./parse/libparse.so \
		./types/libtypes.a \
		./tools/libtools.so

clean:
	$(RM) $(bin_name) $(addsuffix .o, $(basename $(bin_cc)))
	$(RM) -rf /tmp/file.txt

clean_all: clean
	make -C ./types clean
	make -C ./tools clean
	make -C ./lex   clean
	make -C ./parse clean
	make -C ../fs clean

.PHONY: clean clean_all bin
