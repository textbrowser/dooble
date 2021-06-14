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

#ifndef dooble_charts_xyseries_h
#define dooble_charts_xyseries_h

#include "dooble_charts.h"

class dooble_charts_xyseries: public dooble_charts
{
  Q_OBJECT

 public:
  dooble_charts_xyseries(QWidget *parent);
  ~dooble_charts_xyseries();
  QHash<dooble_charts::Properties, QVariant> properties(void) const;
#ifdef DOOBLE_QTCHARTS_PRESENT
  static QString tick_type_to_string(const QValueAxis::TickType tick_type);
  static QValueAxis::TickType string_to_tick_type(const QString &t);
#endif
  static const QString s_chart_properties_strings[];
  void open(const QString &name);
  void save(QString &error);

 private:
  QHash<QString, QVariant> properties_for_database(void) const;
  QHash<QString, QVariant> x_axis_properties_for_database(void) const;
  QHash<QString, QVariant> y_axis_properties_for_database(void) const;
  QString property_to_name(const dooble_charts::Properties property) const;

 private slots:
  void slot_clear(void);
  void slot_data_ready(const QVector<qreal> &vector);
  void slot_item_changed(QStandardItem *item);
};

#endif
