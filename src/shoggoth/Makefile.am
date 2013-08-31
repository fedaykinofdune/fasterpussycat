AUTOMAKE_OPTIONS = subdir-objects
# what flags you want to pass to the C compiler & linker
AM_CFLAGS =  -Werror -O3 -g 

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = shoggoth 

shoggoth_SOURCES = shoggoth.c zeromq.c zeromq.h common/zeromq_common.h http_response.c http_response.h http_request.c \
                   http_request.h prepared_http_request.c prepared_http_request.h common/simple_buffer.c common/simple_buffer.h common/uthash.h address_map.c \
                   address_map.h server_endpoint.c server_endpoint.h connection.c connection.h connection_pool.c \
                   connection_pool.h global_options.h default_options.c

shoggoth_LDADD = $(SSL_LIBS) $(Z_LIBS) $(ZMQ_LIBS)
