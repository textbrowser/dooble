cache()

libspoton.commands = $(MAKE) -C libSpotOn library
libspoton.depends =
libspoton.target = libspoton.dll

purge.commands = del /F Documentation\\*~ && del /F Include\\*~ && \
		 del /F Installers\\*~ && del /F Source\\*~ && \
                 del /F *~

CONFIG		+= qt release warn_on
LANGUAGE	= C++
QT		+= concurrent network printsupport sql \
                   webenginewidgets widgets xml
TEMPLATE	= app

# The function gcry_kdf_derive() is available in version
# 1.5.0 of the gcrypt library.

DEFINES         += DOOBLE_LINKED_WITH_LIBSPOTON \
                   DOOBLE_MINIMUM_GCRYPT_VERSION=0x010601 \
		   DOOBLE_USE_PTHREADS

# Unfortunately, the clean target assumes too much knowledge
# about the internals of libspoton.

QMAKE_CLEAN     += Dooble libSpotOn\\*.dll libSpotOn\\*.o libSpotOn\\test.exe
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -fwrapv -mtune=generic -pie -std=c++11 -Os \
			  -Wall -Wcast-align -Wcast-qual \
			  -Wextra \
			  -Woverloaded-virtual -Wpointer-arith \
			  -Wstrict-overflow=5
QMAKE_DISTCLEAN	+= .qmake.cache .qmake.stash
QMAKE_EXTRA_TARGETS = libspoton purge

INCLUDEPATH	+= . Include Include.win32 libSpoton\\Include.win32
LIBS		+= -LLibraries.win32 -LlibSpotOn \
                   -LlibSpotOn\\Libraries.win32 \
		   -lgcrypt-20 -lgpg-error-0 -lpthread -lspoton
PRE_TARGETDEPS = libspoton.dll

RC_FILE		= Resources\\dooble.rc

FORMS           = UI\\dapplicationPropertiesWindow.ui \
		  UI\\dbookmarksPopup.ui \
		  UI\\dbookmarksWindow.ui \
		  UI\\dclearContainersWindow.ui \
		  UI\\dcookieWindow.ui \
		  UI\\ddownloadPrompt.ui \
		  UI\\ddownloadWindowItem.ui \
                  UI\\ddownloadWindow.ui \
		  UI\\derrorLog.ui \
                  UI\\dexceptionsWindow.ui \
		  UI\\dfileManagerForm.ui \
		  UI\\dftpManagerForm.ui \
		  UI\\dhistorySideBar.ui \
		  UI\\dhistoryWindow.ui \
		  UI\\dmainWindow.ui \
                  UI\\dpageSourceWindow.ui \
                  UI\\dpassphrasePrompt.ui \
		  UI\\dpasswordPrompt.ui \
		  UI\\dreinstateWidget.ui \
		  UI\\dsettings.ui \
		  UI\\dsslciphers.ui \
		  UI\\dstatusBar.ui

UI_HEADERS_DIR  = Include

HEADERS		= Include\\dbookmarkspopup.h \
		  Include\\dbookmarkstree.h \
		  Include\\dbookmarkswindow.h \
		  Include\\dclearcontainers.h \
		  Include\\dcookies.h \
		  Include\\dcookiewindow.h \
		  Include\\ddesktopwidget.h \
		  Include\\ddownloadprompt.h \
		  Include\\ddownloadwindow.h \
                  Include\\ddownloadwindowitem.h \
		  Include\\derrorlog.h \
		  Include\\dexceptionsmodel.h \
                  Include\\dexceptionswindow.h \
		  Include\\dfilemanager.h \
		  Include\\dfilesystemmodel.h \
		  Include\\dftp.h \
		  Include\\dgenericsearchwidget.h \
		  Include\\dhistory.h \
		  Include\\dhistorymodel.h \
		  Include\\dhistorysidebar.h \
		  Include\\dmisc.h \
                  Include\\dnetworkaccessmanager.h \
                  Include\\dnetworkcache.h \
		  Include\\dooble.h \
                  Include\\dpagesourcewindow.h \
		  Include\\dprintfromcommandprompt.h \
		  Include\\dreinstatedooble.h \
		  Include\\dsearchwidget.h \
                  Include\\dsettings.h \
                  Include\\dsettingshomelinewidget.h \
		  Include\\dspoton.h \
		  Include\\dsslcipherswindow.h \
		  Include\\dtabwidget.h \
		  Include\\durlwidget.h \
		  Include\\dview.h \
                  Include\\dwebpage.h \
                  Include\\dwebview.h

SOURCES		= Source\\dbookmarkspopup.cc \
		  Source\\dbookmarkstree.cc \
		  Source\\dbookmarkswindow.cc \
		  Source\\dclearcontainers.cc \
		  Source\\dcookies.cc \
		  Source\\dcookiewindow.cc \
		  Source\\dcrypt.cc \
		  Source\\ddesktopwidget.cc \
		  Source\\ddownloadprompt.cc \
		  Source\\ddownloadwindow.cc \
                  Source\\ddownloadwindowitem.cc \
		  Source\\derrorlog.cc \
		  Source\\dexceptionsmodel.cc \
                  Source\\dexceptionswindow.cc \
		  Source\\dfilemanager.cc \
		  Source\\dfilesystemmodel.cc \
		  Source\\dftp.cc \
                  Source\\dgenericsearchwidget.cc \
		  Source\\dhistory.cc \
		  Source\\dhistorymodel.cc \
		  Source\\dhistorysidebar.cc \
		  Source\\dmisc.cc \
                  Source\\dnetworkaccessmanager.cc \
                  Source\\dnetworkcache.cc \
		  Source\\dooble.cc \
                  Source\\dpagesourcewindow.cc \
		  Source\\dreinstatedooble.cc \
		  Source\\dsearchwidget.cc \
                  Source\\dsettings.cc \
                  Source\\dsettingshomelinewidget.cc \
		  Source\\dspoton.cc \
		  Source\\dsslcipherswindow.cc \
		  Source\\dtabwidget.cc \
		  Source\\durlwidget.cc \
		  Source\\dview.cc \
                  Source\\dwebpage.cc \
		  Source\\dwebview.cc

RESOURCES	+= CSS\\css.qrc \
		   Tab\\Default\\htmls.qrc

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

PROJECTNAME	= Dooble
TARGET		= Dooble
