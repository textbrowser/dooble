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
#include <QFileDialog>
#include <QFontDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QtMath>

#include "dooble_application.h"
#include "dooble_charts_property_editor.h"

#include <limits>

static void find_recursive_items(QStandardItem *item,
				 QList<QStandardItem *> &list)
{
  if(!item)
    return;

  list << item;

  for(int i = 0; i < item->rowCount(); ++i)
    for(int j = 0; j < item->columnCount(); ++j)
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
    case dooble_charts::Properties::DATA_SOURCE_ADDRESS:
    case dooble_charts::Properties::DATA_SOURCE_READ_RATE:
      {
	size.setHeight(qMax(50, size.height()));
	break;
      }
    default:
      {
	size.setHeight(qCeil(1.50 * size.height()));
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
    case dooble_charts::Properties::DATA_SOURCE_READ_BUFFER_SIZE:
    case dooble_charts::Properties::XY_SERIES_X_AXIS_MINOR_TICK_COUNT:
    case dooble_charts::Properties::XY_SERIES_X_AXIS_TICK_COUNT:
    case dooble_charts::Properties::XY_SERIES_Y_AXIS_MINOR_TICK_COUNT:
    case dooble_charts::Properties::XY_SERIES_Y_AXIS_TICK_COUNT:
      {
	auto editor = new QSpinBox(parent);

	switch(property)
	  {
	  case dooble_charts::Properties::DATA_SOURCE_READ_BUFFER_SIZE:
	    {
	      editor->setRange(512, std::numeric_limits<int>::max());
	      break;
	    }
	  case dooble_charts::Properties::XY_SERIES_X_AXIS_TICK_COUNT:
	  case dooble_charts::Properties::XY_SERIES_Y_AXIS_TICK_COUNT:
	    {
	      editor->setRange(2, std::numeric_limits<int>::max());
	      break;
	    }
	  default:
	    {
	      editor->setRange(0, std::numeric_limits<int>::max());
	      break;
	    }
	  }

	editor->setToolTip
	  (QString("[%1, %2]").arg(editor->minimum()).arg(editor->maximum()));
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
    case dooble_charts::Properties::LEGEND_BORDER_COLOR:
    case dooble_charts::Properties::LEGEND_COLOR:
    case dooble_charts::Properties::LEGEND_LABEL_COLOR:
    case dooble_charts::Properties::XY_SERIES_COLOR:
    case dooble_charts::Properties::XY_SERIES_POINT_LABELS_COLOR:
      {
	auto editor = new QPushButton(parent);

	connect(editor,
		SIGNAL(clicked(void)),
		this,
		SLOT(slot_show_color_dialog(void)),
		Qt::QueuedConnection);
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
    case dooble_charts::Properties::LEGEND_FONT:
    case dooble_charts::Properties::XY_SERIES_POINT_LABELS_FONT:
      {
	auto editor = new QPushButton(parent);

	connect(editor,
		SIGNAL(clicked(void)),
		this,
		SLOT(slot_show_font_dialog(void)),
		Qt::QueuedConnection);
	editor->setProperty("property", property);
	editor->setStyleSheet
	  (QString("QPushButton {background-color: %1;}").
	   arg(index.data().toString()));
	editor->setText(index.data().toString());
	return editor;
      }
    case dooble_charts::Properties::CHART_BACKGROUND_ROUNDNESS:
    case dooble_charts::Properties::XY_SERIES_OPACITY:
    case dooble_charts::Properties::XY_SERIES_X_AXIS_MAX:
    case dooble_charts::Properties::XY_SERIES_X_AXIS_MIN:
    case dooble_charts::Properties::XY_SERIES_X_AXIS_TICK_ANCHOR:
    case dooble_charts::Properties::XY_SERIES_X_AXIS_TICK_INTERVAL:
    case dooble_charts::Properties::XY_SERIES_Y_AXIS_MAX:
    case dooble_charts::Properties::XY_SERIES_Y_AXIS_MIN:
    case dooble_charts::Properties::XY_SERIES_Y_AXIS_TICK_ANCHOR:
    case dooble_charts::Properties::XY_SERIES_Y_AXIS_TICK_INTERVAL:
      {
	auto editor = new QDoubleSpinBox(parent);

	switch(property)
	  {
	  case dooble_charts::Properties::XY_SERIES_X_AXIS_TICK_INTERVAL:
	  case dooble_charts::Properties::XY_SERIES_Y_AXIS_TICK_INTERVAL:
	    {
	      editor->setRange(0.1, std::numeric_limits<qreal>::max());
	      break;
	    }
	  case dooble_charts::Properties::XY_SERIES_X_AXIS_MAX:
	  case dooble_charts::Properties::XY_SERIES_X_AXIS_MIN:
	  case dooble_charts::Properties::XY_SERIES_Y_AXIS_MAX:
	  case dooble_charts::Properties::XY_SERIES_Y_AXIS_MIN:
	    {
	      editor->setRange(std::numeric_limits<qreal>::lowest(),
			       std::numeric_limits<qreal>::max());
	      break;
	    }
	  default:
	    {
	      editor->setRange(0.0, std::numeric_limits<qreal>::max());
	      break;
	    }
	  }

	editor->setToolTip
	  (QString("[%1, %2]").arg(editor->minimum()).arg(editor->maximum()));
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
    case dooble_charts::Properties::DATA_SOURCE_ADDRESS:
      {
	auto editor = new QFrame(parent);
	auto line_edit = new QLineEdit(editor);
	auto push_button = new QPushButton(tr("Select"), editor);

	connect(push_button,
		SIGNAL(clicked(void)),
		this,
		SLOT(slot_show_file_dialog(void)),
		Qt::QueuedConnection);
	delete editor->layout();
	editor->setAutoFillBackground(true);
	editor->setLayout(new QHBoxLayout());
	editor->layout()->addWidget(line_edit);
	editor->layout()->addWidget(push_button);
	editor->layout()->setContentsMargins(0, 0, 0, 0);
	editor->layout()->setSpacing(0);
	line_edit->setObjectName("source");
#ifdef Q_OS_MACOS
	line_edit->setMinimumHeight(push_button->height());
#endif
	line_edit->setText(index.data().toString());
	push_button->setProperty("property", property);
	return editor;
      }
    case dooble_charts::Properties::DATA_SOURCE_READ_RATE:
      {
	auto editor = new QFrame(parent);
	auto list(index.data().toString().split("/"));
	auto spin_box_1 = new QSpinBox(editor);
	auto spin_box_2 = new QSpinBox(editor);

	delete editor->layout();
	editor->setAutoFillBackground(true);
	editor->setLayout(new QHBoxLayout());
	editor->layout()->addWidget(spin_box_1);
	editor->layout()->addWidget(spin_box_2);
	editor->layout()->setContentsMargins(0, 0, 0, 0);
	editor->layout()->setSpacing(0);
	spin_box_1->setMaximum(std::numeric_limits<int>::max());
	spin_box_1->setMinimum(1);
	spin_box_1->setObjectName("bytes");
	spin_box_1->setSuffix(tr(" Bytes"));
	spin_box_1->setValue(list.value(0).trimmed().toInt());
	spin_box_2->setMaximum(std::numeric_limits<int>::max());
	spin_box_2->setMinimum(1);
	spin_box_2->setObjectName("milliseconds");
	spin_box_2->setSuffix(tr(" Millisecond(s)"));
	spin_box_2->setValue(list.value(1).trimmed().toInt());
	return editor;
      }
    case dooble_charts::Properties::DATA_SOURCE_TYPE:
      {
	auto editor = new QComboBox(parent);
	int i = -1;

	editor->addItem(tr("Binary File"));
	editor->addItem(tr("Text File"));
	i = editor->findText(index.data().toString());

	if(i == -1)
	  i = 1; // Text File

	editor->setCurrentIndex(i);
	return editor;
      }
    case dooble_charts::Properties::LEGEND_ALIGNMENT:
      {
	auto editor = new QComboBox(parent);
	int i = -1;

	editor->addItem(tr("Bottom"));
	editor->addItem(tr("Left"));
	editor->addItem(tr("Right"));
	editor->addItem(tr("Top"));
	i = editor->findText(index.data().toString());

	if(i == -1)
	  i = editor->count() - 1; // Top

	editor->setCurrentIndex(i);
	return editor;
      }
    case dooble_charts::Properties::LEGEND_MARKER_SHAPE:
      {
	auto editor = new QComboBox(parent);
	int i = -1;

	editor->addItem(tr("Circle"));
	editor->addItem(tr("Default"));
	editor->addItem(tr("From Series"));
	editor->addItem(tr("Rectangle"));
	i = editor->findText(index.data().toString());

	if(i == -1)
	  i = editor->count() - 1; // Rectangle

	editor->setCurrentIndex(i);
	return editor;
      }
    case dooble_charts::Properties::XY_SERIES_X_AXIS_TICK_TYPE:
    case dooble_charts::Properties::XY_SERIES_Y_AXIS_TICK_TYPE:
      {
	auto editor = new QComboBox(parent);
	int i = -1;

	editor->addItem(tr("Dynamic"));
	editor->addItem(tr("Fixed"));
	i = editor->findText(index.data().toString());

	if(i == -1)
	  i = editor->count() - 1; // Fixed

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

  if(model)
    {
      auto frame = qobject_cast<QFrame *> (editor);

      if(frame)
	{
	  auto line_edit = frame->findChild<QLineEdit *> ("source");

	  if(line_edit)
	    {
	      model->setData(index, line_edit->text());
	      return;
	    }

	  auto spin_box_1 = frame->findChild<QSpinBox *> ("bytes");
	  auto spin_box_2 = frame->findChild<QSpinBox *> ("milliseconds");

	  if(spin_box_1 && spin_box_2)
	    {
	      auto value1 = spin_box_1->value();
	      auto value2 = spin_box_2->value();

	      model->setData
		(index, QString("%1 / %2").arg(value1).arg(value2));
	      return;
	    }
	}
    }

  QStyledItemDelegate::setModelData(editor, model, index);
}

void dooble_charts_property_editor_model_delegate::slot_show_color_dialog(void)
{
  auto editor = qobject_cast<QPushButton *> (sender());

  if(editor)
    emit show_color_dialog
      (dooble_charts::Properties(editor->property("property").toInt()));
}

void dooble_charts_property_editor_model_delegate::slot_show_file_dialog(void)
{
  auto editor = qobject_cast<QPushButton *> (sender());

  if(editor)
    emit show_file_dialog
      (editor, dooble_charts::Properties(editor->property("property").toInt()));
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

  for(int i = 0; !dooble_charts::s_chart_properties_strings[i].isEmpty(); ++i)
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
	case dooble_charts::Properties::CHART_ANIMATION_DURATION:
	  {
	    item->setToolTip
	      ("<html>" +
	       tr("The duration, in milliseconds, of the animations. "
		  "Please note that theme changes may cause animations. "
		  "After a chart is loaded from disk, chart properties will "
		  "be applied after the animations complete.") +
	       "</html>");
	    break;
	  }
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
	case dooble_charts::Properties::CHART_THEME:
	  {
	    item->setToolTip
	      ("<html>" +
	       tr("Other chart properties will be applied after the "
		  "new theme is applied.") +
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

  for(int i = 0; !dooble_charts::s_axis_properties_strings[i].isEmpty(); ++i)
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
	case dooble_charts::Properties::CHART_AXIS_X_ORIENTATION:
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

  for(int i = 0; !dooble_charts::s_axis_properties_strings[i].isEmpty(); ++i)
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
	case dooble_charts::Properties::CHART_AXIS_Y_ORIENTATION:
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

  for(int i = 0; !dooble_charts::s_data_properties_strings[i].isEmpty(); ++i)
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

      switch(dooble_charts::Properties(offset))
	{
	case dooble_charts::Properties::DATA_SOURCE_ADDRESS:
	  {
	    break;
	  }
	default:
	  {
	    break;
	  }
	}

      list << item;
      data->appendRow(list);
    }

  /*
  ** Legend
  */

  auto legend = new QStandardItem(tr("Legend"));

  legend->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

  for(int i = 0; !dooble_charts::s_legend_properties_strings[i].isEmpty(); ++i)
    {
      QList<QStandardItem *> list;
      auto item = new QStandardItem
	(dooble_charts::s_legend_properties_strings[i]);
      auto offset = 4 +
	chart->rowCount() +
	chart_axis_x->rowCount() +
	chart_axis_y->rowCount() +
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

      switch(dooble_charts::Properties(offset))
	{
	case dooble_charts::Properties::LEGEND_REVERSE_MARKERS:
	  {
	    item->setToolTip
	      (tr("Reverse order for the markers in the legend."));
	    break;
	  }
	case dooble_charts::Properties::LEGEND_SHOW_TOOL_TIPS:
	  {
	    item->setToolTip(tr("Show tool tips if the text is truncated."));
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

  for(int i = 0; i < list.size(); ++i)
    {
      auto item = list.at(i);

      if(!item)
	continue;

      all << item;

      for(int j = 0; j < item->rowCount(); ++j)
        for(int k = 0; k < item->columnCount(); ++k)
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
  auto list(find_all_child_items(tr("Chart")) +
	    find_all_child_items(tr("Chart X-Axis")) +
	    find_all_child_items(tr("Chart Y-Axis")) +
	    find_all_child_items(tr("Data")) +
	    find_all_child_items(tr("Legend")) +
	    find_all_child_items(tr("XY Series")));

  for(int i = 0; i < list.size(); ++i)
    if(list.at(i))
      for(int j = 0; j < list.at(i)->rowCount(); ++j)
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
      m_collapse = new QToolButton(tree);

      auto font(m_collapse->font());

      font.setStyleHint(QFont::Courier);
      m_collapse->resize(25, 25);
      m_collapse->setCheckable(true);
      m_collapse->setChecked(true);
      m_collapse->setFont(font);
      m_collapse->setStyleSheet("QToolButton {border: none;}"
				"QToolButton::menu-button {border: none;}");
      m_collapse->setText(tr("-"));
      m_collapse->setToolTip(tr("Collapse / Expand"));

      auto item_delegate = m_tree->itemDelegate();

      if(item_delegate)
	item_delegate->deleteLater();

      item_delegate = new dooble_charts_property_editor_model_delegate(this);
      connect
	(item_delegate,
	 SIGNAL(show_color_dialog(const dooble_charts::Properties)),
	 this,
	 SLOT(slot_show_color_dialog(const dooble_charts::Properties)),
	 Qt::QueuedConnection);
      connect
	(item_delegate,
	 SIGNAL(show_file_dialog(QPushButton *,
				 const dooble_charts::Properties)),
	 this,
	 SLOT(slot_show_file_dialog(QPushButton *,
				    const dooble_charts::Properties)),
	 Qt::QueuedConnection);
      connect
	(item_delegate,
	 SIGNAL(show_font_dialog(const dooble_charts::Properties)),
	 this,
	 SLOT(slot_show_font_dialog(const dooble_charts::Properties)),
	 Qt::QueuedConnection);
      connect(m_collapse,
	      SIGNAL(clicked(void)),
	      this,
	      SLOT(slot_collapse_all(void)));
      connect(m_tree->horizontalScrollBar(),
	      SIGNAL(valueChanged(int)),
	      this,
	      SLOT(slot_horizontal_scroll_bar_value_changed(int)));
      m_tree->header()->setDefaultAlignment(Qt::AlignCenter);
      m_tree->header()->setMinimumHeight(30);
      m_tree->setItemDelegate(item_delegate);
      m_tree->setMinimumWidth(200);
      m_collapse->move(5, (m_tree->header()->size().height() - 25) / 2 + 2);
    }
  else
    m_collapse = nullptr;
}

dooble_charts_property_editor::~dooble_charts_property_editor()
{
}

QPointer<dooble_charts_property_editor_model> dooble_charts_property_editor::
model(void) const
{
  return m_model;
}

QStandardItem *dooble_charts_property_editor::item_from_property
(const dooble_charts::Properties property, const int column) const
{
  if(m_model)
    return m_model->item_from_property(property, column);
  else
    return nullptr;
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

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  QMultiHash<dooble_charts::Properties, QVariant> hash;

  hash.unite(chart->data_properties());
  hash.unite(chart->legend_properties());
  hash.unite(chart->properties());
  hash.unite(chart->x_axis_properties());
  hash.unite(chart->y_axis_properties());

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QHashIterator<dooble_charts::Properties, QVariant> it(hash);
#else
  QMultiHashIterator<dooble_charts::Properties, QVariant> it(hash);
#endif
#else
  QHashIterator<dooble_charts::Properties, QVariant> it
    (chart->data_properties().
     unite(chart->legend_properties()).
     unite(chart->properties()).
     unite(chart->x_axis_properties()).
     unite(chart->y_axis_properties()));
#endif

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
	  case dooble_charts::Properties::LEGEND_BORDER_COLOR:
	  case dooble_charts::Properties::LEGEND_COLOR:
	  case dooble_charts::Properties::LEGEND_LABEL_COLOR:
	  case dooble_charts::Properties::XY_SERIES_COLOR:
	  case dooble_charts::Properties::XY_SERIES_POINT_LABELS_COLOR:
	    {
	      item->setBackground(QColor(it.value().toString()));
	      item->setText(it.value().toString());
	      break;
	    }
	  case dooble_charts::Properties::DATA_SOURCE_ADDRESS:
	    {
	      item->setText(it.value().toString());
	      item->setToolTip(item->text());
	      break;
	    }
	  default:
	    {
	      if(Qt::ItemIsUserCheckable & item->flags())
		item->setCheckState
		  (it.value().toBool() ? Qt::Checked : Qt::Unchecked);
	      else
		item->setText(it.value().toString());

	      break;
	    }
	  }
    }

  m_tree->setModel(m_model);

  auto item = m_model->item_from_property
    (dooble_charts::Properties::CHART_MARGINS, 0);

  if(item && item->parent())
    {
      m_tree->setFirstColumnSpanned
	(dooble_charts::Properties::CHART_MARGINS,
	 item->parent()->index(),
	 true);

      for(int i = 0; i < 4; ++i)
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
  m_tree->setFirstColumnSpanned(4, m_tree->rootIndex(), true);
  m_tree->expandAll();
  m_tree->resizeColumnToContents(0);
  m_tree->resizeColumnToContents(1);
}

void dooble_charts_property_editor::scroll_to_item
(const dooble_charts::Properties property)
{
  if(!m_model)
    return;

  auto item = m_model->item_from_property(property, 1);

  if(item)
    {
      m_tree->scrollTo(item->index(), QAbstractItemView::PositionAtTop);

      if(m_tree->selectionModel())
	m_tree->selectionModel()->select
	  (item->index(), QItemSelectionModel::ClearAndSelect);
    }
}

void dooble_charts_property_editor::set_property
(const dooble_charts::Properties property, const QVariant &value)
{
  if(!m_model)
    return;

  auto item = m_model->item_from_property(property, 1);

  if(!item)
    return;

  if(item->flags() & Qt::ItemIsUserCheckable)
    item->setCheckState(value.toBool() ? Qt::Checked: Qt::Unchecked);
  else
    {
      switch(property)
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
	case dooble_charts::Properties::LEGEND_BORDER_COLOR:
	case dooble_charts::Properties::LEGEND_COLOR:
	case dooble_charts::Properties::LEGEND_LABEL_COLOR:
	case dooble_charts::Properties::XY_SERIES_COLOR:
	case dooble_charts::Properties::XY_SERIES_POINT_LABELS_COLOR:
	  {
	    item->setBackground(QColor(value.toString()));
	    break;
	  }
	default:
	  {
	    break;
	  }
	}

      item->setText(value.toString());
      m_model->setData(item->index(), value);
    }
}

void dooble_charts_property_editor::slot_collapse_all(void)
{
  if(!m_collapse || !m_tree)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(m_collapse->isChecked())
    {
      m_collapse->setText(tr("-"));
      m_tree->expandAll();
    }
  else
    {
      m_collapse->setText(tr("+"));
      m_tree->collapseAll();
    }

  QApplication::restoreOverrideCursor();
}

void dooble_charts_property_editor::slot_horizontal_scroll_bar_value_changed
(int value)
{
  if(m_collapse)
    m_collapse->setVisible(m_collapse->rect().right() > value);
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

void dooble_charts_property_editor::slot_show_file_dialog
(QPushButton *push_button, const dooble_charts::Properties property)
{
  if(!m_model)
    return;

  auto item = m_model->item_from_property(property, 1);

  if(!item)
    return;

  QFileDialog dialog(this);

  dialog.selectFile(item->text());
  dialog.setOption(QFileDialog::DontUseNativeDialog);

  if(dialog.exec() == QDialog::Accepted)
    {
      item->setText(dialog.selectedFiles().value(0));
      item->setToolTip(item->text());
      m_model->setData(item->index(), item->text());

      if(push_button)
	{
	  auto frame = qobject_cast<QFrame *> (push_button->parent());

	  if(frame)
	    {
	      auto line_edit = frame->findChild<QLineEdit *> ("source");

	      if(line_edit)
		line_edit->setText(item->text());
	    }
	}
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

  if(!item->text().trimmed().isEmpty() &&
     font.fromString(item->text().trimmed()))
    dialog.setCurrentFont(font);
  else
    dialog.setCurrentFont(dooble_application::font());

  if(dialog.exec() == QDialog::Accepted)
    item->setText(dialog.selectedFont().toString());
}
