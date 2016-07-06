EXECPATH = bin
OBJPATH = obj
WEBPATH = web/plugins

ifeq ($(strip $(T)),1)
CC = mipsel-openwrt-linux-gcc 
LDFLAGS = -lpthread -ldl
SQOBJS = sqlite3.o
else
CC ?= gcc
LDFLAGS = -lpthread -ldl -lsqlite3
endif

CFLAGS = -std=gnu99 -Wall -g -I include

EXEC = \
	cgi_server_test
EXEC := $(addprefix $(EXECPATH)/,$(EXEC))

OBJS = \
	cgi_factory.o \
	cgi_param_slist.o \
	cgi_url_dltrie.o \
	cgi_event_dispatcher.o \
	cgi_async.o \
	bs.o \
	db.o \
	kv.o \
	list.o \
	request.o \
	response.o \
	template.o \
	models/like.o \
	models/account.o \
	models/connection.o \
	models/session.o \
	models/post.o 
ifeq ($(strip $(T)),1)
OBJS += $(SQOBJS)
endif
OBJS := $(addprefix $(OBJPATH)/,$(OBJS))

PLUGINS = \
	about \
	connect \
	dashboard \
	home \
	like \
	login \
	logout \
	notFound \
	post \
	profile \
	search \
	signup \
	static_handle \
	unlike 

PLUGINS := $(addprefix $(WEBPATH)/,$(PLUGINS))
PLUGINS := $(addsuffix .so,$(PLUGINS))

all: $(OBJPATH) $(OBJS) $(TESTOBJS) \
     $(EXECPATH) $(EXEC) \
     $(WEBPATH) $(PLUGINS)

$(EXECPATH)/%_test: $(OBJS) $(OBJPATH)/%_test.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(EXECPATH):
	@mkdir -p $@

$(OBJPATH)/%.o: test/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJPATH)/%.o: src/%.c
	$(CC) $(CFLAGS) -c -fPIC -o $@ $<

$(OBJPATH):
	@mkdir -p $@ $@/models

$(WEBPATH)/%.so: web/src/%.c
	$(CC) $(CFLAGS) -I web/include -fPIC -shared -nostartfiles \
		$(OBJS) -o $@ $<

$(WEBPATH):
	@mkdir -p $@

clean:
	-rm -rf $(EXEC) $(OBJS) $(TESTOBJS) $(PLUGINS)
distclean: clean
	-rm -rf $(EXECPATH) $(OBJPATH) $(WEBPATH)
