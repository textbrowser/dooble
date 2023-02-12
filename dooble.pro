!versionAtLeast(QT_VERSION, 5.12) {
  error("Qt version 5.12.0, or newer, is required.")
}

cache()
include(dooble-source.pro)

versionAtLeast(QT_VERSION, 6.0.0) {
win32 {
qtPrepareTool(CONVERT_TOOL, qwebengine_convert_dict)
} else {
qtPrepareTool(CONVERT_TOOL, ../libexec/qwebengine_convert_dict)
}
} else {
qtPrepareTool(CONVERT_TOOL, qwebengine_convert_dict)
}

DICTIONARIES_DIR = qtwebengine_dictionaries

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

dmg.commands = hdiutil create ~/Dooble.dmg -volname Dooble -srcfolder /Applications/Dooble.d
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
                  fr_FR/fr \
                  et_EE/et_EE \
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

for(base_path, dict_base_paths) {
dict.files += $$PWD/Dictionaries/$${base_path}.dic
}

dictoolbuild.CONFIG = no_link target_predeps
dictoolbuild.commands = $${CONVERT_TOOL} ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
dictoolbuild.depends = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.aff
dictoolbuild.input = dict.files
dictoolbuild.name = ${QMAKE_FILE_IN_BASE} Build
dictoolbuild.output = $${DICTIONARIES_DIR}/${QMAKE_FILE_BASE}.bdic

build_deb.bash =

linux {
exists(/usr/bin/dpkg-deb) {
build-deb.commands = Distributions/build_deb.bash
}
}

unix {
QMAKE_LFLAGS_RPATH =
purge.commands = find . -name '*~*' -exec rm -f {} \;
} else {
purge.commands =
}

doxygen.commands =

exists(/usr/bin/doxygen) {
doxygen.commands = doxygen dooble.doxygen
}

FILES = /usr/include/linux/mman.h \
        /usr/include/sys/mman.h

for(file, FILES):exists($$file):{DEFINES += DOOBLE_MMAN_PRESENT}

macx {
DEFINES         += DOOBLE_MMAN_PRESENT
}

CONFIG		+= qt release warn_on
DEFINES         += QT_DEPRECATED_WARNINGS
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
                          -Wdouble-promotion \
                          -Werror \
                          -Wextra \
                          -Wformat=2 \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wstrict-overflow=5 \
                          -Wundef \
                          -Wzero-as-null-pointer-constant \
                          -fPIE \
                          -fstack-protector-all \
                          -fwrapv \
                          -pedantic \
                          -std=c++17
QMAKE_CXXFLAGS_RELEASE -= -O2
} else:macx {
QMAKE_CXXFLAGS_RELEASE += -O3 \
                          -Wall \
                          -Wcast-align \
                          -Wcast-qual \
                          -Werror \
                          -Wextra \
                          -Wformat=2 \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wstrict-overflow=5 \
                          -Wzero-as-null-pointer-constant \
                          -fPIE \
                          -fstack-protector-all \
                          -fwrapv \
                          -pedantic \
                          -std=c++17
QMAKE_CXXFLAGS_RELEASE -= -O2
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
			  -Wformat-signedness \
                          -Wformat-truncation=2 \
                          -Wformat=2 \
                          -Wlogical-op \
                          -Wno-deprecated-copy \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
			  -Wstringop-overflow=2 \
                          -Wundef \
                          -Wzero-as-null-pointer-constant \
                          -fstack-protector-all \
                          -fwrapv \
                          -pedantic \
                          -std=c++17
os2 {
# -fstack-protector requires manual lib spec
LIBS += -lssp
} else {
QMAKE_CXXFLAGS_RELEASE += \
                          -Wl,-z,relro \
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
QMAKE_EXTRA_COMPILERS += dictoolbuild
QMAKE_EXTRA_TARGETS = build-deb dmg doxygen purge

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
DISTFILES += Dictionaries/af_ZA/af_ZA.aff \
             Dictionaries/af_ZA/af_ZA.dic \
             Dictionaries/an_ES/an_ES.aff \
             Dictionaries/an_ES/an_ES.dic \
             Dictionaries/ar/ar.aff \
             Dictionaries/ar/ar.dic \
             Dictionaries/be_BY/be_BY.aff \
             Dictionaries/be_BY/be_BY.dic \
             Dictionaries/bn_BD/bn_BD.aff \
             Dictionaries/bn_BD/bn_BD.dic \
             Dictionaries/ca/ca.aff \
             Dictionaries/ca/ca.dic \
             Dictionaries/ca/ca-valencia.aff \
             Dictionaries/ca/ca-valencia.dic \
             Dictionaries/da_DK/da_DK.aff \
             Dictionaries/da_DK/da_DK.dic \
             Dictionaries/de/de_AT_frami.aff \
             Dictionaries/de/de_AT_frami.dic \
             Dictionaries/de/de_CH_frami.aff \
             Dictionaries/de/de_CH_frami.dic \
             Dictionaries/de/de_DE_frami.aff \
             Dictionaries/de/de_DE_frami.dic \
             Dictionaries/en/en_AU.aff \
             Dictionaries/en/en_AU.dic \
             Dictionaries/en/en_CA.aff \
             Dictionaries/en/en_CA.dic \
             Dictionaries/en/en_GB.aff \
             Dictionaries/en/en_GB.dic \
             Dictionaries/en/en_US.aff \
             Dictionaries/en/en_US.dic \
             Dictionaries/en/en_ZA.aff \
             Dictionaries/en/en_ZA.dic \
             Dictionaries/es/es_ANY.aff \
             Dictionaries/es/es_ANY.dic \
             Dictionaries/gd_GB/gd_GB.aff \
             Dictionaries/gd_GB/gd_GB.dic \
             Dictionaries/gl/gl_ES.aff \
             Dictionaries/gl/gl_ES.dic \
             Dictionaries/gug/gug.aff \
             Dictionaries/gug/gug.dic \
             Dictionaries/he_IL/he_IL.aff \
             Dictionaries/he_IL/he_IL.dic \
             Dictionaries/hi_IN/hi_IN.aff \
             Dictionaries/hi_IN/hi_IN.dic \
             Dictionaries/hr_HR/hr_HR.aff \
             Dictionaries/hr_HR/hr_HR.dic \
             Dictionaries/hu_HU/hu_HU.aff \
             Dictionaries/hu_HU/hu_HU.dic \
             Dictionaries/is/is.aff \
             Dictionaries/is/is.dic \
             Dictionaries/kmr_Latn/kmr_Latn.aff \
             Dictionaries/kmr_Latn/kmr_Latn.dic \
             Dictionaries/lo_LA/lo_LA.aff \
             Dictionaries/lo_LA/lo_LA.dic \
             Dictionaries/ne_NP/ne_NP.aff \
             Dictionaries/ne_NP/ne_NP.dic \
             Dictionaries/nl_NL/nl_NL.aff \
             Dictionaries/nl_NL/nl_NL.dic \
             Dictionaries/no/nb_NO.aff \
             Dictionaries/no/nb_NO.dic \
             Dictionaries/no/nn_NO.aff \
             Dictionaries/no/nn_NO.dic \
             Dictionaries/pt_BR/pt_BR.aff \
             Dictionaries/pt_BR/pt_BR.dic \
             Dictionaries/pt_PT/pt_PT.aff \
             Dictionaries/pt_PT/pt_PT.dic \
             Dictionaries/ro/ro_RO.aff \
             Dictionaries/ro/ro_RO.dic \
             Dictionaries/si_LK/si_LK.aff \
             Dictionaries/si_LK/si_LK.dic \
             Dictionaries/sk_SK/sk_SK.aff \
             Dictionaries/sk_SK/sk_SK.dic \
             Dictionaries/sr/sr.aff \
             Dictionaries/sr/sr.dic \
             Dictionaries/sr/sr-Latn.aff \
             Dictionaries/sr/sr-Latn.dic \
             Dictionaries/sw_TZ/sw_TZ.aff \
             Dictionaries/sw_TZ/sw_TZ.dic \
             Dictionaries/te_IN/te_IN.aff \
             Dictionaries/te_IN/te_IN.dic \
             Dictionaries/uk_UA/uk_UA.aff \
             Dictionaries/uk_UA/uk_UA.dic \
             Dictionaries/vi/vi_VN.aff \
             Dictionaries/vi/vi_VN.dic
} else:unix {
DISTFILES += Dictionaries/af_ZA/af_ZA.aff \
             Dictionaries/af_ZA/af_ZA.dic \
             Dictionaries/an_ES/an_ES.aff \
             Dictionaries/an_ES/an_ES.dic \
             Dictionaries/ar/ar.aff \
             Dictionaries/ar/ar.dic \
             Dictionaries/be_BY/be_BY.aff \
             Dictionaries/be_BY/be_BY.dic \
             Dictionaries/bn_BD/bn_BD.aff \
             Dictionaries/bn_BD/bn_BD.dic \
             Dictionaries/br_FR/br_FR.aff \
             Dictionaries/br_FR/br_FR.dic \
             Dictionaries/bs_BA/bs_BA.aff \
             Dictionaries/bs_BA/bs_BA.dic \
             Dictionaries/ca/ca.aff \
             Dictionaries/ca/ca.dic \
             Dictionaries/ca/ca-valencia.aff \
             Dictionaries/ca/ca-valencia.dic \
             Dictionaries/cs_CZ/cs_CZ.aff \
             Dictionaries/cs_CZ/cs_CZ.dic \
             Dictionaries/da_DK/da_DK.aff \
             Dictionaries/da_DK/da_DK.dic \
             Dictionaries/de/de_AT_frami.aff \
             Dictionaries/de/de_AT_frami.dic \
             Dictionaries/de/de_CH_frami.aff \
             Dictionaries/de/de_CH_frami.dic \
             Dictionaries/de/de_DE_frami.aff \
             Dictionaries/de/de_DE_frami.dic \
             Dictionaries/el_GR/el_GR.aff \
             Dictionaries/el_GR/el_GR.dic \
             Dictionaries/en/en_AU.aff \
             Dictionaries/en/en_AU.dic \
             Dictionaries/en/en_CA.aff \
             Dictionaries/en/en_CA.dic \
             Dictionaries/en/en_GB.aff \
             Dictionaries/en/en_GB.dic \
             Dictionaries/en/en_US.aff \
             Dictionaries/en/en_US.dic \
             Dictionaries/en/en_ZA.aff \
             Dictionaries/en/en_ZA.dic \
             Dictionaries/es/es_ANY.aff \
             Dictionaries/es/es_ANY.dic \
             Dictionaries/et_EE/et_EE.aff \
             Dictionaries/et_EE/et_EE.dic \
             Dictionaries/gd_GB/gd_GB.aff \
             Dictionaries/gd_GB/gd_GB.dic \
             Dictionaries/gl/gl_ES.aff \
             Dictionaries/gl/gl_ES.dic \
             Dictionaries/gug/gug.aff \
             Dictionaries/gug/gug.dic \
             Dictionaries/he_IL/he_IL.aff \
             Dictionaries/he_IL/he_IL.dic \
             Dictionaries/hi_IN/hi_IN.aff \
             Dictionaries/hi_IN/hi_IN.dic \
             Dictionaries/hr_HR/hr_HR.aff \
             Dictionaries/hr_HR/hr_HR.dic \
             Dictionaries/hu_HU/hu_HU.aff \
             Dictionaries/hu_HU/hu_HU.dic \
             Dictionaries/is/is.aff \
             Dictionaries/is/is.dic \
             Dictionaries/it_IT/it_IT.aff \
             Dictionaries/it_IT/it_IT.dic \
             Dictionaries/kmr_Latn/kmr_Latn.aff \
             Dictionaries/kmr_Latn/kmr_Latn.dic \
             Dictionaries/lo_LA/lo_LA.aff \
             Dictionaries/lo_LA/lo_LA.dic \
             Dictionaries/lt_LT/lt.aff \
             Dictionaries/lt_LT/lt.dic \
             Dictionaries/lv_LV/lv_LV.aff \
             Dictionaries/lv_LV/lv_LV.dic \
             Dictionaries/ne_NP/ne_NP.aff \
             Dictionaries/ne_NP/ne_NP.dic \
             Dictionaries/nl_NL/nl_NL.aff \
             Dictionaries/nl_NL/nl_NL.dic \
             Dictionaries/no/nb_NO.aff \
             Dictionaries/no/nb_NO.dic \
             Dictionaries/no/nn_NO.aff \
             Dictionaries/no/nn_NO.dic \
             Dictionaries/oc_FR/oc_FR.aff \
             Dictionaries/oc_FR/oc_FR.dic \
             Dictionaries/pl_PL/pl_PL.aff \
             Dictionaries/pl_PL/pl_PL.dic \
             Dictionaries/pt_BR/pt_BR.aff \
             Dictionaries/pt_BR/pt_BR.dic \
             Dictionaries/pt_PT/pt_PT.aff \
             Dictionaries/pt_PT/pt_PT.dic \
             Dictionaries/ro/ro_RO.aff \
             Dictionaries/ro/ro_RO.dic \
             Dictionaries/ru_RU/ru_RU.aff \
             Dictionaries/ru_RU/ru_RU.dic \
             Dictionaries/si_LK/si_LK.aff \
             Dictionaries/si_LK/si_LK.dic \
             Dictionaries/sk_SK/sk_SK.aff \
             Dictionaries/sk_SK/sk_SK.dic \
             Dictionaries/sl_SI/sl_SI.aff \
             Dictionaries/sl_SI/sl_SI.dic \
             Dictionaries/sr/sr.aff \
             Dictionaries/sr/sr.dic \
             Dictionaries/sr/sr-Latn.aff \
             Dictionaries/sr/sr-Latn.dic \
             Dictionaries/sw_TZ/sw_TZ.aff \
             Dictionaries/sw_TZ/sw_TZ.dic \
             Dictionaries/te_IN/te_IN.aff \
             Dictionaries/te_IN/te_IN.dic \
             Dictionaries/uk_UA/uk_UA.aff \
             Dictionaries/uk_UA/uk_UA.dic \
             Dictionaries/vi/vi_VN.aff \
             Dictionaries/vi/vi_VN.dic
} else:win32 {
DISTFILES += Dictionaries/af_ZA/af_ZA.aff \
             Dictionaries/af_ZA/af_ZA.dic \
             Dictionaries/an_ES/an_ES.aff \
             Dictionaries/an_ES/an_ES.dic \
             Dictionaries/ar/ar.aff \
             Dictionaries/ar/ar.dic \
             Dictionaries/be_BY/be_BY.aff \
             Dictionaries/be_BY/be_BY.dic \
             Dictionaries/bn_BD/bn_BD.aff \
             Dictionaries/bn_BD/bn_BD.dic \
             Dictionaries/br_FR/br_FR.aff \
             Dictionaries/br_FR/br_FR.dic \
             Dictionaries/bs_BA/bs_BA.aff \
             Dictionaries/bs_BA/bs_BA.dic \
             Dictionaries/ca/ca.aff \
             Dictionaries/ca/ca.dic \
             Dictionaries/ca/ca-valencia.aff \
             Dictionaries/ca/ca-valencia.dic \
             Dictionaries/cs_CZ/cs_CZ.aff \
             Dictionaries/cs_CZ/cs_CZ.dic \
             Dictionaries/da_DK/da_DK.aff \
             Dictionaries/da_DK/da_DK.dic \
             Dictionaries/de/de_AT_frami.aff \
             Dictionaries/de/de_AT_frami.dic \
             Dictionaries/de/de_CH_frami.aff \
             Dictionaries/de/de_CH_frami.dic \
             Dictionaries/de/de_DE_frami.aff \
             Dictionaries/de/de_DE_frami.dic \
             Dictionaries/el_GR/el_GR.aff \
             Dictionaries/el_GR/el_GR.dic \
             Dictionaries/en/en_AU.aff \
             Dictionaries/en/en_AU.dic \
             Dictionaries/en/en_CA.aff \
             Dictionaries/en/en_CA.dic \
             Dictionaries/en/en_GB.aff \
             Dictionaries/en/en_GB.dic \
             Dictionaries/en/en_US.aff \
             Dictionaries/en/en_US.dic \
             Dictionaries/en/en_ZA.aff \
             Dictionaries/en/en_ZA.dic \
             Dictionaries/es/es_ANY.aff \
             Dictionaries/es/es_ANY.dic \
             Dictionaries/et_EE/et_EE.aff \
             Dictionaries/et_EE/et_EE.dic \
             Dictionaries/gd_GB/gd_GB.aff \
             Dictionaries/gd_GB/gd_GB.dic \
             Dictionaries/gl/gl_ES.aff \
             Dictionaries/gl/gl_ES.dic \
             Dictionaries/gug/gug.aff \
             Dictionaries/gug/gug.dic \
             Dictionaries/he_IL/he_IL.aff \
             Dictionaries/he_IL/he_IL.dic \
             Dictionaries/hi_IN/hi_IN.aff \
             Dictionaries/hi_IN/hi_IN.dic \
             Dictionaries/hr_HR/hr_HR.aff \
             Dictionaries/hr_HR/hr_HR.dic \
             Dictionaries/hu_HU/hu_HU.aff \
             Dictionaries/hu_HU/hu_HU.dic \
             Dictionaries/is/is.aff \
             Dictionaries/is/is.dic \
             Dictionaries/it_IT/it_IT.aff \
             Dictionaries/it_IT/it_IT.dic \
             Dictionaries/kmr_Latn/kmr_Latn.aff \
             Dictionaries/kmr_Latn/kmr_Latn.dic \
             Dictionaries/lo_LA/lo_LA.aff \
             Dictionaries/lo_LA/lo_LA.dic \
             Dictionaries/lt_LT/lt.aff \
             Dictionaries/lt_LT/lt.dic \
             Dictionaries/lv_LV/lv_LV.aff \
             Dictionaries/lv_LV/lv_LV.dic \
             Dictionaries/ne_NP/ne_NP.aff \
             Dictionaries/ne_NP/ne_NP.dic \
             Dictionaries/nl_NL/nl_NL.aff \
             Dictionaries/nl_NL/nl_NL.dic \
             Dictionaries/no/nb_NO.aff \
             Dictionaries/no/nb_NO.dic \
             Dictionaries/no/nn_NO.aff \
             Dictionaries/no/nn_NO.dic \
             Dictionaries/oc_FR/oc_FR.aff \
             Dictionaries/oc_FR/oc_FR.dic \
             Dictionaries/pl_PL/pl_PL.aff \
             Dictionaries/pl_PL/pl_PL.dic \
             Dictionaries/pt_BR/pt_BR.aff \
             Dictionaries/pt_BR/pt_BR.dic \
             Dictionaries/pt_PT/pt_PT.aff \
             Dictionaries/pt_PT/pt_PT.dic \
             Dictionaries/ro/ro_RO.aff \
             Dictionaries/ro/ro_RO.dic \
             Dictionaries/ru_RU/ru_RU.aff \
             Dictionaries/ru_RU/ru_RU.dic \
             Dictionaries/si_LK/si_LK.aff \
             Dictionaries/si_LK/si_LK.dic \
             Dictionaries/sk_SK/sk_SK.aff \
             Dictionaries/sk_SK/sk_SK.dic \
             Dictionaries/sl_SI/sl_SI.aff \
             Dictionaries/sl_SI/sl_SI.dic \
             Dictionaries/sr/sr.aff \
             Dictionaries/sr/sr.dic \
             Dictionaries/sr/sr-Latn.aff \
             Dictionaries/sr/sr-Latn.dic \
             Dictionaries/sw_TZ/sw_TZ.aff \
             Dictionaries/sw_TZ/sw_TZ.dic \
             Dictionaries/te_IN/te_IN.aff \
             Dictionaries/te_IN/te_IN.dic \
             Dictionaries/uk_UA/uk_UA.aff \
             Dictionaries/uk_UA/uk_UA.dic \
             Dictionaries/vi/vi_VN.aff \
             Dictionaries/vi/vi_VN.dic
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
copycharts.path         = /Applications/Dooble.d/Charts
copydata.files		= Data/*.txt
copydata.path		= /Applications/Dooble.d/Data
copydocumentation.extra	= cp ./Documentation/Documents/*.pdf /Applications/Dooble.d/Documentation/. && cp ./Documentation/TO-DO /Applications/Dooble.d/Documentation/.
copydocumentation.path	= /Applications/Dooble.d/Documentation
copydooble.extra	= cp -r ./Dooble.app /Applications/Dooble.d/.
copydooble.path		= /Applications/Dooble.d
copyinfoplist.extra	= cp Data/Info.plist /Applications/Dooble.d/Dooble.app/Contents/.
copyinfoplist.path	= /Applications/Dooble.d
copywebengine.extra	= cp -r $$[QT_INSTALL_LIBS]/QtWebEngineCore.framework /Applications/Dooble.d/Dooble.app/Contents/Frameworks/.
copywebengine.path	= /Applications/Dooble.d
macdeployqt.extra	= $$[QT_INSTALL_BINS]/macdeployqt /Applications/Dooble.d/Dooble.app -executable=/Applications/Dooble.d/Dooble.app/Contents/MacOS/Dooble
macdeployqt.path	= Dooble.app
purgeheaders.extra	= rm -fr /Applications/Dooble.d/Dooble.app/Contents/Frameworks/QtWebEngineCore.framework/Headers && rm -fr /Applications/Dooble.d/Dooble.app/Contents/Frameworks/QtWebEngineCore.framework/Versions/5/Headers 
purgeheaders.path	= /Applications/Dooble.d
preinstall.extra	= rm -fr /Applications/Dooble.d/Dooble.app
preinstall.path		= /Applications/Dooble.d
translations.files	= Translations/*.qm
translations.path	= /Applications/Dooble.d/Translations
zzz.extra		= chown -Rh root:wheel /Applications/Dooble.d
zzz.path		= /Applications/Dooble.d

INSTALLS	= copycharts \
                  copydata \
                  copydocumentation \
                  preinstall \
                  copydooble \
                  macdeployqt \
		  copyinfoplist \
		  copywebengine \
		  purgeheaders \
                  translations \
		  zzz
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
