# Somewhat generic Makefile modified from example:
# http://www.cs.swarthmore.edu/~newhall/unixhelp/howto_makefiles.html
#
# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .o and executable files
#

# define the C++ compiler to use
CC = g++

# define any compile-time flags
CFLAGS = -Wall -g -pg
CXXFLAGS=`root-config --cflags` -std=c++0x
LDFLAGS=`root-config --ldflags`
LDLIBS=`root-config --glibs`

# define any directories containing header files other than /usr/include
#
INCLUDES = -Imakeclass -Itclap
# INCLUDES = -I/home/newhall/include  -I../include

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
LFLAGS = 
# LFLAGS = -L/home/newhall/lib  -L../lib

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname 
#   option, something like (this will link in libmylib.so and libm.so:
LIBS =
# LIBS = -lmylib -lm

# define the C++ source files
# SRCS = $(wildcard *.cc) $(wildcard *.C)
SRCS = $(wildcard *.cc)

# define the C++ object files 
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .cc of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.cc=.o)

# define the executable file 
MAIN = haddnorm

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean

all:    $(MAIN)
	@echo  Simple compiler named $(MAIN) has been compiled

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LDLIBS) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .cc file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
# .c.o is equivalent to %.o : %.c
.cc.o:
	$(CC) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) -c $<  -o $@
	# $(CC) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) -c $<  -o $@ $(LDLIBS) $(LFLAGS) $(LIBS)

clean:
	rm $(MAIN) $(OBJS)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it
