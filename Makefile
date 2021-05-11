TARGET = ebpc
OBJ_PATH = objs
PREFIX_BIN =

CRYPTO_DIR = ./crypto
CRYPTOPP_DIR = ./crypto/cryptopp/
CRYPTOPP_TARGET = libcryptopp.a
CRYPTOPP_LIB = $(CRYPTOPP_DIR)/$(CRYPTOPP_TARGET)

ROCKSDB_DIR = ./rocksdb
ROCKSDB_TARGET = librocksdb.a
ROCKSDB_LIB = $(ROCKSDB_DIR)/$(ROCKSDB_TARGET)

PROTOBUF_DIR = ./protobuf
PROTOBUF_TARGET = /src/.libs/libprotobuf.a
PROTOBUF_LIB = $(PROTOBUF_DIR)/$(PROTOBUF_TARGET)

BOOST_DIR = ./boost

CC = gcc
CPP = g++ -std=c++17
INCLUDES += -I./ -I../include -I$(ROCKSDB_DIR)/include/ -I$(PROTOBUF_DIR)/src -I ./proto
LIBS = $(CRYPTOPP_LIB)  $(ROCKSDB_LIB) $(PROTOBUF_LIB) /usr/lib64/libpthread.so 

G = -g
CFLAGS :=-Wall  -Wno-unknown-pragmas $(G) 
LINKFLAGS = -ldl -lz


SRCDIR =. ./include ./utils ./ca ./net ./common  ./main ./ca/proto  ./proto

C_SRCDIR = $(SRCDIR)
C_SOURCES = $(foreach d,$(C_SRCDIR),$(wildcard $(d)/*.c) )
C_OBJS = $(patsubst %.c, $(OBJ_PATH)/%.o, $(C_SOURCES))
C_DEPEND = $(patsubst %.c, $(OBJ_PATH)/%.d, $(C_SOURCES))


CC_SRCDIR = $(SRCDIR)
CC_SOURCES = $(foreach d,$(CC_SRCDIR),$(wildcard $(d)/*.cc) )
CC_OBJS = $(patsubst %.cc, $(OBJ_PATH)/%.o, $(CC_SOURCES))
CC_DEPEND = $(patsubst %.cc, $(OBJ_PATH)/%.d, $(CC_SOURCES))


CPP_SRCDIR = $(SRCDIR)
CPP_SOURCES = $(foreach d,$(CPP_SRCDIR),$(wildcard $(d)/*.cpp) )
CPP_OBJS = $(patsubst %.cpp, $(OBJ_PATH)/%.o, $(CPP_SOURCES))
CPP_DEPEND = $(patsubst %.cpp, $(OBJ_PATH)/%.d, $(CPP_SOURCES))


default:genproto testnet init compile version

primary:genproto primarynet init compile version
test:genproto testnet init compile version

version:
	sh ./gen_version_info.sh


genproto:
	# sh ./tools/gen_proto.sh

$(C_OBJS):$(OBJ_PATH)/%.o:%.c
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

$(CC_OBJS):$(OBJ_PATH)/%.o:%.cc
	$(CPP) -c $(CFLAGS) $(INCLUDES) $< -o $@

$(CPP_OBJS):$(OBJ_PATH)/%.o:%.cpp
	$(CPP) -c $(CFLAGS) $(INCLUDES) $< -o $@


init:subdirs_compile
	$(foreach d,$(SRCDIR), mkdir -p $(OBJ_PATH)/$(d);)

test:
	@echo "C_SOURCES: $(C_SOURCES)"
	@echo "C_OBJS: $(C_OBJS)"
	@echo "CPP_SOURCES: $(CPP_SOURCES)"
	@echo "CPP_OBJS: $(CPP_OBJS)"
	@echo "CC_SOURCES: $(CC_SOURCES)"
	@echo "CC_OBJS: $(CC_OBJS)"

compile:$(C_OBJS) $(CC_OBJS) $(CPP_OBJS)
	$(CPP)  $^ -o $(TARGET)  $(LINKFLAGS) $(LIBS)

clean:
	rm -rf $(OBJ_PATH)
	rm -rf $(TARGET)
cleand:
	find ./objs -name *.d | xargs rm -rf

testnet:
	sed -i "s/g_testflag.*;/g_testflag = 1;/g" ./common/global.cpp
primarynet:
	sed -i "s/g_testflag.*;/g_testflag = 0;/g" ./common/global.cpp

cleanall:subdirs_clean
	rm -rf $(OBJ_PATH)
	rm -f $(TARGET)

install: $(TARGET)
	cp $(TARGET) $(PREFIX_BIN)

uninstall:
	rm -f $(PREFIX_BIN)/$(TARGET)

rebuild: clean init compile

subdirs_compile:
	# cryptopp
	if [ -d $(CRYPTOPP_DIR) ]; \
	then \
		# cd $(CRYPTOPP_DIR) && make -j4;\
		echo "cryptopp compile";\
	else\
		mkdir -p $(CRYPTO_DIR);\
		unzip ./3rd/cryptopp-CRYPTOPP_8_2_0.zip -d ./;\
		mv cryptopp-CRYPTOPP_8_2_0 cryptopp;\
		mv cryptopp $(CRYPTO_DIR);\
		# rm -rf  cryptopp-CRYPTOPP_8_2_0.zip;\
		cd $(CRYPTOPP_DIR) && make -j4;\
	fi;\

	# rocksdb
	if [ -d $(ROCKSDB_DIR) ]; \
	then \
		# cd $(ROCKSDB_DIR) && make -j4 static_lib;\
		echo "rocksdb compile";\
	else\
		unzip ./3rd/rocksdb-6.4.6.zip -d ./;\
		mv rocksdb-6.4.6 rocksdb;\
		cd $(ROCKSDB_DIR) && make -j4 static_lib;\
	fi;\

	# protobuf
	if [ -d $(PROTOBUF_DIR) ]; \
	then \
		# cd $(PROTOBUF_DIR) && make -j4;\
		echo "protobuf compile";\
	else\
		unzip ./3rd/protobuf-3.11.1.zip -d ./;\
		mv protobuf-3.11.1 protobuf;\
		cd $(PROTOBUF_DIR) && ./autogen.sh && ./configure && make -j4;\
	fi;\

	# boost
	if [ -d $(BOOST_DIR) ]; \
	then \
		echo "boost ok";\
	else\
		tar -zxvf  ./3rd/boost-1.72.tar.gz ./;\
	fi;\


subdirs_clean:
	cd $(CRYPTOPP_DIR) && make clean
	cd $(ROCKSDB_DIR) && make clean
	cd $(PROTOBUF_DIR) && make clean



ifeq ($(MAKECMDGOALS),)
include $(CPP_DEPEND) init
endif

# $* 不带后缀的target
# $< target
# $@ depend
# $$xx  变量要用两个$$表示
# $$$$  进程号
# sed中的变量用 '"包围， 's,\('"$$BASESRC"'\)\.o[ :]*,\objs\/$*.o $@ : ,g'
$(CPP_DEPEND):$(OBJ_PATH)/%.d:%.cpp
	BASESRC=`basename $*`;\
	set -e; rm -f $@; \
	$(CPP) $(INCLUDES) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\('"$$BASESRC"'\)\.o[ :]*,\objs\/$*.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

