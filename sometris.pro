TEMPLATE = app
CONFIG += console
CONFIG -= qt

include(other.pro)
SOURCES += ./game_common.c \
./game_gfx.c \
./main.c

HEADERS += ./common.h \
./game_common.h \
./game_gfx.h \

