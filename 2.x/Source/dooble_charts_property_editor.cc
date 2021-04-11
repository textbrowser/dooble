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

#include <QColorDialog>
#include <QComboBox>
#include <QFontDialog>
#include <QPlainTextEdit>
#include <QSpinBox>

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
  auto property = dooble_charts::Properties
    (index.data(Qt::ItemDataRole(Qt::UserRole + 1)).toInt());
  auto size(QStyledItemDelegate::sizeHint(option, index));

  switch(property)
    {
    case dooble_charts::Properties::DATA_EXTRACTION_SCRIPT:
      {
	size.setHeight(250);
	break;
      }
    default:
      {
	break;
      }
    }

  return size;
}

QWidget *dooble_charts_property_editor_model_delegate::
createEditor(QWidget *parent,
	     const QStyleOptionViewItem &option,
	     const QModelIndex &index) const
{
  auto property = dooble_charts::Properties
    (index.data(Qt::ItemDataRole(Qt::UserRole + 1)).toInt());

  switch(property)
    {
    case dooble_charts::Properties::CHART_ANIMATION_DURATION:
    case dooble_charts::Properties::CHART_MARGINS_BOTTOM:
    case dooble_charts::Properties::CHART_MARGINS_LEFT:
    case dooble_charts::Properties::CHART_MARGINS_RIGHT:
    case dooble_charts::Properties::CHART_MARGINS_TOP:
      {
	auto editor = new QSpinBox(parent);

	editor->setRange(0, std::numeric_limits<int>::max());
	editor->setValue(index.data().toInt());
	return editor;
      }
    case dooble_charts::Properties::CHART_ANIMATION_OPTIONS:
      {
	auto editor = new QComboBox(parent);
	int i = -1;

	editor->addItem(tr("All"));
	editor->addItem(tr("Grid Axis"));
	editor->addItem(tr("None"));
	editor->addItem(tr("Series"));
	i = editor->findText(index.data().toString());

	if(i == -1)
	  i = 2; // None

	editor->setCurrentIndex(i);
	return editor;
      }
    case dooble_charts::Properties::CHART_AXIS_X_COLOR:
    case dooble_charts::Properties::CHART_AXIS_X_GRID_LINE_COLOR:
    case dooble_charts::Properties::CHART_AXIS_X_LABELS_COLOR:
    case dooble_charts::Properties::CHART_AXIS_X_MINOR_GRID_LINE_COLOR:
    case dooble_charts::Properties::CHART_AXIS_X_SHADES_BORDER_COLOR:
    case dooble_charts::Properties::CHART_AXIS_X_SHADES_COLOR:
    case dooble_charts::Properties::CHART_AXIS_X_TITLE_COLOR:
    case dooble_charts::Properties::CHART_AXIS_Y_COLOR:
    case dooble_charts::Properties::CHART_AXIS_Y_GRID_LINE_COLOR:
    case dooble_charts::Properties::CHART_AXIS_Y_LABELS_COLOR:
    case dooble_charts::Properties::CHART_AXIS_Y_MINOR_GRID_LINE_COLOR:
    case dooble_charts::Properties::CHART_AXIS_Y_SHADES_BORDER_COLOR:
    case dooble_charts::Properties::CHART_AXIS_Y_SHADES_COLOR:
    case dooble_charts::Properties::CHART_AXIS_Y_TITLE_COLOR:
    case dooble_charts::Properties::CHART_BACKGROUND_COLOR:
    case dooble_charts::Properties::CHART_TITLE_COLOR:
      {
	auto editor = new QPushButton(parent);

	connect(editor,
		SIGNAL(clicked(void)),
		this,
		SLOT(slot_show_color_dialog(void)));
	editor->setProperty("property", property);
	editor->setStyleSheet
	  (QString("QPushButton {background-color: %1;}").
	   arg(index.data().toString()));
	editor->setText(index.data().toString());
	return editor;
      }
    case dooble_charts::Properties::CHART_AXIS_X_LABELS_FONT:
    case dooble_charts::Properties::CHART_AXIS_X_TITLE_FONT:
    case dooble_charts::Properties::CHART_AXIS_Y_LABELS_FONT:
    case dooble_charts::Properties::CHART_AXIS_Y_TITLE_FONT:
    case dooble_charts::Properties::CHART_TITLE_FONT:
      {
	auto editor = new QPushButton(parent);

	connect(editor,
		SIGNAL(clicked(void)),
		this,
		SLOT(slot_show_font_dialog(void)));
	editor->setProperty("property", property);
	editor->setStyleSheet
	  (QString("QPushButton {background-color: %1;}").
	   arg(index.data().toString()));
	editor->setText(index.data().toString());
	return editor;
      }
    case dooble_charts::Properties::CHART_BACKGROUND_ROUNDNESS:
      {
	auto editor = new QDoubleSpinBox(parent);

	editor->setRange(0.0, std::numeric_limits<qreal>::max());
	editor->setValue(index.data().toReal());
	return editor;
      }
    case dooble_charts::Properties::CHART_LOCALE:
      {
	auto editor = new QComboBox(parent);
	int i = -1;

	editor->addItem(tr("en_US"));
	i = editor->findText(index.data().toString());

	if(i == -1)
	  i = 0;

	editor->setCurrentIndex(i);
	return editor;
      }
    case dooble_charts::Properties::CHART_THEME:
      {
	auto editor = new QComboBox(parent);
	int i = -1;

	editor->addItem(tr("Blue Cerulean"));
	editor->addItem(tr("Blue Icy"));
	editor->addItem(tr("Blue NCS"));
	editor->addItem(tr("Brown Sand"));
	editor->addItem(tr("Dark"));
	editor->addItem(tr("High Contrast"));
	editor->addItem(tr("Light"));
	editor->addItem(tr("Qt"));
	i = editor->findText(index.data().toString());

	if(i == -1)
	  i = editor->count() - 1; // Qt

	editor->setCurrentIndex(i);
	return editor;
      }
    case dooble_charts::Properties::DATA_EXTRACTION_SCRIPT:
      {
	auto editor = new QPlainTextEdit(parent);

	editor->setPlainText(index.data().toString().trimmed());
	return editor;
      }
    case dooble_charts::Properties::DATA_SOURCE_TYPE:
      {
	auto editor = new QComboBox(parent);
	int i = -1;

	editor->addItem(tr("File"));
	editor->addItem(tr("IP Address"));
	i = editor->findText(index.data().toString());

	if(i == -1)
	  i = 0;

	editor->setCurrentIndex(i);
	return editor;
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
(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
  if(qobject_cast<QPushButton *> (editor))
    return;

  QStyledItemDelegate::setModelData(editor, model, index);
}

void dooble_charts_property_editor_model_delegate::slot_show_color_dialog(void)
{
  auto editor = qobject_cast<QPushButton *> (sender());

  if(editor)
    emit show_color_dialog
      (dooble_charts::Properties(editor->property("property").toInt()));
}

void dooble_charts_property_editor_model_delegate::slot_show_font_dialog(void)
{
  auto editor = qobject_cast<QPushButton *> (sender());

  if(editor)
    emit show_font_dialog
      (dooble_charts::Properties(editor->property("property").toInt()));
}

void dooble_charts_property_editor_model_delegate::slot_text_changed(void)
{
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
  setHorizontalHeaderLabels(QStringList() << tr("Property") << tr("Value"));

  /*
  ** Chart
  */

  QStandardItem *chart_margins = nullptr;
  auto chart = new QStandardItem(tr("Chart"));

  chart->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

  for(int i = 0; !dooble_charts::s_chart_properties_strings[i].isEmpty(); i++)
    {
      if(dooble_charts::Properties(i) ==
	 dooble_charts::Properties::CHART_MARGINS)
	{
	  chart_margins = new QStandardItem
	    (dooble_charts::s_chart_properties_strings[i]);
	  chart_margins->setData(dooble_charts::Properties(i));
	  chart_margins->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	  chart->appendRow(chart_margins);
	  continue;
	}

      QList<QStandardItem *> list;
      auto item = new QStandardItem
	(dooble_charts::s_chart_properties_strings[i]);
      auto offset = i;

      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      list << item;
      item = new QStandardItem();
      item->setData(dooble_charts::Properties(offset));
      item->setFlags
	(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

      switch(dooble_charts::Properties(offset))
	{
	case dooble_charts::Properties::CHART_BACKGROUND_VISIBLE:
	case dooble_charts::Properties::CHART_DROP_SHADOW_ENABLED:
	case dooble_charts::Properties::CHART_LOCALIZE_NUMBERS:
	case dooble_charts::Properties::CHART_PLOT_AREA_BACKGROUND_VISIBLE:
	  {
	    item->setFlags(Qt::ItemIsEnabled |
			   Qt::ItemIsSelectable |
			   Qt::ItemIsUserCheckable);
	    break;
	  }
	case dooble_charts::Properties::CHART_MARGINS:
	  {
	    break;
	  }
	case dooble_charts::Properties::CHART_MARGINS_BOTTOM:
	case dooble_charts::Properties::CHART_MARGINS_LEFT:
	case dooble_charts::Properties::CHART_MARGINS_RIGHT:
	case dooble_charts::Properties::CHART_MARGINS_TOP:
	  {
	    list << item;
	    chart_margins->appendRow(list);
	    continue;
	  }
	case dooble_charts::Properties::CHART_NAME:
	  {
	    item->setToolTip
	      ("<html>" +
	       tr("The chart will be saved via the provided "
		  "name. Please specify a unique value.") +
	       "</html>");
	    break;
	  }
	case dooble_charts::Properties::CHART_CHART_TYPE:
	  {
	    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	    item->setToolTip(tr("Read-Only"));
	    break;
	  }
	default:
	  {
	    break;
	  }
	}

      list << item;
      chart->appendRow(list);
    }

  /*
  ** Chart X-Axis
  */

  auto chart_axis_x = new QStandardItem(tr("Chart X-Axis"));

  chart_axis_x->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

  for(int i = 0; !dooble_charts::s_axis_properties_strings[i].isEmpty(); i++)
    {
      QList<QStandardItem *> list;
      auto item = new QStandardItem
	(dooble_charts::s_axis_properties_strings[i]);
      auto offset = 4 + chart->rowCount() + i;

      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      list << item;
      item = new QStandardItem();
      item->setData(dooble_charts::Properties(offset));
      item->setFlags
	(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

      switch(dooble_charts::Properties(offset))
	{
	case dooble_charts::Properties::CHART_AXIS_X_ALIGNMENT_HORIZONTAL:
	case dooble_charts::Properties::CHART_AXIS_X_ALIGNMENT_VERTICAL:
	  {
	    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	    item->setToolTip(tr("Read-Only"));
	    break;
	  }
	case dooble_charts::Properties::CHART_AXIS_X_GRID_VISIBLE:
	case dooble_charts::Properties::CHART_AXIS_X_LABELS_VISIBLE:
	case dooble_charts::Properties::CHART_AXIS_X_LINE_VISIBLE:
	case dooble_charts::Properties::CHART_AXIS_X_MINOR_GRID_LINE_VISIBLE:
	case dooble_charts::Properties::CHART_AXIS_X_REVERSE:
	case dooble_charts::Properties::CHART_AXIS_X_SHADES_VISIBLE:
	case dooble_charts::Properties::CHART_AXIS_X_TITLE_VISIBLE:
	case dooble_charts::Properties::CHART_AXIS_X_VISIBLE:
	  {
	    item->setFlags(Qt::ItemIsEnabled |
			   Qt::ItemIsSelectable |
			   Qt::ItemIsUserCheckable);
	    break;
	  }
	default:
	  {
	    break;
	  }
	}

      list << item;
      chart_axis_x->appendRow(list);
    }

  /*
  ** Chart Y-Axis
  */

  auto chart_axis_y = new QStandardItem(tr("Chart Y-Axis"));

  chart_axis_y->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

  for(int i = 0; !dooble_charts::s_axis_properties_strings[i].isEmpty(); i++)
    {
      QList<QStandardItem *> list;
      auto item = new QStandardItem
	(dooble_charts::s_axis_properties_strings[i]);
      auto offset = 4 +
	chart_axis_x->rowCount() +
	chart->rowCount() +
	i;

      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      list << item;
      item = new QStandardItem();
      item->setData(dooble_charts::Properties(offset));
      item->setFlags
	(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

      switch(dooble_charts::Properties(offset))
	{
	case dooble_charts::Properties::CHART_AXIS_Y_ALIGNMENT_HORIZONTAL:
	case dooble_charts::Properties::CHART_AXIS_Y_ALIGNMENT_VERTICAL:
	  {
	    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	    item->setToolTip(tr("Read-Only"));
	    break;
	  }
	case dooble_charts::Properties::CHART_AXIS_Y_GRID_VISIBLE:
	case dooble_charts::Properties::CHART_AXIS_Y_LABELS_VISIBLE:
	case dooble_charts::Properties::CHART_AXIS_Y_LINE_VISIBLE:
	case dooble_charts::Properties::CHART_AXIS_Y_MINOR_GRID_LINE_VISIBLE:
	case dooble_charts::Properties::CHART_AXIS_Y_REVERSE:
	case dooble_charts::Properties::CHART_AXIS_Y_SHADES_VISIBLE:
	case dooble_charts::Properties::CHART_AXIS_Y_TITLE_VISIBLE:
	case dooble_charts::Properties::CHART_AXIS_Y_VISIBLE:
	  {
	    item->setFlags(Qt::ItemIsEnabled |
			   Qt::ItemIsSelectable |
			   Qt::ItemIsUserCheckable);
	    break;
	  }
	default:
	  {
	    break;
	  }
	}

      list << item;
      chart_axis_y->appendRow(list);
    }

  /*
  ** Data
  */

  auto data = new QStandardItem(tr("Data"));

  data->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

  for(int i = 0; !dooble_charts::s_data_properties_strings[i].isEmpty(); i++)
    {
      QList<QStandardItem *> list;
      auto item = new QStandardItem
	(dooble_charts::s_data_properties_strings[i]);
      auto offset = 4 +
	chart->rowCount() +
	chart_axis_x->rowCount() +
	chart_axis_y->rowCount() +
	i;

      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      list << item;
      item = new QStandardItem();
      item->setData(dooble_charts::Properties(offset));
      item->setFlags
	(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      list << item;
      data->appendRow(list);
    }

  /*
  ** Legend
  */

  auto legend = new QStandardItem(tr("Legend"));

  legend->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

  for(int i = 0; !dooble_charts::s_legend_properties_strings[i].isEmpty(); i++)
    {
      QList<QStandardItem *> list;
      auto item = new QStandardItem
	(dooble_charts::s_legend_properties_strings[i]);
      auto offset = 4 +
	chart->rowCount() +
	chart_axis_x->rowCount() +
	data->rowCount() +
	i;

      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      list << item;
      item = new QStandardItem();
      item->setData(dooble_charts::Properties(offset));
      item->setFlags
	(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

      switch(dooble_charts::Properties(offset))
	{
	case dooble_charts::Properties::LEGEND_BACKGROUND_VISIBLE:
	case dooble_charts::Properties::LEGEND_REVERSE_MARKERS:
	case dooble_charts::Properties::LEGEND_SHOW_TOOL_TIPS:
	case dooble_charts::Properties::LEGEND_VISIBLE:
	  {
	    item->setFlags(Qt::ItemIsEnabled |
			   Qt::ItemIsSelectable |
			   Qt::ItemIsUserCheckable);
	    break;
	  }
	default:
	  {
	    break;
	  }
	}

      list << item;
      legend->appendRow(list);
    }

  appendRow(chart);
  appendRow(chart_axis_x);
  appendRow(chart_axis_y);
  appendRow(data);
  appendRow(legend);
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
  auto list(findItems(tr("Chart")) +
	    findItems(tr("Chart X-Axis")) +
	    findItems(tr("Chart Y-Axis")) +
	    findItems(tr("Data")) +
	    findItems(tr("Legend")));

  for(int i = 0; i < list.size(); i++)
    if(list.at(i))
      for(int j = 0; j < list.at(i)->rowCount(); j++)
	{
	  auto item = list.at(i)->child(j, column);

	  if(!item)
	    continue;

	  auto p = dooble_charts::Properties
	    (item->data(Qt::ItemDataRole(Qt::UserRole + 1)).toInt());

	  if(p == property)
	    return item;
	}

  return nullptr;
}

dooble_charts_property_editor::
dooble_charts_property_editor(QTreeView *tree):QWidget(tree)
{
  m_tree = tree;

  if(m_tree)
    {
      auto item_delegate = m_tree->itemDelegate();

      if(item_delegate)
	item_delegate->deleteLater();

      item_delegate = new dooble_charts_property_editor_model_delegate(this);
      connect
	(item_delegate,
	 SIGNAL(show_color_dialog(const dooble_charts::Properties)),
	 this,
	 SLOT(slot_show_color_dialog(const dooble_charts::Properties)));
      connect
	(item_delegate,
	 SIGNAL(show_font_dialog(const dooble_charts::Properties)),
	 this,
	 SLOT(slot_show_font_dialog(const dooble_charts::Properties)));
      m_tree->setItemDelegate(item_delegate);
    }
}

dooble_charts_property_editor::~dooble_charts_property_editor()
{
}

QPointer<dooble_charts_property_editor_model> dooble_charts_property_editor::
model(void) const
{
  return m_model;
}

QVariant dooble_charts_property_editor::property
(const dooble_charts::Properties property)
{
  if(!m_model)
    return QVariant();

  auto item = m_model->item_from_property(property, 1);

  if(item)
    {
      if(item->flags() & Qt::ItemIsUserCheckable)
	return item->checkState() == Qt::Checked;
      else
	return item->text();
    }

  return QVariant();
}

void dooble_charts_property_editor::prepare_generic(dooble_charts *chart)
{
  if(!chart || !m_model || !m_tree)
    return;

  QHashIterator<dooble_charts::Properties, QVariant> it
    (chart->properties().
     unite(chart->x_axis_properties()).
     unite(chart->y_axis_properties()));

  while(it.hasNext())
    {
      it.next();

      auto item = m_model->item_from_property(it.key(), 1);

      if(item)
	switch(it.key())
	  {
	  case dooble_charts::Properties::CHART_AXIS_X_COLOR:
	  case dooble_charts::Properties::CHART_AXIS_X_GRID_LINE_COLOR:
	  case dooble_charts::Properties::CHART_AXIS_X_LABELS_COLOR:
	  case dooble_charts::Properties::CHART_AXIS_X_MINOR_GRID_LINE_COLOR:
	  case dooble_charts::Properties::CHART_AXIS_X_SHADES_BORDER_COLOR:
	  case dooble_charts::Properties::CHART_AXIS_X_SHADES_COLOR:
	  case dooble_charts::Properties::CHART_AXIS_X_TITLE_COLOR:
	  case dooble_charts::Properties::CHART_AXIS_Y_COLOR:
	  case dooble_charts::Properties::CHART_AXIS_Y_GRID_LINE_COLOR:
	  case dooble_charts::Properties::CHART_AXIS_Y_LABELS_COLOR:
	  case dooble_charts::Properties::CHART_AXIS_Y_MINOR_GRID_LINE_COLOR:
	  case dooble_charts::Properties::CHART_AXIS_Y_SHADES_BORDER_COLOR:
	  case dooble_charts::Properties::CHART_AXIS_Y_SHADES_COLOR:
	  case dooble_charts::Properties::CHART_AXIS_Y_TITLE_COLOR:
	  case dooble_charts::Properties::CHART_BACKGROUND_COLOR:
	  case dooble_charts::Properties::CHART_TITLE_COLOR:
	    {
	      item->setBackground(QColor(it.value().toString()));
	      item->setText(it.value().toString());
	      break;
	    }
	  case dooble_charts::Properties::CHART_AXIS_X_GRID_VISIBLE:
	  case dooble_charts::Properties::CHART_AXIS_X_LABELS_VISIBLE:
	  case dooble_charts::Properties::CHART_AXIS_X_LINE_VISIBLE:
	  case dooble_charts::Properties::CHART_AXIS_X_MINOR_GRID_LINE_VISIBLE:
	  case dooble_charts::Properties::CHART_AXIS_X_REVERSE:
	  case dooble_charts::Properties::CHART_AXIS_X_SHADES_VISIBLE:
	  case dooble_charts::Properties::CHART_AXIS_X_TITLE_VISIBLE:
	  case dooble_charts::Properties::CHART_AXIS_X_VISIBLE:
	  case dooble_charts::Properties::CHART_AXIS_Y_GRID_VISIBLE:
	  case dooble_charts::Properties::CHART_AXIS_Y_LABELS_VISIBLE:
	  case dooble_charts::Properties::CHART_AXIS_Y_LINE_VISIBLE:
	  case dooble_charts::Properties::CHART_AXIS_Y_MINOR_GRID_LINE_VISIBLE:
	  case dooble_charts::Properties::CHART_AXIS_Y_REVERSE:
	  case dooble_charts::Properties::CHART_AXIS_Y_SHADES_VISIBLE:
	  case dooble_charts::Properties::CHART_AXIS_Y_TITLE_VISIBLE:
	  case dooble_charts::Properties::CHART_AXIS_Y_VISIBLE:
	  case dooble_charts::Properties::CHART_BACKGROUND_VISIBLE:
	  case dooble_charts::Properties::CHART_DROP_SHADOW_ENABLED:
	  case dooble_charts::Properties::CHART_LOCALIZE_NUMBERS:
	  case dooble_charts::Properties::CHART_PLOT_AREA_BACKGROUND_VISIBLE:
	  case dooble_charts::Properties::LEGEND_VISIBLE:
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

  m_tree->setModel(m_model);

  QStandardItem *item = m_model->item_from_property
    (dooble_charts::Properties::CHART_MARGINS, 0);

  if(item && item->parent())
    {
      m_tree->setFirstColumnSpanned
	(dooble_charts::Properties::CHART_MARGINS,
	 item->parent()->index(),
	 true);

      for(int i = 0; i < 4; i++)
	if(item->child(i, 1))
	  {
	    auto property = dooble_charts::Properties
	      (item->child(i, 1)->data(Qt::ItemDataRole(Qt::UserRole + 1)).
	       toInt());

	    item->child(i, 1)->setText
	      (chart->properties().value(property).toString());
	  }
    }

  m_tree->setFirstColumnSpanned(0, m_tree->rootIndex(), true);
  m_tree->setFirstColumnSpanned(1, m_tree->rootIndex(), true);
  m_tree->setFirstColumnSpanned(2, m_tree->rootIndex(), true);
  m_tree->setFirstColumnSpanned(3, m_tree->rootIndex(), true);
  m_tree->expandAll();
  m_tree->resizeColumnToContents(0);
  m_tree->resizeColumnToContents(1);
}

void dooble_charts_property_editor::slot_show_color_dialog
(const dooble_charts::Properties property)
{
  if(!m_model)
    return;

  auto item = m_model->item_from_property(property, 1);

  if(!item)
    return;

  QColorDialog dialog(this);

  dialog.setCurrentColor(QColor(item->text()));

  if(dialog.exec() == QDialog::Accepted)
    {
      item->setBackground(dialog.selectedColor());
      item->setText(dialog.selectedColor().name());
    }
}

void dooble_charts_property_editor::slot_show_font_dialog
(const dooble_charts::Properties property)
{
  if(!m_model)
    return;

  auto item = m_model->item_from_property(property, 1);

  if(!item)
    return;

  QFont font;
  QFontDialog dialog(this);

  font.fromString(item->text());
  dialog.setCurrentFont(font);

  if(dialog.exec() == QDialog::Accepted)
    item->setText(dialog.selectedFont().toString());
}
