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
	make -C ./types libtypes.a
	make -C ./tools libtools.a
	make -C ./lex   liblex.a
	make -C ./parse libparse.a
	$(CC) -o $@ $^ \
		./lex/liblex.a \
		./parse/libparse.a \
		./types/libtypes.a \
		./tools/libtools.a \
		 $(FS_LIB)

clean:
	$(RM) $(bin_name) $(addsuffix .o, $(basename $(bin_cc)))
	$(RM) -rf /tmp/file.txt

clean_all: clean
	make -C ./types clean
	make -C ./tools clean
	make -C ./lex   clean
	make -C ./parse clean

.PHONY: clean clean_all bin
