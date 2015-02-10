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

#include "ui_settings.h"

class dooble;
class QKeyEvent;

class dsettings: public QMainWindow
{
  Q_OBJECT

 public:
  dsettings(void);
  ~dsettings();
  void exec(dooble *parent);
  dooble *parentDooble(void) const;
  Ui_settingsWindow UI(void) const;

 private:
  QTimer m_updateLabelTimer;
  QTimer m_purgeMemoryCachesTimer;
  QString m_previousIconSet;
  QPointer<dooble> m_parentDooble;
  Ui_settingsWindow ui;
  bool event(QEvent *event);
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void updateFontWidgets(const QString &fontName, QComboBox *fontSizeWidget);

 public slots:
  void slotPopulateApplications(const QMap<QString, QString> &suffixes);
  void slotPassphraseWasAuthenticated(const bool state);

 private slots:
  void slotClicked(QAbstractButton *button);
  void slotSetIcons(void);
  void slotChangePage(bool checked);
  void slotDeleteSuffix(void);
  void slotIconsPreview(void);
  void slotUpdateLabels(void);
  void slotClearFavicons(void);
  void slotClearDiskCache(void);
  void slotWebFontChanged(const QString &text);
  void slotGroupBoxClicked(bool checked);
  void slotEnablePassphrase(void);
  void slotPurgeMemoryCaches(void);
  void slotSelectIconCfgFile(void);
  void slotUpdateApplication(const QString &suffix, const QString &action);
  void slotCookieTimerTimeChanged(int index);
  void slotCustomContextMenuRequested(const QPoint &point);
  void slotApplicationPulldownActivated(int index);
#ifdef DOOBLE_LINKED_WITH_LIBSPOTON
  void slotChooseSpotOnSharedDatabaseFile(void);
#endif
  void slotChooseMyRetrievedFilesDirectory(void);

 signals:
  void showTabBar(const bool state);
  void iconsChanged(void);
  void settingsReset(void);
  void showIpAddress(const bool state);
  void cookieTimerChanged(void);
  void reencodeRestorationFile(void);
  void textSizeMultiplierChanged(const qreal multiplier);
};

#endif
