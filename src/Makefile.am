bin_PROGRAMS = fasterpussycat

AM_CXXFLAGS = $(IDN_CFLAGS) $(CRYPTO_CFLAGS) $(M_CFLAGS) $(MAGIC_CFLAGS) $(SQLITE3_CFLAGS) $(SSL_CFLAGS) $(Z_CFLAGS)

fasterpussycat_SOURCES = fasterpussycat.c engine.c engine.h bayes.c bayes.h post.c post.h tadns.c tadns.h \
  http_client.c http_client.h match_rule.c match_rule.h id3.c id3.h ac.c ac.h db.c db.h string-inl.h alloc-inl.h \
  util.c util.h detect_404.h detect_404.c backup_bruteforce.c backup_bruteforce.h find_uploaded_file.c \
  find_uploaded_file.h

fasterpussycat_LDADD = $(IDN_LIBS)  $(CRYPTO_LIBS) $(M_LIBS) $(MAGIC_LIBS) $(SQLITE3_LIBS) $(SSL_LIBS) $(Z_LIBS)
