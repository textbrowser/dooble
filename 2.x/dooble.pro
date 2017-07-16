cache()

purge.commands = rm -f Documentation/*~ Include/*~ Installers/*~ \
                 Source/*~ *~

CONFIG		+= qt release warn_on
LANGUAGE	= C++
QT		+= concurrent gui network printsupport sql \
	           webenginewidgets widgets xml
TEMPLATE	= app

QMAKE_CLEAN     += Dooble
QMAKE_CXXFLAGS_RELEASE -= -O2

macx {
QMAKE_CXXFLAGS_RELEASE += -Wall -Wcast-align -Wcast-qual \
			  -Werror -Wextra \
			  -Woverloaded-virtual -Wpointer-arith \
			  -Wstack-protector -Wstrict-overflow=5 \
                          -fPIE -fstack-protector-all -fwrapv \
                          -mtune=generic -std=c++11 -O3
} else {
QMAKE_CXXFLAGS_RELEASE += -Wall -Wcast-align -Wcast-qual \
			  -Werror -Wextra \
			  -Woverloaded-virtual -Wpointer-arith \
			  -Wstack-protector -Wstrict-overflow=5 \
                          -fPIE -fstack-protector-all -fwrapv \
                          -mtune=generic -pie -std=c++11 -O3
}

QMAKE_DISTCLEAN += -r temp .qmake.cache .qmake.stash
QMAKE_EXTRA_TARGETS = purge

INCLUDEPATH	+= Source
LIBS		+=
PRE_TARGETDEPS =

MOC_DIR = temp/moc
OBJECTS_DIR = temp/obj
RCC_DIR = temp/rcc
UI_DIR = temp/ui

FORMS           = UI\\dooble.ui \
                  UI\\dooble_page.ui

HEADERS		= Source\\dooble.h \
                  Source\\dooble_address_widget.h \
                  Source\\dooble_label_widget.h \
                  Source\\dooble_page.h \
                  Source\\dooble_tab_bar.h \
                  Source\\dooble_tab_widget.h \
                  Source\\dooble_web_engine_page.h \
                  Source\\dooble_web_engine_view.h

RESOURCES       += Icons\\icons.qrc

SOURCES		= Source\\dooble.cc \
                  Source\\dooble_address_widget.cc \
                  Source\\dooble_label_widget.cc \
                  Source\\dooble_main.cc \
                  Source\\dooble_page.cc \
                  Source\\dooble_tab_bar.cc \
                  Source\\dooble_tab_widget.cc \
                  Source\\dooble_web_engine_page.cc \
                  Source\\dooble_web_engine_view.cc

TRANSLATIONS    =

UI_HEADERS_DIR  = Source

PROJECTNAME	= Dooble
TARGET		= Dooble
