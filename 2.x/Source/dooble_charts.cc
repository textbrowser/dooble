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

#include "dooble_charts.h"

#include <QMetaType>
#ifdef DOOBLE_QTCHARTS_PRESENT
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
Q_DECLARE_METATYPE(QChart::AnimationOptions)
#endif

const QString dooble_charts::s_generic_properties_strings[] =
  {
   tr("Animation Duration"),
   tr("Animation Options"),
   tr("Background Color"),
   tr("Background Roundness"),
   tr("Background Visible"),
   tr("Chart Type"),
   tr("Data Source Type"),
   tr("Drop Shadow Enabled"),
   tr("Legend Visible"),
   tr("Locale"),
   tr("Localize Numbers"),
   tr("Margins"),
   tr("Plot Area Background Visible"),
   tr("Theme"),
   tr("Title"),
   tr("Title Color"),
   tr("Title Font"),
   tr("X-Axis Range"),
   tr("X-Axis Title"),
   tr("Y-Axis Range"),
   tr("Y-Axis Title"),
   QString(""),
  };

dooble_charts::dooble_charts(QWidget *parent):QWidget(parent)
{
#ifdef DOOBLE_QTCHARTS_PRESENT
  m_chart = new QChart();
  m_chart->addAxis(m_x_axis = new QValueAxis(this), Qt::AlignBottom);
  m_chart->addAxis(m_y_axis = new QValueAxis(this), Qt::AlignLeft);
  m_chart->addSeries(m_series = new QLineSeries(this));
  m_chart_view = new QChartView(m_chart);
  m_series->attachAxis(m_x_axis);
  m_series->attachAxis(m_y_axis);
#endif
  m_property_editor = nullptr;
  m_ui.setupUi(this);
  m_ui.splitter->setStretchFactor(0, 1);
  m_ui.splitter->setStretchFactor(1, 0);
#ifdef DOOBLE_QTCHARTS_PRESENT
  m_ui.charts_frame->layout()->addWidget(m_chart_view);
#endif
}

dooble_charts::~dooble_charts()
{
#ifdef DOOBLE_QTCHARTS_PRESENT
  m_chart->deleteLater();
#endif
}

QHash<dooble_charts::Properties, QVariant> dooble_charts::
properties(void) const
{
  QHash<dooble_charts::Properties, QVariant> properties;

#ifdef DOOBLE_QTCHARTS_PRESENT
  properties[dooble_charts::Properties::ANIMATION_DURATION] = m_chart->
    animationDuration();
  properties[dooble_charts::Properties::ANIMATION_OPTIONS] =
    QVariant::fromValue(m_chart->animationOptions());
  properties[dooble_charts::Properties::BACKGROUND_COLOR] = m_chart->
    backgroundBrush().color();
  properties[dooble_charts::Properties::BACKGROUND_ROUNDNESS] = m_chart->
    backgroundRoundness();
  properties[dooble_charts::Properties::BACKGROUND_VISIBLE] = m_chart->
    isBackgroundVisible();
  properties[dooble_charts::Properties::CHART_TYPE] = dooble_charts::
    chart_type_to_string(m_chart->chartType());
  properties[dooble_charts::Properties::DATA_SOURCE_TYPE] = tr("File");
  properties[dooble_charts::Properties::DROP_SHADOW_ENABLED] = m_chart->
    isDropShadowEnabled();
  properties[dooble_charts::Properties::LEGEND_VISIBLE] = m_chart->
    legend()->isVisible();
  properties[dooble_charts::Properties::LOCALE] = m_chart->locale().name();
  properties[dooble_charts::Properties::LOCALIZE_NUMBERS] = m_chart->
    localizeNumbers();
  properties[dooble_charts::Properties::MARGINS] = QString("%1, %2, %3, %4").
    arg(m_chart->margins().bottom()).
    arg(m_chart->margins().left()).
    arg(m_chart->margins().right()).
    arg(m_chart->margins().top());
  properties[dooble_charts::Properties::PLOT_AREA_BACKGROUND_VISIBLE] =
    m_chart->isPlotAreaBackgroundVisible();
  properties[dooble_charts::Properties::THEME] = m_chart->theme();
  properties[dooble_charts::Properties::TITLE] = m_chart->title();
  properties[dooble_charts::Properties::TITLE_COLOR] =
    m_chart->titleBrush().color();
  properties[dooble_charts::Properties::TITLE_FONT] = m_chart->titleFont();
  properties[dooble_charts::Properties::X_AXIS_RANGE] = QString("[%1, %2]").
    arg(m_x_axis->min()).arg(m_x_axis->max());
  properties[dooble_charts::Properties::Y_AXIS_RANGE] = QString("[%1, %2]").
    arg(m_y_axis->min()).arg(m_y_axis->max());
#endif
  return properties;
}

#ifdef DOOBLE_QTCHARTS_PRESENT
QString dooble_charts::chart_type_to_string(const QChart::ChartType chart_type)
{
  switch(chart_type)
    {
    case QChart::ChartTypeCartesian:
      {
	return tr("Cartesian");
      }
    case QChart::ChartTypePolar:
      {
	return tr("Polar");
      }
    default:
      {
	return tr("Undefined");
      }
    }
}
#endif
