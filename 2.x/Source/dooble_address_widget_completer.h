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

#ifndef dooble_address_widget_completer_h
#define dooble_address_widget_completer_h

#include <QCompleter>
#include <QTimer>

class QStandardItemModel;
class dooble_address_widget_completer_popup;

class dooble_address_widget_completer: public QCompleter
{
  Q_OBJECT

 public:
  dooble_address_widget_completer(QWidget *parent);
  ~dooble_address_widget_completer();
  static void add_item(const QIcon &icon, const QUrl &url);
  static void remove_item(const QUrl &url);
  void complete(void);
  void set_item_icon(const QIcon &icon, const QUrl &url);

 private:
  QStandardItemModel *m_model;
  QTimer m_text_edited_timer;
  dooble_address_widget_completer_popup *m_popup;
  static QHash<QUrl, char> s_urls;
  static QStandardItemModel *s_model;
  int levenshtein_distance(const QString &str1, const QString &str2) const;
  void complete(const QString &text);

 private slots:
  void slot_clicked(const QModelIndex &index);
  void slot_history_cleared(void);
  void slot_text_edited_timeout(void);
};

#endif
