!versionAtLeast(QT_VERSION, 5.12) {
  error("Qt version 5.12.0, or newer, is required.")
}

cache()
include(dooble-source.qt-project)

macx {
CONVERT_DICT = ""
} else {
versionAtLeast(QT_VERSION, 6.0.0) {
freebsd-* {
CONVERT_DICT = "/usr/local/libexec/qt6/qwebengine_convert_dict"
} else:win32 {
qtPrepareTool(CONVERT_DICT, qwebengine_convert_dict)
} else {
qtPrepareTool(CONVERT_DICT, ../libexec/qwebengine_convert_dict)
}
} else {
qtPrepareTool(CONVERT_DICT, qwebengine_convert_dict)
}
}

DICTIONARIES = $$(DOOBLE_DICTIONARIES_DIRECTORY)

isEmpty(DICTIONARIES) {
warning("DOOBLE_DICTIONARIES_DIRECTORY does not exist.")
} else:!exists($$DICTIONARIES) {
warning("DOOBLE_DICTIONARIES_DIRECTORY does not exist.")
}

WEB_DICTIONARIES = qtwebengine_dictionaries

macx {
dict_base_paths = af_ZA/af_ZA \
                  an_ES/an_ES \
                  ar/ar \
                  be_BY/be_BY \
                  bn_BD/bn_BD \
                  ca/ca \
                  ca/ca-valencia \
                  da_DK/da_DK \
                  de/de_AT_frami \
                  de/de_CH_frami \
                  de/de_DE_frami \
                  en/en_AU \
                  en/en_CA \
                  en/en_GB \
                  en/en_US \
                  en/en_ZA \
                  es/es_ANY \
                  fr_FR/fr \
                  gd_GB/gd_GB \
                  gl/gl_ES \
                  gug/gug \
                  he_IL/he_IL \
                  hi_IN/hi_IN \
                  hr_HR/hr_HR \
                  hu_HU/hu_HU \
                  is/is \
                  kmr_Latn/kmr_Latn \
                  lo_LA/lo_LA \
                  ne_NP/ne_NP \
                  nl_NL/nl_NL \
                  no/nb_NO \
                  no/nn_NO \
                  pt_BR/pt_BR \
                  pt_PT/pt_PT \
                  ro/ro_RO \
                  si_LK/si_LK \
                  sk_SK/sk_SK \
                  sr/sr \
                  sr/sr-Latn \
                  sw_TZ/sw_TZ \
                  te_IN/te_IN \
                  uk_UA/uk_UA \
                  vi/vi_VN

dmg.commands = hdiutil create Dooble.dmg -volname Dooble -srcfolder Dooble.d
} else:unix {
dict_base_paths = af_ZA/af_ZA \
                  an_ES/an_ES \
                  ar/ar \
                  be_BY/be_BY \
                  bn_BD/bn_BD \
                  br_FR/br_FR \
                  bs_BA/bs_BA \
                  ca/ca \
                  ca/ca-valencia \
                  cs_CZ/cs_CZ \
                  da_DK/da_DK \
                  de/de_AT_frami \
                  de/de_CH_frami \
                  de/de_DE_frami \
                  el_GR/el_GR \
                  en/en_AU \
                  en/en_CA \
                  en/en_GB \
                  en/en_US \
                  en/en_ZA \
                  es/es_ANY \
                  et_EE/et_EE \
                  fr_FR/fr \
                  gd_GB/gd_GB \
                  gl/gl_ES \
                  gug/gug \
                  he_IL/he_IL \
                  hi_IN/hi_IN \
                  hr_HR/hr_HR \
                  hu_HU/hu_HU \
                  is/is \
                  it_IT/it_IT \
                  kmr_Latn/kmr_Latn \
                  lo_LA/lo_LA \
                  lt_LT/lt \
                  lv_LV/lv_LV \
                  ne_NP/ne_NP \
                  nl_NL/nl_NL \
                  no/nb_NO \
                  no/nn_NO \
                  oc_FR/oc_FR \
                  pl_PL/pl_PL \
                  pt_BR/pt_BR \
                  pt_PT/pt_PT \
                  ro/ro_RO \
                  ru_RU/ru_RU \
                  si_LK/si_LK \
                  sk_SK/sk_SK \
                  sl_SI/sl_SI \
                  sr/sr \
                  sr/sr-Latn \
                  sw_TZ/sw_TZ \
                  te_IN/te_IN \
                  uk_UA/uk_UA \
                  vi/vi_VN
} else:win32 {
dict_base_paths = af_ZA/af_ZA \
                  an_ES/an_ES \
                  ar/ar \
                  be_BY/be_BY \
                  bn_BD/bn_BD \
                  br_FR/br_FR \
                  bs_BA/bs_BA \
                  ca/ca \
                  ca/ca-valencia \
                  cs_CZ/cs_CZ \
                  da_DK/da_DK \
                  de/de_AT_frami \
                  de/de_CH_frami \
                  de/de_DE_frami \
                  el_GR/el_GR \
                  en/en_AU \
                  en/en_CA \
                  en/en_GB \
                  en/en_US \
                  en/en_ZA \
                  es/es_ANY \
                  et_EE/et_EE \
                  fr_FR/fr \
                  gd_GB/gd_GB \
                  gl/gl_ES \
                  gug/gug \
                  he_IL/he_IL \
                  hi_IN/hi_IN \
                  hr_HR/hr_HR \
                  hu_HU/hu_HU \
                  is/is \
                  it_IT/it_IT \
                  kmr_Latn/kmr_Latn \
                  lo_LA/lo_LA \
                  lt_LT/lt \
                  lv_LV/lv_LV \
                  ne_NP/ne_NP \
                  nl_NL/nl_NL \
                  no/nb_NO \
                  no/nn_NO \
                  oc_FR/oc_FR \
                  pl_PL/pl_PL \
                  pt_BR/pt_BR \
                  pt_PT/pt_PT \
                  ro/ro_RO \
                  ru_RU/ru_RU \
                  si_LK/si_LK \
                  sk_SK/sk_SK \
                  sl_SI/sl_SI \
                  sr/sr \
                  sr/sr-Latn \
                  sw_TZ/sw_TZ \
                  te_IN/te_IN \
                  uk_UA/uk_UA \
                  vi/vi_VN
}

isEmpty(DICTIONARIES) {
} else:exists($$DICTIONARIES) {
for(base_path, dict_base_paths) {
dict.files += $$DICTIONARIES/$${base_path}.dic
}
}

dictoolbuild.CONFIG = no_link target_predeps
dictoolbuild.commands = $${CONVERT_DICT} ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
dictoolbuild.depends = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.aff
dictoolbuild.input = dict.files
dictoolbuild.name = ${QMAKE_FILE_IN_BASE} Build
dictoolbuild.output = $${WEB_DICTIONARIES}/${QMAKE_FILE_BASE}.bdic

build_deb.bash =

linux-* {
exists(/usr/bin/dpkg-deb) {
}

exists(/usr/include/gpgme.h) {
DEFINES += DOOBLE_PEEKABOO
LIBS += -lgpgme
}
}

unix {
QMAKE_LFLAGS_RPATH =
purge.commands = find . -name \'*~*\' -exec rm -f {} \;
} else {
purge.commands =
}

FILES = /usr/include/linux/mman.h \
        /usr/include/sys/mman.h

for(file, FILES):exists($$file):{DEFINES += DOOBLE_MMAN_PRESENT}

macx {
DEFINES         += DOOBLE_MMAN_PRESENT
}

CONFIG		+= qt release warn_on
DEFINES         += DOOBLE_REGISTER_JAR_SCHEME QT_DEPRECATED_WARNINGS
LANGUAGE	= C++
QT		+= concurrent \
                   gui \
                   network \
                   printsupport \
                   qml \
                   sql \
                   webenginewidgets \
                   widgets \
                   xml

qtHaveModule(charts) {
DEFINES         += DOOBLE_QTCHARTS_PRESENT
QT              += charts
message("The QtCharts module has been discovered.")
} else {
warning("The QtCharts module is not present. I'm very sorry!")
}

lessThan(QT_MAJOR_VERSION, 6) {
QT              += webengine
}

TEMPLATE	= app

QMAKE_CLEAN     += Dooble

freebsd-* {
# Enable only if FreeBSD's Qt and WebEngine versions differ.
DEFINES -= DOOBLE_FREEBSD_WEBENGINE_MISMATCH
QMAKE_CXXFLAGS_RELEASE += -O3 \
                          -Wall \
                          -Wcast-align \
                          -Wcast-qual \
                          -Werror \
                          -Wextra \
                          -Wformat=2 \
                          -Wold-style-cast \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wstrict-overflow=5 \
                          -Wundef \
                          -Wzero-as-null-pointer-constant \
                          -fPIE \
                          -fstack-protector-all \
                          -funroll-loops \
                          -fwrapv \
                          -pedantic \
                          -std=c++17
QMAKE_CXXFLAGS_RELEASE -= -O2
} else:macx {
QMAKE_CXXFLAGS_RELEASE += -O3 \
                          -Wall \
                          -Wcast-align \
                          -Wcast-qual \
                          -Wextra \
                          -Wformat=2 \
                          -Wno-c++20-attribute-extensions \
                          -Wold-style-cast \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wstrict-overflow=5 \
                          -fPIE \
                          -fstack-protector-all \
                          -funroll-loops \
                          -fwrapv \
                          -pedantic \
                          -std=c++17
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_MACOSX_DEPLOYMENT_TARGET = 12.0
} else:win32 {
versionAtLeast(QT_VERSION, 6.0.0) {
QMAKE_LFLAGS += /entry:mainCRTStartup
}
} else {
QMAKE_CXXFLAGS_RELEASE += -O3 \
                          -Wall \
			  -Warray-bounds=2 \
                          -Wcast-align \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Werror \
                          -Wextra \
                          -Wformat-overflow=2 \
                          -Wformat-security \
			  -Wformat-signedness \
                          -Wformat-truncation=2 \
                          -Wformat=2 \
                          -Wlogical-op \
                          -Wno-deprecated-copy \
                          -Wold-style-cast \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
			  -Wstrict-overflow=1 \
			  -Wstringop-overflow=4 \
                          -Wundef \
                          -Wzero-as-null-pointer-constant \
                          -fstack-clash-protection \
                          -fstack-protector-all \
                          -funroll-loops \
                          -fwrapv \
                          -pedantic \
                          -std=c++17
os2 {
LIBS += -lssp
QMAKE_CXXFLAGS_RELEASE += -Wstrict-overflow=1
QMAKE_CXXFLAGS_RELEASE -= -Wstrict-overflow=5
} else {
QMAKE_CXXFLAGS_RELEASE += -Wl,-z,relro \
                          -fPIE \
                          -pie \
}

versionAtLeast(QT_VERSION, 6.0.0) {
QMAKE_CXXFLAGS_RELEASE += -Wno-int-in-bool-context
}
QMAKE_CXXFLAGS_RELEASE -= -O2
}

QMAKE_DISTCLEAN += -r qtwebengine_dictionaries \
                   .qmake.cache \
                   .qmake.stash \
                   temp

isEmpty(CONVERT_DICT) {
} else {
QMAKE_EXTRA_COMPILERS += dictoolbuild
}

QMAKE_EXTRA_TARGETS = build-deb dmg purge

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

macx {
DISTFILES += $$DICTIONARIES/af_ZA/af_ZA.aff \
             $$DICTIONARIES/af_ZA/af_ZA.dic \
             $$DICTIONARIES/an_ES/an_ES.aff \
             $$DICTIONARIES/an_ES/an_ES.dic \
             $$DICTIONARIES/ar/ar.aff \
             $$DICTIONARIES/ar/ar.dic \
             $$DICTIONARIES/be_BY/be_BY.aff \
             $$DICTIONARIES/be_BY/be_BY.dic \
             $$DICTIONARIES/bn_BD/bn_BD.aff \
             $$DICTIONARIES/bn_BD/bn_BD.dic \
             $$DICTIONARIES/ca/ca.aff \
             $$DICTIONARIES/ca/ca.dic \
             $$DICTIONARIES/ca/ca-valencia.aff \
             $$DICTIONARIES/ca/ca-valencia.dic \
             $$DICTIONARIES/da_DK/da_DK.aff \
             $$DICTIONARIES/da_DK/da_DK.dic \
             $$DICTIONARIES/de/de_AT_frami.aff \
             $$DICTIONARIES/de/de_AT_frami.dic \
             $$DICTIONARIES/de/de_CH_frami.aff \
             $$DICTIONARIES/de/de_CH_frami.dic \
             $$DICTIONARIES/de/de_DE_frami.aff \
             $$DICTIONARIES/de/de_DE_frami.dic \
             $$DICTIONARIES/en/en_AU.aff \
             $$DICTIONARIES/en/en_AU.dic \
             $$DICTIONARIES/en/en_CA.aff \
             $$DICTIONARIES/en/en_CA.dic \
             $$DICTIONARIES/en/en_GB.aff \
             $$DICTIONARIES/en/en_GB.dic \
             $$DICTIONARIES/en/en_US.aff \
             $$DICTIONARIES/en/en_US.dic \
             $$DICTIONARIES/en/en_ZA.aff \
             $$DICTIONARIES/en/en_ZA.dic \
             $$DICTIONARIES/es/es_ANY.aff \
             $$DICTIONARIES/es/es_ANY.dic \
             $$DICTIONARIES/gd_GB/gd_GB.aff \
             $$DICTIONARIES/gd_GB/gd_GB.dic \
             $$DICTIONARIES/gl/gl_ES.aff \
             $$DICTIONARIES/gl/gl_ES.dic \
             $$DICTIONARIES/gug/gug.aff \
             $$DICTIONARIES/gug/gug.dic \
             $$DICTIONARIES/he_IL/he_IL.aff \
             $$DICTIONARIES/he_IL/he_IL.dic \
             $$DICTIONARIES/hi_IN/hi_IN.aff \
             $$DICTIONARIES/hi_IN/hi_IN.dic \
             $$DICTIONARIES/hr_HR/hr_HR.aff \
             $$DICTIONARIES/hr_HR/hr_HR.dic \
             $$DICTIONARIES/hu_HU/hu_HU.aff \
             $$DICTIONARIES/hu_HU/hu_HU.dic \
             $$DICTIONARIES/is/is.aff \
             $$DICTIONARIES/is/is.dic \
             $$DICTIONARIES/kmr_Latn/kmr_Latn.aff \
             $$DICTIONARIES/kmr_Latn/kmr_Latn.dic \
             $$DICTIONARIES/lo_LA/lo_LA.aff \
             $$DICTIONARIES/lo_LA/lo_LA.dic \
             $$DICTIONARIES/ne_NP/ne_NP.aff \
             $$DICTIONARIES/ne_NP/ne_NP.dic \
             $$DICTIONARIES/nl_NL/nl_NL.aff \
             $$DICTIONARIES/nl_NL/nl_NL.dic \
             $$DICTIONARIES/no/nb_NO.aff \
             $$DICTIONARIES/no/nb_NO.dic \
             $$DICTIONARIES/no/nn_NO.aff \
             $$DICTIONARIES/no/nn_NO.dic \
             $$DICTIONARIES/pt_BR/pt_BR.aff \
             $$DICTIONARIES/pt_BR/pt_BR.dic \
             $$DICTIONARIES/pt_PT/pt_PT.aff \
             $$DICTIONARIES/pt_PT/pt_PT.dic \
             $$DICTIONARIES/ro/ro_RO.aff \
             $$DICTIONARIES/ro/ro_RO.dic \
             $$DICTIONARIES/si_LK/si_LK.aff \
             $$DICTIONARIES/si_LK/si_LK.dic \
             $$DICTIONARIES/sk_SK/sk_SK.aff \
             $$DICTIONARIES/sk_SK/sk_SK.dic \
             $$DICTIONARIES/sr/sr.aff \
             $$DICTIONARIES/sr/sr.dic \
             $$DICTIONARIES/sr/sr-Latn.aff \
             $$DICTIONARIES/sr/sr-Latn.dic \
             $$DICTIONARIES/sw_TZ/sw_TZ.aff \
             $$DICTIONARIES/sw_TZ/sw_TZ.dic \
             $$DICTIONARIES/te_IN/te_IN.aff \
             $$DICTIONARIES/te_IN/te_IN.dic \
             $$DICTIONARIES/uk_UA/uk_UA.aff \
             $$DICTIONARIES/uk_UA/uk_UA.dic \
             $$DICTIONARIES/vi/vi_VN.aff \
             $$DICTIONARIES/vi/vi_VN.dic
} else:unix {
DISTFILES += $$DICTIONARIES/af_ZA/af_ZA.aff \
             $$DICTIONARIES/af_ZA/af_ZA.dic \
             $$DICTIONARIES/an_ES/an_ES.aff \
             $$DICTIONARIES/an_ES/an_ES.dic \
             $$DICTIONARIES/ar/ar.aff \
             $$DICTIONARIES/ar/ar.dic \
             $$DICTIONARIES/be_BY/be_BY.aff \
             $$DICTIONARIES/be_BY/be_BY.dic \
             $$DICTIONARIES/bn_BD/bn_BD.aff \
             $$DICTIONARIES/bn_BD/bn_BD.dic \
             $$DICTIONARIES/br_FR/br_FR.aff \
             $$DICTIONARIES/br_FR/br_FR.dic \
             $$DICTIONARIES/bs_BA/bs_BA.aff \
             $$DICTIONARIES/bs_BA/bs_BA.dic \
             $$DICTIONARIES/ca/ca.aff \
             $$DICTIONARIES/ca/ca.dic \
             $$DICTIONARIES/ca/ca-valencia.aff \
             $$DICTIONARIES/ca/ca-valencia.dic \
             $$DICTIONARIES/cs_CZ/cs_CZ.aff \
             $$DICTIONARIES/cs_CZ/cs_CZ.dic \
             $$DICTIONARIES/da_DK/da_DK.aff \
             $$DICTIONARIES/da_DK/da_DK.dic \
             $$DICTIONARIES/de/de_AT_frami.aff \
             $$DICTIONARIES/de/de_AT_frami.dic \
             $$DICTIONARIES/de/de_CH_frami.aff \
             $$DICTIONARIES/de/de_CH_frami.dic \
             $$DICTIONARIES/de/de_DE_frami.aff \
             $$DICTIONARIES/de/de_DE_frami.dic \
             $$DICTIONARIES/el_GR/el_GR.aff \
             $$DICTIONARIES/el_GR/el_GR.dic \
             $$DICTIONARIES/en/en_AU.aff \
             $$DICTIONARIES/en/en_AU.dic \
             $$DICTIONARIES/en/en_CA.aff \
             $$DICTIONARIES/en/en_CA.dic \
             $$DICTIONARIES/en/en_GB.aff \
             $$DICTIONARIES/en/en_GB.dic \
             $$DICTIONARIES/en/en_US.aff \
             $$DICTIONARIES/en/en_US.dic \
             $$DICTIONARIES/en/en_ZA.aff \
             $$DICTIONARIES/en/en_ZA.dic \
             $$DICTIONARIES/es/es_ANY.aff \
             $$DICTIONARIES/es/es_ANY.dic \
             $$DICTIONARIES/et_EE/et_EE.aff \
             $$DICTIONARIES/et_EE/et_EE.dic \
             $$DICTIONARIES/gd_GB/gd_GB.aff \
             $$DICTIONARIES/gd_GB/gd_GB.dic \
             $$DICTIONARIES/gl/gl_ES.aff \
             $$DICTIONARIES/gl/gl_ES.dic \
             $$DICTIONARIES/gug/gug.aff \
             $$DICTIONARIES/gug/gug.dic \
             $$DICTIONARIES/he_IL/he_IL.aff \
             $$DICTIONARIES/he_IL/he_IL.dic \
             $$DICTIONARIES/hi_IN/hi_IN.aff \
             $$DICTIONARIES/hi_IN/hi_IN.dic \
             $$DICTIONARIES/hr_HR/hr_HR.aff \
             $$DICTIONARIES/hr_HR/hr_HR.dic \
             $$DICTIONARIES/hu_HU/hu_HU.aff \
             $$DICTIONARIES/hu_HU/hu_HU.dic \
             $$DICTIONARIES/is/is.aff \
             $$DICTIONARIES/is/is.dic \
             $$DICTIONARIES/it_IT/it_IT.aff \
             $$DICTIONARIES/it_IT/it_IT.dic \
             $$DICTIONARIES/kmr_Latn/kmr_Latn.aff \
             $$DICTIONARIES/kmr_Latn/kmr_Latn.dic \
             $$DICTIONARIES/lo_LA/lo_LA.aff \
             $$DICTIONARIES/lo_LA/lo_LA.dic \
             $$DICTIONARIES/lt_LT/lt.aff \
             $$DICTIONARIES/lt_LT/lt.dic \
             $$DICTIONARIES/lv_LV/lv_LV.aff \
             $$DICTIONARIES/lv_LV/lv_LV.dic \
             $$DICTIONARIES/ne_NP/ne_NP.aff \
             $$DICTIONARIES/ne_NP/ne_NP.dic \
             $$DICTIONARIES/nl_NL/nl_NL.aff \
             $$DICTIONARIES/nl_NL/nl_NL.dic \
             $$DICTIONARIES/no/nb_NO.aff \
             $$DICTIONARIES/no/nb_NO.dic \
             $$DICTIONARIES/no/nn_NO.aff \
             $$DICTIONARIES/no/nn_NO.dic \
             $$DICTIONARIES/oc_FR/oc_FR.aff \
             $$DICTIONARIES/oc_FR/oc_FR.dic \
             $$DICTIONARIES/pl_PL/pl_PL.aff \
             $$DICTIONARIES/pl_PL/pl_PL.dic \
             $$DICTIONARIES/pt_BR/pt_BR.aff \
             $$DICTIONARIES/pt_BR/pt_BR.dic \
             $$DICTIONARIES/pt_PT/pt_PT.aff \
             $$DICTIONARIES/pt_PT/pt_PT.dic \
             $$DICTIONARIES/ro/ro_RO.aff \
             $$DICTIONARIES/ro/ro_RO.dic \
             $$DICTIONARIES/ru_RU/ru_RU.aff \
             $$DICTIONARIES/ru_RU/ru_RU.dic \
             $$DICTIONARIES/si_LK/si_LK.aff \
             $$DICTIONARIES/si_LK/si_LK.dic \
             $$DICTIONARIES/sk_SK/sk_SK.aff \
             $$DICTIONARIES/sk_SK/sk_SK.dic \
             $$DICTIONARIES/sl_SI/sl_SI.aff \
             $$DICTIONARIES/sl_SI/sl_SI.dic \
             $$DICTIONARIES/sr/sr.aff \
             $$DICTIONARIES/sr/sr.dic \
             $$DICTIONARIES/sr/sr-Latn.aff \
             $$DICTIONARIES/sr/sr-Latn.dic \
             $$DICTIONARIES/sw_TZ/sw_TZ.aff \
             $$DICTIONARIES/sw_TZ/sw_TZ.dic \
             $$DICTIONARIES/te_IN/te_IN.aff \
             $$DICTIONARIES/te_IN/te_IN.dic \
             $$DICTIONARIES/uk_UA/uk_UA.aff \
             $$DICTIONARIES/uk_UA/uk_UA.dic \
             $$DICTIONARIES/vi/vi_VN.aff \
             $$DICTIONARIES/vi/vi_VN.dic
} else:win32 {
DISTFILES += $$DICTIONARIES/af_ZA/af_ZA.aff \
             $$DICTIONARIES/af_ZA/af_ZA.dic \
             $$DICTIONARIES/an_ES/an_ES.aff \
             $$DICTIONARIES/an_ES/an_ES.dic \
             $$DICTIONARIES/ar/ar.aff \
             $$DICTIONARIES/ar/ar.dic \
             $$DICTIONARIES/be_BY/be_BY.aff \
             $$DICTIONARIES/be_BY/be_BY.dic \
             $$DICTIONARIES/bn_BD/bn_BD.aff \
             $$DICTIONARIES/bn_BD/bn_BD.dic \
             $$DICTIONARIES/br_FR/br_FR.aff \
             $$DICTIONARIES/br_FR/br_FR.dic \
             $$DICTIONARIES/bs_BA/bs_BA.aff \
             $$DICTIONARIES/bs_BA/bs_BA.dic \
             $$DICTIONARIES/ca/ca.aff \
             $$DICTIONARIES/ca/ca.dic \
             $$DICTIONARIES/ca/ca-valencia.aff \
             $$DICTIONARIES/ca/ca-valencia.dic \
             $$DICTIONARIES/cs_CZ/cs_CZ.aff \
             $$DICTIONARIES/cs_CZ/cs_CZ.dic \
             $$DICTIONARIES/da_DK/da_DK.aff \
             $$DICTIONARIES/da_DK/da_DK.dic \
             $$DICTIONARIES/de/de_AT_frami.aff \
             $$DICTIONARIES/de/de_AT_frami.dic \
             $$DICTIONARIES/de/de_CH_frami.aff \
             $$DICTIONARIES/de/de_CH_frami.dic \
             $$DICTIONARIES/de/de_DE_frami.aff \
             $$DICTIONARIES/de/de_DE_frami.dic \
             $$DICTIONARIES/el_GR/el_GR.aff \
             $$DICTIONARIES/el_GR/el_GR.dic \
             $$DICTIONARIES/en/en_AU.aff \
             $$DICTIONARIES/en/en_AU.dic \
             $$DICTIONARIES/en/en_CA.aff \
             $$DICTIONARIES/en/en_CA.dic \
             $$DICTIONARIES/en/en_GB.aff \
             $$DICTIONARIES/en/en_GB.dic \
             $$DICTIONARIES/en/en_US.aff \
             $$DICTIONARIES/en/en_US.dic \
             $$DICTIONARIES/en/en_ZA.aff \
             $$DICTIONARIES/en/en_ZA.dic \
             $$DICTIONARIES/es/es_ANY.aff \
             $$DICTIONARIES/es/es_ANY.dic \
             $$DICTIONARIES/et_EE/et_EE.aff \
             $$DICTIONARIES/et_EE/et_EE.dic \
             $$DICTIONARIES/gd_GB/gd_GB.aff \
             $$DICTIONARIES/gd_GB/gd_GB.dic \
             $$DICTIONARIES/gl/gl_ES.aff \
             $$DICTIONARIES/gl/gl_ES.dic \
             $$DICTIONARIES/gug/gug.aff \
             $$DICTIONARIES/gug/gug.dic \
             $$DICTIONARIES/he_IL/he_IL.aff \
             $$DICTIONARIES/he_IL/he_IL.dic \
             $$DICTIONARIES/hi_IN/hi_IN.aff \
             $$DICTIONARIES/hi_IN/hi_IN.dic \
             $$DICTIONARIES/hr_HR/hr_HR.aff \
             $$DICTIONARIES/hr_HR/hr_HR.dic \
             $$DICTIONARIES/hu_HU/hu_HU.aff \
             $$DICTIONARIES/hu_HU/hu_HU.dic \
             $$DICTIONARIES/is/is.aff \
             $$DICTIONARIES/is/is.dic \
             $$DICTIONARIES/it_IT/it_IT.aff \
             $$DICTIONARIES/it_IT/it_IT.dic \
             $$DICTIONARIES/kmr_Latn/kmr_Latn.aff \
             $$DICTIONARIES/kmr_Latn/kmr_Latn.dic \
             $$DICTIONARIES/lo_LA/lo_LA.aff \
             $$DICTIONARIES/lo_LA/lo_LA.dic \
             $$DICTIONARIES/lt_LT/lt.aff \
             $$DICTIONARIES/lt_LT/lt.dic \
             $$DICTIONARIES/lv_LV/lv_LV.aff \
             $$DICTIONARIES/lv_LV/lv_LV.dic \
             $$DICTIONARIES/ne_NP/ne_NP.aff \
             $$DICTIONARIES/ne_NP/ne_NP.dic \
             $$DICTIONARIES/nl_NL/nl_NL.aff \
             $$DICTIONARIES/nl_NL/nl_NL.dic \
             $$DICTIONARIES/no/nb_NO.aff \
             $$DICTIONARIES/no/nb_NO.dic \
             $$DICTIONARIES/no/nn_NO.aff \
             $$DICTIONARIES/no/nn_NO.dic \
             $$DICTIONARIES/oc_FR/oc_FR.aff \
             $$DICTIONARIES/oc_FR/oc_FR.dic \
             $$DICTIONARIES/pl_PL/pl_PL.aff \
             $$DICTIONARIES/pl_PL/pl_PL.dic \
             $$DICTIONARIES/pt_BR/pt_BR.aff \
             $$DICTIONARIES/pt_BR/pt_BR.dic \
             $$DICTIONARIES/pt_PT/pt_PT.aff \
             $$DICTIONARIES/pt_PT/pt_PT.dic \
             $$DICTIONARIES/ro/ro_RO.aff \
             $$DICTIONARIES/ro/ro_RO.dic \
             $$DICTIONARIES/ru_RU/ru_RU.aff \
             $$DICTIONARIES/ru_RU/ru_RU.dic \
             $$DICTIONARIES/si_LK/si_LK.aff \
             $$DICTIONARIES/si_LK/si_LK.dic \
             $$DICTIONARIES/sk_SK/sk_SK.aff \
             $$DICTIONARIES/sk_SK/sk_SK.dic \
             $$DICTIONARIES/sl_SI/sl_SI.aff \
             $$DICTIONARIES/sl_SI/sl_SI.dic \
             $$DICTIONARIES/sr/sr.aff \
             $$DICTIONARIES/sr/sr.dic \
             $$DICTIONARIES/sr/sr-Latn.aff \
             $$DICTIONARIES/sr/sr-Latn.dic \
             $$DICTIONARIES/sw_TZ/sw_TZ.aff \
             $$DICTIONARIES/sw_TZ/sw_TZ.dic \
             $$DICTIONARIES/te_IN/te_IN.aff \
             $$DICTIONARIES/te_IN/te_IN.dic \
             $$DICTIONARIES/uk_UA/uk_UA.aff \
             $$DICTIONARIES/uk_UA/uk_UA.dic \
             $$DICTIONARIES/vi/vi_VN.aff \
             $$DICTIONARIES/vi/vi_VN.dic
}

macx {
OBJECTIVE_HEADERS += Source/Cocoainitializer.h
OBJECTIVE_SOURCES += Source/Cocoainitializer.mm
}

win32 {
RC_FILE         = Icons/dooble.rc
}

UI_HEADERS_DIR  = Source

PROJECTNAME	= Dooble
TARGET		= Dooble

!win32 {
VERSION         = DOOBLE_VERSION
}

macx {
copycharts.files        = Charts/*
copycharts.path         = Dooble.d/Charts
copydata.files		= Data/*.txt
copydata.path		= Dooble.d/Data
copydocumentation.extra	= cp ./Documentation/Documents/*.pdf Dooble.d/Documentation/. && cp ./Documentation/REMINDERS Dooble.d/Documentation/.
copydocumentation.path	= Dooble.d/Documentation
copydooble.extra	= cp -r ./Dooble.app Dooble.d/.
copydooble.path		= Dooble.d
copyinfoplist.extra	= cp Data/Info.plist Dooble.d/Dooble.app/Contents/.
copyinfoplist.path	= Dooble.d
macdeployqt.extra	= $$[QT_INSTALL_BINS]/macdeployqt Dooble.d/Dooble.app -executable=Dooble.d/Dooble.app/Contents/MacOS/Dooble
macdeployqt.path	= Dooble.app
purgeheaders.extra	= rm -fr Dooble.d/Dooble.app/Contents/Frameworks/QtWebEngineCore.framework/Headers && rm -fr Dooble.d/Dooble.app/Contents/Frameworks/QtWebEngineCore.framework/Versions/5/Headers
purgeheaders.path	= Dooble.d
preinstall.extra	= rm -fr Dooble.d/Dooble.app
preinstall.path		= Dooble.d
translations.files	= Translations/*.qm
translations.path	= Dooble.d/Translations

INSTALLS	= copycharts \
                  copydata \
                  copydocumentation \
                  preinstall \
                  copydooble \
                  macdeployqt \
		  copyinfoplist \
		  purgeheaders \
                  translations
}

macx:app_bundle {
isEmpty(CONVERT_DICT) {
} else {
for (base_path, dict_base_paths) {
base_path_splitted = $$split(base_path, /)
base_name = $$last(base_path_splitted)
binary_dict_files.files += $${WEB_DICTIONARIES}/$${base_name}.bdic
}

binary_dict_files.path = Contents/Resources/$$WEB_DICTIONARIES
QMAKE_BUNDLE_DATA += binary_dict_files
}
}
