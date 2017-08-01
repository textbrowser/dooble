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

#ifndef dooble_settings_h
#define dooble_settings_h

#include <QAtomicInteger>
#include <QFuture>
#include <QFutureWatcher>
#include <QMainWindow>
#include <QPointer>
#include <QProgressDialog>
#include <QReadWriteLock>

#include "ui_dooble_settings.h"

class dooble_settings: public QMainWindow
{
  Q_OBJECT

 public:
  enum Panels
  {
    CACHE_PANEL = 0,
    DISPLAY_PANEL,
    HISTORY_PANEL,
    PRIVACY_PANEL,
    WEB_PANEL,
    WINDOWS_PANEL
  };

  dooble_settings(void);
  static QVariant setting(const QString &key);
  static bool has_dooble_credentials(void);
  static void set_setting(const QString &key, const QVariant &value);
  void show_panel(dooble_settings::Panels panel);

 protected:
  void closeEvent(QCloseEvent *event);

 private:
  QFuture<QList<QByteArray> > m_pbkdf2_future;
  QFutureWatcher<QList<QByteArray> > m_pbkdf2_future_watcher;
  QPointer<QProgressDialog> m_pbkdf2_dialog;
  Ui_dooble_settings m_ui;
  static QAtomicInteger<quint64> s_db_id;
  static QMap<QString, QVariant> s_settings;
  static QReadWriteLock s_settings_mutex;
  void restore(void);

 public slots:
  void show(void);
  void showNormal(void);

 private slots:
  void slot_apply(void);
  void slot_clear_cache(void);
  void slot_page_button_clicked(void);
  void slot_pbkdf2_future_finished(void);
  void slot_save_credentials(void);

 signals:
  void credentials_created(void);
};

#endif
