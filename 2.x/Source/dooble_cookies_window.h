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

#ifndef dooble_cookies_window_h
#define dooble_cookies_window_h

#include <QMainWindow>
#include <QNetworkCookie>
#include <QPointer>
#include <QTimer>

#include "ui_dooble_cookies_window.h"

class QWebEngineCookieStore;
class dooble_cookies;

class dooble_cookies_window: public QMainWindow
{
  Q_OBJECT

 public:
  dooble_cookies_window(bool is_private, QWidget *parent);
  void filter(const QString &text);
  void populate(void);
  void setCookieStore(QWebEngineCookieStore *cookieStore);
  void setCookies(dooble_cookies *cookies);

 protected:
  void closeEvent(QCloseEvent *event);

 private:
  QHash<QString, QHash<QByteArray, QTreeWidgetItem *> > m_child_items;
  QHash<QString, QTreeWidgetItem *> m_top_level_items;
  QPointer<QWebEngineCookieStore> m_cookieStore;
  QPointer<dooble_cookies> m_cookies;
  QTimer m_domain_filter_timer;
  QTimer m_purge_domains_timer;
  Ui_dooble_cookies_window m_ui;
  bool m_is_private;

 public slots:
  void show(void);
  void showNormal(void);

 private slots:
  void slot_cookie_added(const QNetworkCookie &cookie, bool is_favorite);
  void slot_cookie_removed(const QNetworkCookie &cookie);
  void slot_delete_selected(void);
  void slot_delete_shown(void);
  void slot_domain_filter_timer_timeout(void);
  void slot_item_changed(QTreeWidgetItem *item, int column);
  void slot_item_selection_changed(void);
  void slot_periodically_purge_temporary_domains(bool state);
  void slot_purge_domains_timer_timeout(void);

 signals:
  void delete_cookie(const QNetworkCookie &cookie);
};

#endif
