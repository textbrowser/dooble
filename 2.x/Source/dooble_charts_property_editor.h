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

#ifndef dooble_charts_property_editor_h
#define dooble_charts_property_editor_h

#include <QPointer>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

#include "dooble_charts.h"

class dooble_charts_property_editor_model_delegate: public QStyledItemDelegate
{
  Q_OBJECT

 public:
  dooble_charts_property_editor_model_delegate(QWidget *parent):
    QStyledItemDelegate(parent)
  {
  }

  ~dooble_charts_property_editor_model_delegate()
  {
  }

  QSize sizeHint(const QStyleOptionViewItem &option,
		 const QModelIndex &index) const;
  QWidget *createEditor(QWidget *parent,
			const QStyleOptionViewItem &option,
			const QModelIndex &index) const;
  void setEditorData(QWidget *editor, const QModelIndex &index) const;
  void setModelData(QWidget *editor,
		    QAbstractItemModel *model,
		    const QModelIndex &index) const;
  void updateEditorGeometry(QWidget *editor,
			    const QStyleOptionViewItem &option,
			    const QModelIndex &index) const;

 private slots:
  void slot_show_color_dialog(void);
  void slot_show_file_dialog(void);
  void slot_show_font_dialog(void);
  void slot_text_changed(void);

 signals:
  void show_color_dialog(const dooble_charts::Properties property);
  void show_file_dialog
    (QPushButton *push_button, const dooble_charts::Properties property);
  void show_font_dialog(const dooble_charts::Properties property);
};

class dooble_charts_property_editor_model: public QStandardItemModel
{
  Q_OBJECT

 public:
  dooble_charts_property_editor_model(QObject *parent);
  virtual ~dooble_charts_property_editor_model();
  QStandardItem *item_from_property
    (const dooble_charts::Properties property, const int column) const;

 protected:
  QList<QStandardItem *> find_all_child_items(const QString &text) const;
  QStandardItem *find_specific_item(const QString &text) const;
};

class dooble_charts_property_editor: public QWidget
{
  Q_OBJECT

 public:
  dooble_charts_property_editor(QTreeView *tree);
  virtual ~dooble_charts_property_editor();
  QPointer<dooble_charts_property_editor_model> model(void) const;
  QVariant property(const dooble_charts::Properties property);

 private slots:
  void slot_show_color_dialog(const dooble_charts::Properties property);
  void slot_show_file_dialog
    (QPushButton *push_button, const dooble_charts::Properties property);
  void slot_show_font_dialog(const dooble_charts::Properties property);

 protected:
  QPointer<QTreeView> m_tree;
  QPointer<dooble_charts_property_editor_model> m_model;
  void prepare_generic(dooble_charts *chart);
};

#endif
