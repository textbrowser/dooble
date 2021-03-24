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

const QString dooble_charts::s_data_properties_strings[] =
  {
   tr("Extraction Script"),
   tr("Source Address"),
   tr("Source Type"),
   QString("")
  };

const QString dooble_charts::s_chart_properties_strings[] =
  {
   tr("Animation Duration"),
   tr("Animation Options"),
   tr("Background Color"),
   tr("Background Roundness"),
   tr("Background Visible"),
   tr("Chart Type"),
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

#ifdef DOOBLE_QTCHARTS_PRESENT
QChart::ChartTheme dooble_charts::string_to_chart_theme(const QString &t)
{
  QString text(t.toLower().trimmed());

  if(text == tr("blue cerulean"))
    return QChart::ChartThemeBlueCerulean;
  else if(text == tr("blue icy"))
    return QChart::ChartThemeBlueIcy;
  else if(text == tr("blue ncs"))
    return QChart::ChartThemeBlueNcs;
  else if(text == tr("brown sand"))
    return QChart::ChartThemeBrownSand;
  else if(text == tr("dark"))
    return QChart::ChartThemeDark;
  else if(text == tr("high contrast"))
    return QChart::ChartThemeHighContrast;
  else if(text == tr("light"))
    return QChart::ChartThemeLight;
  else
    return QChart::ChartThemeQt;
}
#endif

QHash<dooble_charts::Properties, QVariant> dooble_charts::
properties(void) const
{
  QHash<dooble_charts::Properties, QVariant> properties;

#ifdef DOOBLE_QTCHARTS_PRESENT
  properties[dooble_charts::Properties::CHART_ANIMATION_DURATION] = m_chart->
    animationDuration();
  properties[dooble_charts::Properties::CHART_ANIMATION_OPTIONS] =
    QVariant::fromValue(m_chart->animationOptions());
  properties[dooble_charts::Properties::CHART_BACKGROUND_COLOR] = m_chart->
    backgroundBrush().color();
  properties[dooble_charts::Properties::CHART_BACKGROUND_ROUNDNESS] = m_chart->
    backgroundRoundness();
  properties[dooble_charts::Properties::CHART_BACKGROUND_VISIBLE] = m_chart->
    isBackgroundVisible();
  properties[dooble_charts::Properties::CHART_CHART_TYPE] =
    chart_type_to_string(m_chart->chartType());
  properties[dooble_charts::Properties::CHART_DROP_SHADOW_ENABLED] = m_chart->
    isDropShadowEnabled();
  properties[dooble_charts::Properties::CHART_LEGEND_VISIBLE] = m_chart->
    legend()->isVisible();
  properties[dooble_charts::Properties::CHART_LOCALE] =
    m_chart->locale().name();
  properties[dooble_charts::Properties::CHART_LOCALIZE_NUMBERS] = m_chart->
    localizeNumbers();
  properties[dooble_charts::Properties::CHART_MARGINS] =
    QString("%1, %2, %3, %4").
    arg(m_chart->margins().bottom()).
    arg(m_chart->margins().left()).
    arg(m_chart->margins().right()).
    arg(m_chart->margins().top());
  properties[dooble_charts::Properties::CHART_PLOT_AREA_BACKGROUND_VISIBLE] =
    m_chart->isPlotAreaBackgroundVisible();
  properties[dooble_charts::Properties::CHART_THEME] =
    chart_theme_to_string(m_chart->theme());
  properties[dooble_charts::Properties::CHART_TITLE] = m_chart->title();
  properties[dooble_charts::Properties::CHART_TITLE_COLOR] =
    m_chart->titleBrush().color();
  properties[dooble_charts::Properties::CHART_TITLE_FONT] =
    m_chart->titleFont();
  properties[dooble_charts::Properties::CHART_X_AXIS_RANGE] =
    QString("[%1, %2]").arg(m_x_axis->min()).arg(m_x_axis->max());
  properties[dooble_charts::Properties::CHART_Y_AXIS_RANGE] =
    QString("[%1, %2]").arg(m_y_axis->min()).arg(m_y_axis->max());
  properties[dooble_charts::Properties::DATA_SOURCE_TYPE] = tr("File");
#endif
  return properties;
}

#ifdef DOOBLE_QTCHARTS_PRESENT
QString dooble_charts::chart_theme_to_string
(const QChart::ChartTheme chart_theme)
{
  switch(chart_theme)
    {
    case QChart::ChartThemeBlueCerulean:
      {
	return tr("Blue Cerulean");
      }
    case QChart::ChartThemeBlueIcy:
      {
	return tr("Blue Icy");
      }
    case QChart::ChartThemeBlueNcs:
      {
	return tr("Blue NCS");
      }
    case QChart::ChartThemeBrownSand:
      {
	return tr("Brown Sand");
      }
    case QChart::ChartThemeDark:
      {
	return tr("Dark");
      }
    case QChart::ChartThemeHighContrast:
      {
	return tr("High Contrast");
      }
    case QChart::ChartThemeLight:
      {
	return tr("Light");
      }
    default:
      {
	return tr("Qt");
      }
    }
}
#endif

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

void dooble_charts::slot_item_changed(QStandardItem *item)
{
  if(!item)
    return;

#ifdef DOOBLE_QTCHARTS_PRESENT
  auto property = dooble_charts::Properties
    (item->data(Qt::ItemDataRole(Qt::UserRole + 1)).toInt());

  switch(property)
    {
    case dooble_charts::CHART_ANIMATION_DURATION:
      {
	m_chart->setAnimationDuration(item->text().toInt());
	break;
      }
    case dooble_charts::CHART_BACKGROUND_COLOR:
      {
	QBrush brush(m_chart->backgroundBrush());

	brush.setColor(QColor(item->text()));
	m_chart->setBackgroundBrush(brush);
	break;
      }
    case dooble_charts::CHART_BACKGROUND_ROUNDNESS:
      {
	m_chart->setBackgroundRoundness(QVariant(item->text()).toReal());
	break;
      }
    case dooble_charts::CHART_BACKGROUND_VISIBLE:
      {
	m_chart->setBackgroundVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::CHART_CHART_TYPE:
      {
	break;
      }
    case dooble_charts::CHART_DROP_SHADOW_ENABLED:
      {
	m_chart->setDropShadowEnabled(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::CHART_LEGEND_VISIBLE:
      {
	m_chart->legend()->setVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::CHART_LOCALIZE_NUMBERS:
      {
	m_chart->setLocalizeNumbers(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::CHART_PLOT_AREA_BACKGROUND_VISIBLE:
      {
	m_chart->setPlotAreaBackgroundVisible
	  (item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::CHART_THEME:
      {
	m_chart->setTheme(string_to_chart_theme(item->text()));
	break;
      }
    default:
      {
	break;
      }
    }
#endif
}
