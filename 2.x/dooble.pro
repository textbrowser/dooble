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

TRANSLATIONS    = Translations\\dooble_en.ts \
                  Translations\\dooble_ae.ts \
                  Translations\\dooble_al_sq.ts \
                  Translations\\dooble_af.ts \
                  Translations\\dooble_al.ts \
                  Translations\\dooble_am.ts \
                  Translations\\dooble_as.ts \
                  Translations\\dooble_az.ts \
		  Translations\\dooble_ast.ts \
                  Translations\\dooble_be.ts \
                  Translations\\dooble_bd_bn.ts \
                  Translations\\dooble_bg.ts \
                  Translations\\dooble_ca.ts \
                  Translations\\dooble_crh.ts \
                  Translations\\dooble_cz.ts \
                  Translations\\dooble_de.ts \
                  Translations\\dooble_dk.ts \
                  Translations\\dooble_ee.ts \
                  Translations\\dooble_es.ts \
                  Translations\\dooble_eo.ts \
                  Translations\\dooble_et.ts \
                  Translations\\dooble_eu.ts \
                  Translations\\dooble_fi.ts \
                  Translations\\dooble_fr.ts \
                  Translations\\dooble_galician.ts \
                  Translations\\dooble_gl.ts \
                  Translations\\dooble_gr.ts \
                  Translations\\dooble_hb.ts \
                  Translations\\dooble_hi.ts \
                  Translations\\dooble_hr.ts \
                  Translations\\dooble_hu.ts \
                  Translations\\dooble_it.ts \
                  Translations\\dooble_il.ts \
                  Translations\\dooble_ie.ts \
                  Translations\\dooble_id.ts \
                  Translations\\dooble_jp.ts \
                  Translations\\dooble_kk.ts \
                  Translations\\dooble_kn.ts \
                  Translations\\dooble_ko.ts \
		  Translations\\dooble_ky.ts \
                  Translations\\dooble_ku.ts \
                  Translations\\dooble_lt.ts \
                  Translations\\dooble_lk.ts \
                  Translations\\dooble_lv.ts \
                  Translations\\dooble_ml.ts \
                  Translations\\dooble_mk.ts \
                  Translations\\dooble_mn.ts \
                  Translations\\dooble_ms.ts \
                  Translations\\dooble_my-bn.ts \
                  Translations\\dooble_mr.ts \
                  Translations\\dooble_mt.ts \
                  Translations\\dooble_nl.ts \
                  Translations\\dooble_no.ts \
                  Translations\\dooble_np.ts \
                  Translations\\dooble_pl.ts \
                  Translations\\dooble_pa.ts \
                  Translations\\dooble_pt.ts \
                  Translations\\dooble_pt-BR.ts \
                  Translations\\dooble_ps.ts \
                  Translations\\dooble_ro.ts \
                  Translations\\dooble_ru.ts \
                  Translations\\dooble_rw.ts \
                  Translations\\dooble_se.ts \
                  Translations\\dooble_sk.ts \
                  Translations\\dooble_sl.ts \
                  Translations\\dooble_sr.ts \
                  Translations\\dooble_sq.ts \
                  Translations\\dooble_sw.ts \
                  Translations\\dooble_th.ts \
                  Translations\\dooble_tr.ts \
                  Translations\\dooble_vn.ts \
                  Translations\\dooble_zh-CN-simple.ts \
                  Translations\\dooble_zh-TW.ts \
                  Translations\\dooble_zh-CN-traditional.ts \
                  Translations\\dooble_Arab_BH_DZ_EG_IQ_JO_KW_LY_MA_OM_QA_SA_SY_YE.ts \
                  Translations\\dooble_French_BE_BJ_BF_BI_FR_KM_CD_CI_DJ_DM_PF_TF_GA_GN_HT_LB_LU_ML_MR_YT_MC_NC_NE_NG_SN_TG_TN.ts \
                  Translations\\dooble_Portuguese_AO_BR_CV_GW_MO_MZ_ST_TL.ts

UI_HEADERS_DIR  = Source

PROJECTNAME	= Dooble
TARGET		= Dooble
