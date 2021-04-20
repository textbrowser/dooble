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

const QString dooble_charts_xyseries::s_axis_properties_strings[] =
  {
   tr("Labels Format"),
   tr("Maximum"),
   tr("Minimum"),
   tr("Minor Tick Count"),
   tr("Tick Anchor"),
   tr("Tick Count"),
   tr("Tick Interval"),
   tr("Tick Type"),
   QString("")
  };

dooble_charts_xyseries::dooble_charts_xyseries(QWidget *parent):
  dooble_charts(parent)
{
#ifdef DOOBLE_QTCHARTS_PRESENT
  m_chart->addAxis(m_x_axis = new QValueAxis(this), Qt::AlignBottom);
  m_chart->addAxis(m_y_axis = new QValueAxis(this), Qt::AlignLeft);
  m_chart->addSeries(m_series = new QLineSeries(this));
#endif
  m_property_editor = new dooble_charts_property_editor_xyseries
    (m_ui.properties, this);
#ifdef DOOBLE_QTCHARTS_PRESENT
  m_series->attachAxis(m_x_axis);
  m_series->attachAxis(m_y_axis);
#endif
  connect(m_property_editor->model(),
	  SIGNAL(itemChanged(QStandardItem *)),
	  this,
	  SLOT(slot_item_changed(QStandardItem *)));
}

dooble_charts_xyseries::~dooble_charts_xyseries()
{
}

void dooble_charts_xyseries::slot_item_changed(QStandardItem *item)
{
  if(!item)
    return;

  dooble_charts::slot_item_changed(item);
}
