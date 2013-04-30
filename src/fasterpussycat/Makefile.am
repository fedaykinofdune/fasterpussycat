AUTOMAKE_OPTIONS = subdir-objects
# what flags you want to pass to the C compiler & linker
AM_CFLAGS =  -Werror -O3 -g 

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = fasterpussycat

fasterpussycat_SOURCES = common/zeromq_common.h shoggoth_api.c shoggoth_api.h common/simple_buffer.c base64.c base64.h common/simple_buffer.h lua_helpers.c lua_helpers.h fasterpussycat.c 

fasterpussycat_LDADD = $(ZMQ_LIBS) $(LUA_LIBS)
