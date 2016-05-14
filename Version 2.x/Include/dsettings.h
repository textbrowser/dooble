/*
** Copyright (c) 2008 - present, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from Dooble without specific prior written permission.
**
** DOOBLE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** DOOBLE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _dsettings_h_
#define _dsettings_h_

#include <QMainWindow>
#include <QPointer>
#include <QTimer>

#include "ui_dsettings.h"

class QKeyEvent;
class dooble;

class dsettings: public QMainWindow
{
  Q_OBJECT

 public:
  dsettings(void);
  ~dsettings();
  Ui_settingsWindow UI(void) const;
  dooble *parentDooble(void) const;
  void exec(dooble *parent);

 private:
  QPointer<dooble> m_parentDooble;
  QString m_previousIconSet;
  QTimer m_purgeMemoryCachesTimer;
  QTimer m_updateLabelTimer;
  Ui_settingsWindow ui;
  bool event(QEvent *event);
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void updateFontWidgets(const QString &fontName, QComboBox *fontSizeWidget);

 public slots:
  void slotPassphraseWasAuthenticated(const bool state);
  void slotPopulateApplications(const QMap<QString, QString> &suffixes);

 private slots:
  void slotApplicationPulldownActivated(int index);
  void slotChangePage(bool checked);
  void slotChooseMyRetrievedFilesDirectory(void);
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  void slotChooseSpotOnSharedDatabaseFile(void);
#endif
  void slotClearDiskCache(void);
  void slotClearFavicons(void);
  void slotClicked(QAbstractButton *button);
  void slotCookieTimerTimeChanged(int index);
  void slotCustomContextMenuRequested(const QPoint &point);
  void slotDeleteAllSuffixes(void);
  void slotDeleteSuffix(void);
  void slotEnablePassphrase(void);
  void slotGroupBoxClicked(bool checked);
  void slotIconsPreview(void);
  void slotPurgeMemoryCaches(void);
  void slotResetUrlAgentString(void);
  void slotSelectIconCfgFile(void);
  void slotSetIcons(void);
  void slotUpdateApplication(const QString &suffix, const QString &action);
  void slotUpdateLabels(void);
  void slotWebFontChanged(const QString &text);

 signals:
  void cookieTimerChanged(void);
  void iconsChanged(void);
  void reencodeRestorationFile(void);
  void settingsChanged(void);
  void settingsReset(void);
  void showIpAddress(const bool state);
  void showTabBar(const bool state);
  void textSizeMultiplierChanged(const qreal multiplier);
};

#endif
