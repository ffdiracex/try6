
EXTRA_DIST = Manifest

# Need to include ../src in addition to top_srcdir because gcrypt.h is
# a built header.
AM_CPPFLAGS = -I../src -I$(top_srcdir)/src
AM_CFLAGS = $(GPG_ERROR_CFLAGS)


noinst_LTLIBRARIES = libcipher.la

GCRYPT_MODULES = @GCRYPT_CIPHERS@ @GCRYPT_PUBKEY_CIPHERS@ @GCRYPT_DIGESTS@

libcipher_la_DEPENDENCIES = $(GCRYPT_MODULES)
libcipher_la_LIBADD = $(GCRYPT_MODULES)

libcipher_la_SOURCES = \
cipher.c pubkey.c ac.c md.c kdf.c \
hmac-tests.c \
bithelp.h  \
primegen.c  \
hash-common.c hash-common.h \
rmd.h

EXTRA_libcipher_la_SOURCES = \
arcfour.c \
blowfish.c \
cast5.c \
crc.c \
des.c \
dsa.c \
elgamal.c \
ecc.c \
idea.c \
md4.c \
md5.c \
rijndael.c rijndael-tables.h \
rmd160.c \
rsa.c \
seed.c \
serpent.c \
sha1.c \
sha256.c \
sha512.c \
tiger.c \
whirlpool.c \
twofish.c \
rfc2268.c \
camellia.c camellia.h camellia-glue.c

if ENABLE_O_FLAG_MUNGING
o_flag_munging = sed -e 's/-O\([2-9s][2-9s]*\)/-O1/' -e 's/-Ofast/-O1/g'
else
o_flag_munging = cat
endif


# We need to lower the optimization for this module.
tiger.o: $(srcdir)/tiger.c
	`echo $(COMPILE) -c $(srcdir)/tiger.c | $(o_flag_munging) `

tiger.lo: $(srcdir)/tiger.c
	`echo $(LTCOMPILE) -c $(srcdir)/tiger.c | $(o_flag_munging) `
