LEVEL	:= .
DIRS	:= 2006-10 2006-10-MR2 9.12-bach

default: all

$(DIRS)::
	$(MAKE) -C $@ $(MAKECMDGOALS)

all clean: $(DIRS)
