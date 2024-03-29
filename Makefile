
BLUE = `printf "\033[1;34m"`
RED =  `printf "\033[1;31m"`
NONE = `printf "\033[0m"`

SUBDIR := src

.PHONY: $(SUBDIR) libxputty  recurse

$(MAKECMDGOALS) recurse: $(SUBDIR)

check-and-reinit-submodules :
	@if git submodule status 2>/dev/null | egrep -q '^[-]|^[+]' ; then \
		echo $(RED)"INFO: Need to reinitialize git submodules"$(NONE); \
		git submodule update --init; \
		echo $(BLUE)"Done"$(NONE); \
	else echo $(BLUE)"Submodule up to date"$(NONE); \
	fi

clean:
	@rm -f ./libxputty/xputty/resources/XTuner.png

libxputty: check-and-reinit-submodules
ifeq (,$(wildcard ./libxputty/xputty/resources/XTuner.png))
	cp ./src/XTuner.png ./libxputty/xputty/resources/
endif
	@exec $(MAKE) -j 1 -C $@ $(MAKECMDGOALS)

$(SUBDIR): libxputty
	@exec $(MAKE) -j 1 -C $@ $(MAKECMDGOALS)

