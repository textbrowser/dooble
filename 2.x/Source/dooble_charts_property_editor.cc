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

#include "dooble_charts_property_editor.h"

#include <limits>

static void find_recursive_items(QStandardItem *item,
				 QList<QStandardItem *> &list)
{
  if(!item)
    return;

  list << item;

  for(int i = 0; i < item->rowCount(); i++)
    for(int j = 0; j < item->columnCount(); j++)
      find_recursive_items(item->child(i, j), list);
}

QSize dooble_charts_property_editor_model_delegate::
sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  auto size(QStyledItemDelegate::sizeHint(option, index));

  return size;
}

QWidget *dooble_charts_property_editor_model_delegate::
createEditor(QWidget *parent,
	     const QStyleOptionViewItem &option,
	     const QModelIndex &index) const
{
  dooble_charts::Properties property = dooble_charts::Properties
    (index.data(Qt::ItemDataRole(Qt::UserRole + 1)).toInt());

  switch(property)
    {
    case dooble_charts::ANIMATION_DURATION:
      {
	auto spin_box = new QSpinBox(parent);

	spin_box->setRange(0, std::numeric_limits<int>::max());
	spin_box->setValue(index.data().toInt());
	return spin_box;
      }
    case dooble_charts::DATA_SOURCE_TYPE:
      {
	auto combo_box = new QComboBox(parent);
	int i = -1;

	combo_box->addItem(tr("File"));
	combo_box->addItem(tr("IP Address"));
	i = combo_box->findText(index.data().toString());

	if(i == -1)
	  i = 0;

	combo_box->setCurrentIndex(i);
	return combo_box;
      }
    default:
      {
	break;
      }
    }

  return QStyledItemDelegate::createEditor(parent, option, index);
}

void dooble_charts_property_editor_model_delegate::
setEditorData(QWidget *editor, const QModelIndex &index) const
{
  QStyledItemDelegate::setEditorData(editor, index);
}

void dooble_charts_property_editor_model_delegate::setModelData
(QWidget *editor, QAbstractItemModel *m, const QModelIndex &index) const
{
  QStyledItemDelegate::setModelData(editor, m, index);
}

void dooble_charts_property_editor_model_delegate::
slot_current_index_changed(int index)
{
  Q_UNUSED(index);
}

void dooble_charts_property_editor_model_delegate::slot_emit_signal(void)
{
  auto widget = qobject_cast<QWidget *> (sender());

  if(!widget)
    return;
}

void dooble_charts_property_editor_model_delegate::slot_text_changed(void)
{
  auto textEdit = qobject_cast<QPlainTextEdit *> (sender());

  if(!textEdit)
    return;
}

void dooble_charts_property_editor_model_delegate::updateEditorGeometry
(QWidget *editor,
 const QStyleOptionViewItem &option,
 const QModelIndex &index) const
{
  if(editor)
    {
      auto rect(option.rect);

      rect.setHeight(sizeHint(option, index).height());
      editor->setGeometry(rect);
    }
  else
    QStyledItemDelegate::updateEditorGeometry(editor, option, index);
}

dooble_charts_property_editor_model::
dooble_charts_property_editor_model(QObject *parent):
  QStandardItemModel(parent)
{
  setHorizontalHeaderLabels(QStringList() << "Property" << "Value");

  auto generic = new QStandardItem("Generic");

  generic->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

  for(int i = 0; !dooble_charts::s_generic_properties_strings[i].isEmpty(); i++)
    {
      QList<QStandardItem *> list;
      auto item = new QStandardItem
	(dooble_charts::s_generic_properties_strings[i]);

      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      list << item;
      item = new QStandardItem();
      item->setData(dooble_charts::Properties(i));
      item->setFlags
	(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

      switch(dooble_charts::Properties(i))
	{
	case dooble_charts::BACKGROUND_VISIBLE:
	case dooble_charts::DROP_SHADOW_ENABLED:
	case dooble_charts::LEGEND_VISIBLE:
	case dooble_charts::LOCALIZE_NUMBERS:
	case dooble_charts::PLOT_AREA_BACKGROUND_VISIBLE:
	  {
	    item->setFlags(Qt::ItemIsUserCheckable | item->flags());
	    break;
	  }
	default:
	  {
	    break;
	  }
	}

      list << item;
      generic->appendRow(list);
    }

  appendRow(generic);
  connect(this,
	  SIGNAL(itemChanged(QStandardItem *)),
	  this,
	  SLOT(slot_item_changed(QStandardItem *)));
}

dooble_charts_property_editor_model::~dooble_charts_property_editor_model()
{
}

QList<QStandardItem *> dooble_charts_property_editor_model::
find_all_child_items(const QString &text) const
{
  auto list(findItems(text));

  if(list.isEmpty())
    return list;

  QList<QStandardItem *> all;

  for(int i = 0; i < list.size(); i++)
    {
      auto item = list.at(i);

      if(!item)
	continue;

      all << item;

      for(int j = 0; j < item->rowCount(); j++)
	for(int k = 0; k < item->columnCount(); k++)
	  find_recursive_items(item->child(j, k), all);
    }

  return all;
}

QStandardItem *dooble_charts_property_editor_model::
find_specific_item(const QString &text) const
{
  auto list(findItems(text, Qt::MatchExactly | Qt::MatchRecursive));

  if(!list.isEmpty())
    return list.at(0);

  return nullptr;
}

QStandardItem *dooble_charts_property_editor_model::item_from_property
(const dooble_charts::Properties property, const int column) const
{
  auto list(findItems("Generic"));

  if(list.isEmpty() || !list.at(0))
    return nullptr;

  for(int i = 0; i < list.at(0)->rowCount(); i++)
    {
      auto item = list.at(0)->child(i, column);

      if(!item)
	continue;

      dooble_charts::Properties p = dooble_charts::Properties
	(item->data(Qt::ItemDataRole(Qt::UserRole + 1)).toInt());

      if(p == property)
	return item;
    }

  return nullptr;
}

void dooble_charts_property_editor_model::slot_item_changed(QStandardItem *item)
{
  if(!item)
    return;
}

dooble_charts_property_editor::
dooble_charts_property_editor(QTreeView *tree):QWidget(tree)
{
  m_model = nullptr;
  m_tree = tree;
}

dooble_charts_property_editor::~dooble_charts_property_editor()
{
}

void dooble_charts_property_editor::prepare_generic(dooble_charts *chart)
{
  if(!chart || !m_model || !m_tree)
    return;

  QHashIterator<dooble_charts::Properties, QVariant> it(chart->properties());

  while(it.hasNext())
    {
      it.next();

      QStandardItem *item = m_model->item_from_property(it.key(), 1);

      if(item)
	switch(it.key())
	  {
	  case dooble_charts::BACKGROUND_VISIBLE:
	  case dooble_charts::DROP_SHADOW_ENABLED:
	  case dooble_charts::LEGEND_VISIBLE:
	  case dooble_charts::LOCALIZE_NUMBERS:
	  case dooble_charts::PLOT_AREA_BACKGROUND_VISIBLE:
	    {
	      item->setCheckState
		(it.value().toBool() ? Qt::Checked : Qt::Unchecked);
	      break;
	    }
	  default:
	    {
	      item->setText(it.value().toString());
	      break;
	    }
	  }
    }

  auto item_delegate = m_tree->itemDelegate();

  if(item_delegate)
    item_delegate->deleteLater();

  item_delegate = new dooble_charts_property_editor_model_delegate(this);
  connect
    (item_delegate,
     SIGNAL(emit_signal(const dooble_charts::Properties)),
     this,
     SLOT(slot_delegate_signal(const dooble_charts::Properties)));
  m_tree->setItemDelegate(item_delegate);
  m_tree->setModel(m_model);
  m_tree->setFirstColumnSpanned(0, m_tree->rootIndex(), true);
  m_tree->expandAll();
  m_tree->resizeColumnToContents(0);
  m_tree->resizeColumnToContents(1);
}

void dooble_charts_property_editor::slot_delegate_signal
(const dooble_charts::Properties property)
{
  Q_UNUSED(property);
}
