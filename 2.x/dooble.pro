cache()

libspoton.commands = $(MAKE) -C libSpotOn library
libspoton.depends =
libspoton.target = libspoton.so
purge.commands = rm -f Documentation/*~ Include/*~ Installers/*~ \
                 Source/*~ *~

CONFIG		+= qt release warn_on
LANGUAGE	= C++
QT		+= concurrent gui network printsupport sql \
	           webenginewidgets widgets xml
TEMPLATE	= app

DEFINES         += DOOBLE_LINKED_WITH_LIBSPOTON
QMAKE_CLEAN     += Dooble libSpotOn/*.o libSpotOn/*.so libSpotOn/test
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -fPIE -fstack-protector-all -fwrapv \
			  -mtune=generic -pie -std=c++11 -Os \
			  -Wall -Wcast-align -Wcast-qual \
			  -Werror -Wextra \
			  -Woverloaded-virtual -Wpointer-arith \
			  -Wstack-protector -Wstrict-overflow=5
QMAKE_DISTCLEAN += -r temp .qmake.cache .qmake.stash
QMAKE_EXTRA_TARGETS = libspoton purge

INCLUDEPATH	+= Source
LIBS		+= -LlibSpotOn -lgcrypt -lgpg-error -lspoton
PRE_TARGETDEPS = libspoton.so

MOC_DIR = temp/moc
OBJECTS_DIR = temp/obj
RCC_DIR = temp/rcc
UI_DIR = temp/ui

FORMS           = UI\\dooble.ui

HEADERS		= Source\\dooble.h

RESOURCES       +=

SOURCES		= Source\\dooble.cc

TRANSLATIONS    =

UI_HEADERS_DIR  = Source

PROJECTNAME	= Dooble
TARGET		= Dooble
