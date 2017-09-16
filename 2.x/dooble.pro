lessThan(QT_VERSION, 5.9.1) {
error("Qt version 5.9.1, or newer, is required.")
}

cache()

unix {
purge.commands = rm -f Documentation/*~ Include/*~ Installers/*~ \
                 Source/*~ *~
}
else {
purge.commands =
}

CONFIG		+= qt release warn_on
DEFINES         += QT_DEPRECATED_WARNINGS
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
} else:win32 {
QMAKE_CXXFLAGS_RELEASE +=
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

macx {
ICON            = Icons/Logo/dooble.icns
}

INCLUDEPATH	+= Source

macx {
LIBS		+= -framework Cocoa
}

PRE_TARGETDEPS =

MOC_DIR = temp/moc
OBJECTS_DIR = temp/obj
RCC_DIR = temp/rcc
UI_DIR = temp/ui

FORMS           = UI\\dooble.ui \
                  UI\\dooble_about.ui \
                  UI\\dooble_accepted_or_blocked_domains.ui \
                  UI\\dooble_authentication_dialog.ui \
                  UI\\dooble_certificate_exceptions_menu_widget.ui \
                  UI\\dooble_certificate_exceptions_widget.ui \
                  UI\\dooble_clear_items.ui \
		  UI\\dooble_cookies_window.ui \
                  UI\\dooble_downloads.ui \
                  UI\\dooble_downloads_item.ui \
		  UI\\dooble_history_window.ui \
                  UI\\dooble_page.ui \
		  UI\\dooble_popup_menu.ui \
                  UI\\dooble_settings.ui

HEADERS		= Source\\dooble.h \
                  Source\\dooble_about.h \
                  Source\\dooble_accepted_or_blocked_domains.h \
                  Source\\dooble_address_widget.h \
                  Source\\dooble_address_widget_completer.h \
                  Source\\dooble_address_widget_completer_popup.h \
                  Source\\dooble_application.h \
                  Source\\dooble_certificate_exceptions_menu_widget.h \
                  Source\\dooble_clear_items.h \
                  Source\\dooble_cookies.h \
                  Source\\dooble_cookies_window.h \
                  Source\\dooble_cryptography.h \
                  Source\\dooble_downloads.h \
                  Source\\dooble_downloads_item.h \
                  Source\\dooble_favicons.h \
                  Source\\dooble_gopher.h \
                  Source\\dooble_history.h \
                  Source\\dooble_history_window.h \
                  Source\\dooble_label_widget.h \
                  Source\\dooble_page.h \
                  Source\\dooble_pbkdf2.h \
                  Source\\dooble_popup_menu.h \
                  Source\\dooble_search_widget.h \
                  Source\\dooble_settings.h \
                  Source\\dooble_tab_bar.h \
                  Source\\dooble_tab_widget.h \
                  Source\\dooble_tool_button.h \
		  Source\\dooble_web_engine_url_request_interceptor.h \
                  Source\\dooble_web_engine_page.h \
                  Source\\dooble_web_engine_view.h

macx {
OBJECTIVE_HEADERS += Include/Cocoainitializer.h
OBJECTIVE_SOURCES += Source/Cocoainitializer.mm
}

RESOURCES       += Icons\\icons.qrc

SOURCES		= Source\\dooble.cc \
                  Source\\dooble_about.cc \
                  Source\\dooble_accepted_or_blocked_domains.cc \
                  Source\\dooble_address_widget.cc \
                  Source\\dooble_address_widget_completer.cc \
                  Source\\dooble_address_widget_completer_popup.cc \
                  Source\\dooble_aes256.cc \
                  Source\\dooble_application.cc \
                  Source\\dooble_certificate_exceptions_menu_widget.cc \
                  Source\\dooble_clear_items.cc \
                  Source\\dooble_cookies.cc \
                  Source\\dooble_cookies_window.cc \
                  Source\\dooble_cryptography.cc \
                  Source\\dooble_downloads.cc \
                  Source\\dooble_downloads_item.cc \
                  Source\\dooble_favicons.cc \
                  Source\\dooble_gopher.cc \
                  Source\\dooble_history.cc \
                  Source\\dooble_history_window.cc \
		  Source\\dooble_hmac.cc \
                  Source\\dooble_label_widget.cc \
                  Source\\dooble_main.cc \
                  Source\\dooble_page.cc \
                  Source\\dooble_pbkdf2.cc \
                  Source\\dooble_popup_menu.cc \
                  Source\\dooble_random.cc \
                  Source\\dooble_search_widget.cc \
                  Source\\dooble_settings.cc \
                  Source\\dooble_tab_bar.cc \
                  Source\\dooble_tab_widget.cc \
                  Source\\dooble_text_utilities.cc \
                  Source\\dooble_tool_button.cc \
                  Source\\dooble_ui_utilities.cc \
		  Source\\dooble_web_engine_url_request_interceptor.cc \
                  Source\\dooble_web_engine_page.cc \
                  Source\\dooble_web_engine_view.cc

TRANSLATIONS    =

UI_HEADERS_DIR  = Source

PROJECTNAME	= Dooble
TARGET		= Dooble
