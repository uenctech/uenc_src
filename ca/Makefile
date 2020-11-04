OBJECTS =  ca_rawtx.o \
		   ca.o \
		   ca_transaction.o \
		   ca_message.o \
		   ca_hexcode.o  \
		   ca_block.o \
		   ca_serialize.o \
		   ca_parr.o \
		   ca_cstr.o \
		   ca_util.o \
		   ca_sha2.o \
		   ca_buint.o \
		   ca_blkdb.o \
		   ca_global.o \
		   ca_hashtab.o \
		   ca_ripemd160.o \
		   Crypto_ECDSA.o \
		   ca_uxto.o \
		   ca_txdb.o \
		   ca_file.o \
		   ca_base58.o \
		   ca_pbkdf2.o \
		   ca_sha3_256.o \
		   ca_sha3.o \
		   ca_hmac.o \
		   ca_bip39.o \
		   ca_rand.o \
		   ca_uuid4.o \
		   ca_clientinfo.o\
		   ../include/cJSON.o \
		   ../include/logging.o\
		   ../include/config.o\
		   ca_interface.o\
		   ca_console.o

		   

LIBS = ../crypto/cryptopp/libcryptopp.a
LIBDIR = -L../crypto/cryptopp
INCL = -I../include -I../crypto/cryptopp
CFLAGS = -std=c++11 -fPIC -DDEBUG -D _CA_FILTER_FUN_ $(INCL) $(LIBDIR) 
CC = g++

TARGET = libca.so

ALL: $(TARGET)
$(TARGET):$(OBJECTS)
	$(CC)  $(OBJECTS) $(LIBS) $(INCL) $(LIBDIR) $(CFLAGS) -shared -o $(TARGET)

$(OBJECTS):%.o:%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS)
