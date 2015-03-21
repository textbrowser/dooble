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

#ifndef _dexceptionsmodel_h_
#define _dexceptionsmodel_h_

#include <QStandardItemModel>

class QProgressBar;

class dexceptionsmodel: public QStandardItemModel
{
  Q_OBJECT

 public:
  dexceptionsmodel(const QString &tableName);
  ~dexceptionsmodel();
  bool allow(const QString &host);
  bool allowed(const QString &host) const;
  bool contains(const QString &host) const;
  void add(const QString &host,
	   const QUrl &url,
	   const QDateTime &dateTime);
  void populate(void);
  void reencode(QProgressBar *progress);
  void deleteList(const QModelIndexList &list);
  QStringList allowedHosts(void) const;

 private:
  QString m_tableName;
  void purge(void);
  void createExceptionsDatabase(void);

 private slots:
  void slotCheckBoxItemChanged(QStandardItem *item);
};

#endif
