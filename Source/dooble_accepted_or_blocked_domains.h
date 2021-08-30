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

#ifndef dooble_accepted_or_blocked_domains_h
#define dooble_accepted_or_blocked_domains_h

#include <QFuture>
#include <QMessageBox>
#include <QPointer>
#include <QSqlDatabase>
#include <QTableWidgetItem>
#include <QTimer>

#include "dooble_main_window.h"
#include "ui_dooble_accepted_or_blocked_domains.h"

class dooble_accepted_or_blocked_domains: public dooble_main_window
{
  Q_OBJECT

 public:
  dooble_accepted_or_blocked_domains(void);
  ~dooble_accepted_or_blocked_domains();
  bool contains(const QString &domain) const;
  bool exception(const QUrl &url) const;
  void abort(void);
  void accept_or_block_domain(const QString &domain, bool replace = true);
  void new_exception(const QString &url);
  void purge(void);
  void show_normal(QWidget *parent);

 public slots:
  void show(void);

 protected:
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void resizeEvent(QResizeEvent *event);

 private:
  QFuture<void> m_future;
  QHash<QString, char> m_domains;
  QHash<QString, char> m_exceptions;
  QHash<QString, char> m_session_origin_hosts;
  QPointer<QMessageBox> m_import_dialog;
  QTimer m_search_timer;
  Ui_dooble_accepted_or_blocked_domains m_ui;
  void create_tables(QSqlDatabase &db);
  void populate(void);
  void populate_exceptions(void);
  void save(const QByteArray &authentication_key,
	    const QByteArray &encryption_key,
	    const QHash<QString, char> &hash);
  void save_blocked_domain(const QString &domain, bool replace, bool state);
  void save_exception(const QString &url, bool state);
  void save_settings(void);

 private slots:
  void slot_add(void);
  void slot_add_session_url(const QUrl &first_party_url,
			    const QUrl &origin_url);
  void slot_delete_all_exceptions(void);
  void slot_delete_selected(void);
  void slot_delete_selected_exceptions(void);
  void slot_exceptions_item_changed(QTableWidgetItem *item);
  void slot_find(void);
  void slot_import(void);
  void slot_imported(void);
  void slot_item_changed(QTableWidgetItem *item);
  void slot_maximum_entries_changed(int value);
  void slot_new_exception(const QString &url);
  void slot_new_exception(void);
  void slot_populate(void);
  void slot_radio_button_toggled(bool state);
  void slot_save(void);
  void slot_save_selected(void);
  void slot_search_timer_timeout(void);
  void slot_splitter_moved(int pos, int index);

 signals:
  void add_session_url(const QUrl &first_party_url, const QUrl &origin_url);
  void imported(void);
  void populated(void);
};

class dooble_accepted_or_blocked_domains_item: public QTableWidgetItem
{
 public:
  dooble_accepted_or_blocked_domains_item(const QString &text):
    QTableWidgetItem(text)
  {
  }

  dooble_accepted_or_blocked_domains_item(void):QTableWidgetItem()
  {
  }

  bool operator < (const QTableWidgetItem &other) const
  {
    if(Qt::ItemIsUserCheckable & flags())
      return checkState() < other.checkState();
    else
      return other.text() > text();
  }
};

#endif
