lessThan(QT_VERSION, 5.9.1) {
error("Qt version 5.9.1, or newer, is required.")
}

cache()
qtPrepareTool(CONVERT_TOOL, qwebengine_convert_dict)
DICTIONARIES_DIR = qtwebengine_dictionaries
dict_base_paths = en/en_US

for(base_path, dict_base_paths) {
dict.files += $$PWD/Dictionaries/$${base_path}.dic
}

dictoolbuild.CONFIG = no_link target_predeps
dictoolbuild.commands = $${CONVERT_TOOL} ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
dictoolbuild.depends = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.aff
dictoolbuild.input = dict.files
dictoolbuild.name = Build ${QMAKE_FILE_IN_BASE}
dictoolbuild.output = $${DICTIONARIES_DIR}/${QMAKE_FILE_BASE}.bdic

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

macx {
QMAKE_CXXFLAGS_RELEASE += -Wall -Wcast-align -Wcast-qual \
			  -Werror -Wextra \
			  -Woverloaded-virtual -Wpointer-arith \
			  -Wstack-protector -Wstrict-overflow=5 \
                          -fPIE -fstack-protector-all -fwrapv \
                          -mtune=generic -std=c++11
} else:win32 {
QMAKE_CXXFLAGS_RELEASE +=
} else {
QMAKE_CXXFLAGS_RELEASE += -Wall -Wcast-align -Wcast-qual \
			  -Werror -Wextra \
			  -Woverloaded-virtual -Wpointer-arith \
			  -Wstack-protector -Wstrict-overflow=5 \
                          -fPIE -fstack-protector-all -fwrapv \
                          -mtune=generic -pie -std=c++11
}

QMAKE_DISTCLEAN += -r qtwebengine_dictionaries temp .qmake.cache .qmake.stash
QMAKE_EXTRA_COMPILERS += dictoolbuild
QMAKE_EXTRA_TARGETS = purge

macx {
ICON            = Icons/Logo/dooble.icns
}

INCLUDEPATH	+= Source

macx {
LIBS		+= -framework Cocoa
} else:win32 {
LIBS            += -ladvapi32 -lcrypt32
}

PRE_TARGETDEPS =

MOC_DIR = temp/moc
OBJECTS_DIR = temp/obj
RCC_DIR = temp/rcc
UI_DIR = temp/ui

DISTFILES += Dictionaries/en/en_US.dic \
             Dictionaries/en/en_US.aff

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

win32 {
RC_FILE         = Icons\\dooble.rc
}

SOURCES		= Source\\dooble.cc \
                  Source\\dooble_about.cc \
                  Source\\dooble_accepted_or_blocked_domains.cc \
                  Source\\dooble_address_widget.cc \
                  Source\\dooble_address_widget_completer.cc \
                  Source\\dooble_address_widget_completer_popup.cc \
                  Source\\dooble_aes256.cc \
                  Source\\dooble_application.cc \
                  Source\\dooble_block_cipher.cc \
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
                  Source\\dooble_threefish256.cc \
                  Source\\dooble_tool_button.cc \
                  Source\\dooble_ui_utilities.cc \
		  Source\\dooble_web_engine_url_request_interceptor.cc \
                  Source\\dooble_web_engine_page.cc \
                  Source\\dooble_web_engine_view.cc

TRANSLATIONS    =

UI_HEADERS_DIR  = Source

PROJECTNAME	= Dooble
TARGET		= Dooble

macx {
data.path          = /Applications/Dooble.d/Data
data.files         = Data/*.txt
dooble.path	   = /Applications/Dooble.d/Dooble.app
dooble.files	   = Dooble.app/*
install_tool.path  = .
install_tool.extra = install_name_tool -change @rpath/QtWebEngineCore.framework/Versions/5/QtWebEngineCore /Applications/Dooble.d/Dooble.app/Contents/Frameworks/QtWebEngineCore.framework/Versions/5/QtWebEngineCore /Applications/Dooble.d/Dooble.app/Contents/Frameworks/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess
macdeployqt.path   = Dooble.app
macdeployqt.extra  = $$[QT_INSTALL_BINS]/macdeployqt ./Dooble.app
preinstall.path    = /Applications/Dooble.d
preinstall.extra   = rm -rf /Applications/Dooble.d/Dooble.app
postinstall.path   = /Applications/Dooble.d
postinstall.extra  = cp -r ./Dooble.app /Applications/Dooble.d/.

INSTALLS	= preinstall \
                  data \
                  macdeployqt \
                  dooble \
                  postinstall \
                  install_tool
}

macx:app_bundle {
for (base_path, dict_base_paths) {
base_path_splitted = $$split(base_path, /)
base_name = $$last(base_path_splitted)
binary_dict_files.files += $${DICTIONARIES_DIR}/$${base_name}.bdic
}

binary_dict_files.path = Contents/Resources/$$DICTIONARIES_DIR
QMAKE_BUNDLE_DATA += binary_dict_files
}
