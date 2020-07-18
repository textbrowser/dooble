!versionAtLeast(QT_VERSION, 5.12) {
  error("Qt version 5.12.0, or newer, is required.")
}

cache()
qtPrepareTool(CONVERT_TOOL, qwebengine_convert_dict)
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

unix {
purge.commands = rm -f Documentation/*~ Source/*~ *~
} else {
purge.commands =
}

doxygen.commands =

exists(/usr/bin/doxygen) {
doxygen.commands = doxygen dooble.doxygen
}

CONFIG		+= qt release warn_on
DEFINES         += QT_DEPRECATED_WARNINGS
LANGUAGE	= C++
QT		+= concurrent \
                   gui \
                   network \
                   printsupport \
                   sql \
                   webenginewidgets \
                   widgets \
                   xml
TEMPLATE	= app

QMAKE_CLEAN     += Dooble

freebsd-* {
DEFINES += DOOBLE_FREEBSD_WEBENGINE_MISMATCH
QMAKE_CXXFLAGS_RELEASE += -O3 \
                          -Wall \
                          -Wcast-align \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Werror \
                          -Wextra \
                          -Wformat=2 \
                          -Wformat-overflow=2 \
                          -Wformat-truncation=2 \
                          -Wl,-z,relro \
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
                          -std=c++11
QMAKE_CXXFLAGS_RELEASE -= -O2
} else:macx {
QMAKE_CXXFLAGS_RELEASE += -O3 \
                          -Wall \
                          -Wcast-align \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Werror \
                          -Wextra \
                          -Wformat=2 \
                          -Wformat-overflow=2 \
                          -Wformat-truncation=2 \
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
                          -std=c++17
QMAKE_CXXFLAGS_RELEASE -= -O2
} else:win32 {
} else {
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
                          -pie \
                          -std=c++17
QMAKE_CXXFLAGS_RELEASE -= -O2
}

QMAKE_DISTCLEAN += -r qtwebengine_dictionaries \
                   .qmake.cache \
                   .qmake.stash \
                   temp
QMAKE_EXTRA_COMPILERS += dictoolbuild
QMAKE_EXTRA_TARGETS = dmg doxygen purge

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

FORMS           = UI/dooble.ui \
                  UI/dooble_about.ui \
                  UI/dooble_accepted_or_blocked_domains.ui \
                  UI/dooble_authenticate.ui \
                  UI/dooble_authentication_dialog.ui \
                  UI/dooble_certificate_exceptions.ui \
                  UI/dooble_certificate_exceptions_menu_widget.ui \
                  UI/dooble_certificate_exceptions_widget.ui \
                  UI/dooble_clear_items.ui \
		  UI/dooble_cookies_window.ui \
                  UI/dooble_downloads.ui \
                  UI/dooble_downloads_item.ui \
                  UI/dooble_favorites_popup.ui \
                  UI/dooble_floating_digital_clock.ui \
		  UI/dooble_history_window.ui \
                  UI/dooble_page.ui \
                  UI/dooble_popup_menu.ui \
                  UI/dooble_search_engines_popup.ui \
                  UI/dooble_settings.ui \
                  UI/dooble_style_sheet.ui

HEADERS		= Source/dooble.h \
                  Source/dooble_about.h \
                  Source/dooble_accepted_or_blocked_domains.h \
                  Source/dooble_address_widget.h \
                  Source/dooble_address_widget_completer.h \
                  Source/dooble_address_widget_completer_popup.h \
                  Source/dooble_application.h \
                  Source/dooble_certificate_exceptions.h \
                  Source/dooble_certificate_exceptions_menu_widget.h \
                  Source/dooble_clear_items.h \
                  Source/dooble_cookies.h \
                  Source/dooble_cookies_window.h \
                  Source/dooble_cryptography.h \
                  Source/dooble_downloads.h \
                  Source/dooble_downloads_item.h \
                  Source/dooble_favorites_popup.h \
                  Source/dooble_gopher.h \
                  Source/dooble_history.h \
                  Source/dooble_history_table_widget.h \
                  Source/dooble_history_window.h \
                  Source/dooble_page.h \
                  Source/dooble_pbkdf2.h \
                  Source/dooble_popup_menu.h \
                  Source/dooble_search_engines_popup.h \
                  Source/dooble_search_widget.h \
                  Source/dooble_settings.h \
                  Source/dooble_style_sheet.h \
                  Source/dooble_tab_bar.h \
                  Source/dooble_tab_widget.h \
                  Source/dooble_table_view.h \
                  Source/dooble_tool_button.h \
                  Source/dooble_version.h \
		  Source/dooble_web_engine_url_request_interceptor.h \
                  Source/dooble_web_engine_page.h \
                  Source/dooble_web_engine_view.h

macx {
OBJECTIVE_HEADERS += Source/Cocoainitializer.h
OBJECTIVE_SOURCES += Source/Cocoainitializer.mm
}

RESOURCES       += Documentation/documentation.qrc \
                   Icons/icons.qrc

win32 {
RC_FILE         = Icons/dooble.rc
}

SOURCES		= Source/dooble.cc \
                  Source/dooble_about.cc \
                  Source/dooble_accepted_or_blocked_domains.cc \
                  Source/dooble_address_widget.cc \
                  Source/dooble_address_widget_completer.cc \
                  Source/dooble_address_widget_completer_popup.cc \
                  Source/dooble_aes256.cc \
                  Source/dooble_application.cc \
                  Source/dooble_block_cipher.cc \
                  Source/dooble_certificate_exceptions.cc \
                  Source/dooble_certificate_exceptions_menu_widget.cc \
                  Source/dooble_clear_items.cc \
                  Source/dooble_cookies.cc \
                  Source/dooble_cookies_window.cc \
                  Source/dooble_cryptography.cc \
                  Source/dooble_database_utilities.cc \
                  Source/dooble_downloads.cc \
                  Source/dooble_downloads_item.cc \
                  Source/dooble_favicons.cc \
                  Source/dooble_favorites_popup.cc \
		  Source/dooble_gopher.cc \
                  Source/dooble_history.cc \
                  Source/dooble_history_table_widget.cc \
                  Source/dooble_history_window.cc \
                  Source/dooble_hmac.cc \
                  Source/dooble_main.cc \
                  Source/dooble_page.cc \
                  Source/dooble_pbkdf2.cc \
                  Source/dooble_popup_menu.cc \
                  Source/dooble_random.cc \
                  Source/dooble_search_engines_popup.cc \
                  Source/dooble_search_widget.cc \
                  Source/dooble_settings.cc \
                  Source/dooble_style_sheet.cc \
                  Source/dooble_tab_bar.cc \
                  Source/dooble_tab_widget.cc \
                  Source/dooble_table_view.cc \
                  Source/dooble_text_utilities.cc \
                  Source/dooble_threefish256.cc \
                  Source/dooble_tool_button.cc \
                  Source/dooble_ui_utilities.cc \
		  Source/dooble_web_engine_url_request_interceptor.cc \
                  Source/dooble_web_engine_page.cc \
                  Source/dooble_web_engine_view.cc

TRANSLATIONS    = Translations/dooble_en.ts \
                  Translations/dooble_ae.ts \
                  Translations/dooble_al_sq.ts \
                  Translations/dooble_af.ts \
                  Translations/dooble_al.ts \
                  Translations/dooble_am.ts \
                  Translations/dooble_as.ts \
                  Translations/dooble_az.ts \
		  Translations/dooble_ast.ts \
                  Translations/dooble_be.ts \
                  Translations/dooble_bd_bn.ts \
                  Translations/dooble_bg.ts \
                  Translations/dooble_ca.ts \
                  Translations/dooble_crh.ts \
                  Translations/dooble_cz.ts \
                  Translations/dooble_de.ts \
		  Translations/dooble_de_DE.ts \
                  Translations/dooble_dk.ts \
                  Translations/dooble_ee.ts \
                  Translations/dooble_es.ts \
                  Translations/dooble_eo.ts \
                  Translations/dooble_et.ts \
                  Translations/dooble_eu.ts \
                  Translations/dooble_fi.ts \
                  Translations/dooble_fr.ts \
                  Translations/dooble_galician.ts \
                  Translations/dooble_gl.ts \
                  Translations/dooble_gr.ts \
                  Translations/dooble_hb.ts \
                  Translations/dooble_hi.ts \
                  Translations/dooble_hr.ts \
                  Translations/dooble_hu.ts \
                  Translations/dooble_it.ts \
                  Translations/dooble_il.ts \
                  Translations/dooble_ie.ts \
                  Translations/dooble_id.ts \
                  Translations/dooble_jp.ts \
                  Translations/dooble_kk.ts \
                  Translations/dooble_kn.ts \
                  Translations/dooble_ko.ts \
		  Translations/dooble_ky.ts \
                  Translations/dooble_ku.ts \
                  Translations/dooble_lt.ts \
                  Translations/dooble_lk.ts \
                  Translations/dooble_lv.ts \
                  Translations/dooble_ml.ts \
                  Translations/dooble_mk.ts \
                  Translations/dooble_mn.ts \
                  Translations/dooble_ms.ts \
                  Translations/dooble_my-bn.ts \
                  Translations/dooble_mr.ts \
                  Translations/dooble_mt.ts \
                  Translations/dooble_nl.ts \
                  Translations/dooble_no.ts \
                  Translations/dooble_np.ts \
                  Translations/dooble_pl.ts \
                  Translations/dooble_pa.ts \
                  Translations/dooble_pt.ts \
                  Translations/dooble_pt-BR.ts \
                  Translations/dooble_ps.ts \
                  Translations/dooble_ro.ts \
                  Translations/dooble_ru.ts \
                  Translations/dooble_rw.ts \
                  Translations/dooble_se.ts \
                  Translations/dooble_sk.ts \
                  Translations/dooble_sl.ts \
                  Translations/dooble_sr.ts \
                  Translations/dooble_sq.ts \
                  Translations/dooble_sw.ts \
                  Translations/dooble_th.ts \
                  Translations/dooble_tr.ts \
                  Translations/dooble_vn.ts \
                  Translations/dooble_zh-CN-simple.ts \
                  Translations/dooble_zh-TW.ts \
                  Translations/dooble_zh-CN-traditional.ts \
                  Translations/dooble_Arab_BH_DZ_EG_IQ_JO_KW_LY_MA_OM_QA_SA_SY_YE.ts \
                  Translations/dooble_French_BE_BJ_BF_BI_FR_KM_CD_CI_DJ_DM_PF_TF_GA_GN_HT_LB_LU_ML_MR_YT_MC_NC_NE_NG_SN_TG_TN.ts \
                  Translations/dooble_Portuguese_AO_BR_CV_GW_MO_MZ_ST_TL.ts

UI_HEADERS_DIR  = Source

PROJECTNAME	= Dooble
TARGET		= Dooble

!win32 {
VERSION         = DOOBLE_VERSION
}

macx {
copydata.files		= Data/*.txt
copydata.path		= /Applications/Dooble.d/Data
copydocumentation.extra	= cp ./Documentation/*.pdf /Applications/Dooble.d/Documentation/. && cp ./Documentation/TO-DO /Applications/Dooble.d/Documentation/.
copydocumentation.path	= /Applications/Dooble.d/Documentation
copydooble.extra	= cp -r ./Dooble.app /Applications/Dooble.d/.
copydooble.path		= /Applications/Dooble.d
copyinfoplist.extra	= cp Data/Info.plist /Applications/Dooble.d/Dooble.app/Contents/.
copyinfoplist.path	= /Applications/Dooble.d
macdeployqt.extra	= $$[QT_INSTALL_BINS]/macdeployqt /Applications/Dooble.d/Dooble.app -executable=/Applications/Dooble.d/Dooble.app/Contents/MacOS/Dooble
macdeployqt.path	= Dooble.app
preinstall.extra	= rm -rf /Applications/Dooble.d/Dooble.app
preinstall.path		= /Applications/Dooble.d
translations.files	= Translations/*.qm
translations.path	= /Applications/Dooble.d/Translations

INSTALLS	= copydata \
                  copydocumentation \
                  preinstall \
                  copydooble \
                  macdeployqt \
		  copyinfoplist \
                  translations
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
