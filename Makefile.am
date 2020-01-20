bin_PROGRAMS=powerlink-cn-joystick

powerlink_cn_joystick_SOURCES=\
	src/main.c \
	src/app.c \
	src/app.h \
	src/event.c \
	src/event.h \
	src/netselect.c \
	src/netselect.h \
	src/obdcreate.c \
	src/obdcreate.h \
	src/system-linux.c \
	src/system.h \
	src/options.c \
	src/options.h \
	src/screen.c \
	src/screen.h \
	src/CiA401_CN/objdict.h
powerlink_cn_joystick_CFLAGS=-I src/CiA401_CN $(NCURSES_CFLAGS) $(OPENPOWERLINK_CFLAGS)
powerlink_cn_joystick_LDADD=$(NCURSES_LIBS) $(OPENPOWERLINK_LIBS)

