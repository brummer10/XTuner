
	# check CPU and supported optimization flags
	ifeq ($(UNAME), Linux)
		ifneq ($(shell cat /proc/cpuinfo | grep sse3 ) , )
			SSE_CFLAGS = -msse3 -mfpmath=sse
		else ifneq ($(shell cat /proc/cpuinfo | grep sse2 ) , )
			SSE_CFLAGS = -msse2 -mfpmath=sse
		else ifneq ($(shell cat /proc/cpuinfo | grep sse ) , )
			SSE_CFLAGS = -msse -mfpmath=sse
			else ifneq ($(shell cat /proc/cpuinfo | grep ARM ) , )
			ifneq ($(shell cat /proc/cpuinfo | grep ARMv7 ) , )
				ifneq ($(shell cat /proc/cpuinfo | grep vfpd32 ) , )
					SSE_CFLAGS = -march=armv7-a -mfpu=vfpv3 
				else ifneq ($(shell cat /proc/cpuinfo | grep vfpv3 ) , )
					SSE_CFLAGS = -march=armv7-a -mfpu=vfpv3
				endif
			else
				ARMCPU = "YES"
			endif
		else
			SSE_CFLAGS =
		endif
	endif

	# set bundle name
	NAME = XTuner
	EXEC_NAME  = $(shell echo $(NAME) | tr A-Z a-z)
	BUILD_DIR = build
	VER = 1.0

	LIB_DIR := ../libxputty/libxputty/
	HEADER_DIR := $(LIB_DIR)include/
	PREFIX ?= /usr
	BIN_DIR ?= $(PREFIX)/bin
	SHARE_DIR ?= $(PREFIX)/share
	DESKAPPS_DIR ?= $(SHARE_DIR)/applications
	PIXMAPS_DIR ?= $(SHARE_DIR)/pixmaps
	MAN_DIR ?= $(SHARE_DIR)/man/man1

	# set compile flags
	DEFAULT_CXXFLAGS = -O2 -D_FORTIFY_SOURCE=2 -Wall -fstack-protector -funroll-loops -ffast-math -fomit-frame-pointer \
	-fstrength-reduce $(SSE_CFLAGS)
	DEBUG_CXXFLAGS += -g -D DEBUG
	LDFLAGS += -Wl,-z,noexecstack -I./ -I../libxputty/libxputty/include/ \
	`pkg-config --cflags --libs jack cairo x11 sigc++-2.0 fftw3f ` \
	-lm -lzita-resampler -lpthread -llo -DVERSION=\"$(VER)\"
	# invoke build files
	OBJECTS = NsmHandler.cpp  xtuner.cpp
	## output style (bash colours)
	BLUE = `printf "\033[1;34m"`
	RED =  `printf "\033[1;31m"`
	NONE = `printf "\033[0m"`

.PHONY : $(HEADER_DIR)*.h all debug clean install uninstall 

all : check $(NAME)
	@mkdir -p ./$(BUILD_DIR)
	@mv ./$(EXEC_NAME) ./$(BUILD_DIR)
	@if [ -f ./$(BUILD_DIR)/$(EXEC_NAME) ]; then echo $(BLUE)"build finish, now run make install"; \
	else echo $(RED)"sorry, build failed"; fi
	@echo $(NONE)

debug: DEFAULT_CXXFLAGS = $(DEBUG_CXXFLAGS) 
debug: all

check :
ifdef ARMCPU
	@echo $(RED)ARM CPU DEDECTED, please check the optimization flags
	@echo $(NONE)
endif

clean :
	@rm -f ./$(BUILD_DIR)/$(EXEC_NAME)
	@rm -rf ./$(BUILD_DIR)
	@echo ". ." $(BLUE)", clean up"$(NONE)

install :
ifneq ("$(wildcard ./$(BUILD_DIR))","")
	mkdir -p $(DESTDIR)$(BIN_DIR)
	cp ./$(BUILD_DIR)/$(EXEC_NAME) $(DESTDIR)$(BIN_DIR)/$(EXEC_NAME)
	mkdir -p $(DESTDIR)$(DESKAPPS_DIR)
	cp $(NAME).desktop $(DESTDIR)$(DESKAPPS_DIR)
	mkdir -p $(DESTDIR)$(PIXMAPS_DIR)
	cp $(NAME).png $(DESTDIR)$(PIXMAPS_DIR)
	@echo ". ." $(BLUE)", done"$(NONE)
else
	@echo ". ." $(BLUE)", you must build first"$(NONE)
endif

uninstall :
	@rm -rf $(DESTDIR)$(BIN_DIR)/$(EXEC_NAME)
	@rm -rf $(DESTDIR)$(DESKAPPS_DIR)/$(NAME).desktop
	@rm -rf $(DESTDIR)$(PIXMAPS_DIR)/$(NAME).png
	@echo ". ." $(BLUE)", done"$(NONE)

$(NAME) :
	$(CXX) $(DEFAULT_CXXFLAGS) $(CXXFLAGS) $(OBJECTS) -L. ../libxputty/libxputty/libxputty.a -o $(EXEC_NAME) $(LDFLAGS)

doc:
	#pass
