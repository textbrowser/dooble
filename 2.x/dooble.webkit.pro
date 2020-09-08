cache()
include(dooble-source.pro)
purge.commands = rm -f Documentation/*~ Source/*~ *~

CONFIG		+= qt release warn_on
DEFINES         += DOOBLE_WEBKIT_ENABLED \
		   QT_DEPRECATED_WARNINGS
LANGUAGE	= C++
QT		+= concurrent \
                   gui \
                   network \
                   printsupport \
                   sql \
                   webkit
TEMPLATE	= app

QMAKE_CLEAN     += Dooble
QMAKE_CXXFLAGS_RELEASE += -O3 \
                          -Wall \
                          -Wcast-align \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Werror \
                          -Wextra \
                          -Wformat-overflow=2 \
                          -Wformat-truncation=2 \
                          -Wformat=2 \
                          -Wl,-z,relro \
                          -Wno-deprecated-copy \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wstrict-overflow=5 \
                          -Wundef \
                          -Wzero-as-null-pointer-constant \
                          -fPIE \
                          -fstack-protector-all \
                          -fwrapv \
                          -mtune=generic \
                          -pedantic \
                          -pie
QMAKE_CXXFLAGS_RELEASE -= -O2

QMAKE_DISTCLEAN += .qmake.cache \
                   .qmake.stash \
                   temp
QMAKE_EXTRA_TARGETS = purge
INCLUDEPATH	+= Source
PRE_TARGETDEPS =

MOC_DIR = temp/moc
OBJECTS_DIR = temp/obj
RCC_DIR = temp/rcc
UI_DIR = temp/ui
UI_HEADERS_DIR  = Source

PROJECTNAME	= Dooble
TARGET		= Dooble
VERSION         = DOOBLE_VERSION
