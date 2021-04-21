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

#include "dooble_charts_property_editor_xyseries.h"
#include "dooble_charts_xyseries.h"

dooble_charts_property_editor_model_xyseries::
dooble_charts_property_editor_model_xyseries(QObject *parent):
  dooble_charts_property_editor_model(parent)
{
  /*
  ** Chart
  */

  QStandardItem *chart_x_axis = nullptr;
  QStandardItem *chart_y_axis = nullptr;
  auto chart = new QStandardItem(tr("XY Series"));

  chart->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

  for(int i = 0;
      !dooble_charts_xyseries::s_chart_properties_strings[i].isEmpty();
      i++)
    {
      auto offset = dooble_charts::Properties::XY_SERIES_COLOR + i;

      if(dooble_charts::Properties(offset) ==
	 dooble_charts::Properties::XY_SERIES_X_AXIS)
	{
	  chart_x_axis = new QStandardItem
	    (dooble_charts_xyseries::s_chart_properties_strings[i]);
	  chart_x_axis->setData(dooble_charts::Properties(offset));
	  chart_x_axis->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	  chart->appendRow(chart_x_axis);
	  continue;
	}
      else if(dooble_charts::Properties(offset) ==
	      dooble_charts::Properties::XY_SERIES_Y_AXIS)
	{
	  chart_y_axis = new QStandardItem
	    (dooble_charts_xyseries::s_chart_properties_strings[i]);
	  chart_y_axis->setData(dooble_charts::Properties(offset));
	  chart_y_axis->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	  chart->appendRow(chart_y_axis);
	  continue;
	}

      QList<QStandardItem *> list;
      auto item = new QStandardItem
	(dooble_charts_xyseries::s_chart_properties_strings[i]);

      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      list << item;
      item = new QStandardItem();
      item->setData(dooble_charts::Properties(offset));
      item->setFlags
	(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

      switch(dooble_charts::Properties(offset))
	{
	case dooble_charts::Properties::XY_SERIES_POINTS_VISIBLE:
	case dooble_charts::Properties::XY_SERIES_POINT_LABELS_CLIPPING:
	case dooble_charts::Properties::XY_SERIES_POINT_LABELS_VISIBLE:
	  {
	    item->setFlags(Qt::ItemIsEnabled |
			   Qt::ItemIsSelectable |
			   Qt::ItemIsUserCheckable);
	    break;
	  }
	case dooble_charts::Properties::XY_SERIES_X_AXIS:
	case dooble_charts::Properties::XY_SERIES_Y_AXIS:
	  {
	    break;
	  }
	case dooble_charts::Properties::XY_SERIES_X_AXIS_LABEL_FORMAT:
	case dooble_charts::Properties::XY_SERIES_X_AXIS_MAX:
	case dooble_charts::Properties::XY_SERIES_X_AXIS_MIN:
	case dooble_charts::Properties::XY_SERIES_X_AXIS_MINOR_TICK_COUNT:
	case dooble_charts::Properties::XY_SERIES_X_AXIS_TICK_ANCHOR:
	case dooble_charts::Properties::XY_SERIES_X_AXIS_TICK_COUNT:
	case dooble_charts::Properties::XY_SERIES_X_AXIS_TICK_INTERVAL:
	case dooble_charts::Properties::XY_SERIES_X_AXIS_TICK_TYPE:
	  {
	    list << item;
	    chart_x_axis->appendRow(list);
	    continue;
	  }
	case dooble_charts::Properties::XY_SERIES_Y_AXIS_LABEL_FORMAT:
	case dooble_charts::Properties::XY_SERIES_Y_AXIS_MAX:
	case dooble_charts::Properties::XY_SERIES_Y_AXIS_MIN:
	case dooble_charts::Properties::XY_SERIES_Y_AXIS_MINOR_TICK_COUNT:
	case dooble_charts::Properties::XY_SERIES_Y_AXIS_TICK_ANCHOR:
	case dooble_charts::Properties::XY_SERIES_Y_AXIS_TICK_COUNT:
	case dooble_charts::Properties::XY_SERIES_Y_AXIS_TICK_INTERVAL:
	case dooble_charts::Properties::XY_SERIES_Y_AXIS_TICK_TYPE:
	  {
	    list << item;
	    chart_y_axis->appendRow(list);
	    continue;
	  }
	default:
	  {
	    break;
	  }
	}

      list << item;
      chart->appendRow(list);
    }

  appendRow(chart);
}

dooble_charts_property_editor_model_xyseries::
~dooble_charts_property_editor_model_xyseries()
{
}

dooble_charts_property_editor_xyseries::
dooble_charts_property_editor_xyseries(QTreeView *tree, dooble_charts *chart):
  dooble_charts_property_editor(tree)
{
  m_model = new dooble_charts_property_editor_model_xyseries(this);
  prepare_generic(chart);
  prepare_xy_series(chart);
}

dooble_charts_property_editor_xyseries::
~dooble_charts_property_editor_xyseries()
{
}

void dooble_charts_property_editor_xyseries::prepare_xy_series
(dooble_charts *chart)
{
  if(!chart)
    return;

  m_tree->setFirstColumnSpanned(5, m_tree->rootIndex(), true);

  auto item = m_model->item_from_property
    (dooble_charts::Properties::XY_SERIES_X_AXIS, 0);

  if(item && item->parent())
    {
      m_tree->setFirstColumnSpanned
	(dooble_charts::Properties::XY_SERIES_X_AXIS,
	 item->parent()->index(),
	 true);
    }

  item = m_model->item_from_property
    (dooble_charts::Properties::XY_SERIES_Y_AXIS, 0);

  if(item && item->parent())
    {
      m_tree->setFirstColumnSpanned
	(dooble_charts::Properties::XY_SERIES_Y_AXIS,
	 item->parent()->index(),
	 true);
    }
}
