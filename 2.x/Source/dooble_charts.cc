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
#include "dooble_charts_property_editor.h"
#include "dooble_database_utilities.h"
#include "dooble_settings.h"
#include "dooble_ui_utilities.h"

#include <QDir>
#include <QMetaType>
#include <QSqlQuery>
#ifdef DOOBLE_QTCHARTS_PRESENT
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#endif

const QString dooble_charts::s_axis_properties_strings[] =
  {
   tr("Alignment Horizontal"),
   tr("Alignment Vertical"),
   tr("Color"),
   tr("Grid Line Color"),
   tr("Grid Visible"),
   tr("Labels Angle"),
   tr("Labels Color"),
   tr("Labels Font"),
   tr("Labels Visible"),
   tr("Line Visible"),
   tr("Minor Grid Line Color"),
   tr("Minor Grid Line Visible"),
   tr("Orientation"),
   tr("Reverse"),
   tr("Shades Border Color"),
   tr("Shades Color"),
   tr("Shades Visible"),
   tr("Title Color"),
   tr("Title Font"),
   tr("Title Text"),
   tr("Title Visible"),
   tr("Visible"),
   ""
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
   tr("Locale"),
   tr("Localize Numbers"),
   tr("Margins"),
   tr("Bottom"),
   tr("Left"),
   tr("Right"),
   tr("Top"),
   tr("Name"),
   tr("Plot Area Background Visible"),
   tr("Theme"),
   tr("Title"),
   tr("Title Color"),
   tr("Title Font"),
   "",
  };

const QString dooble_charts::s_data_properties_strings[] =
  {
   tr("Extraction Script"),
   tr("Source Address"),
   tr("Source Read Buffer Size"),
   tr("Source Read Rate"),
   tr("Source Type"),
   ""
  };

const QString dooble_charts::s_legend_properties_strings[] =
  {
   tr("Alignment"),
   tr("Background Visible"),
   tr("Border Color"),
   tr("Color"),
   tr("Font"),
   tr("Label Color"),
   tr("Marker Shape"),
   tr("Reverse Markers"),
   tr("Show Tool Tips"),
   tr("Visible"),
   ""
  };

dooble_charts::dooble_charts(QWidget *parent):QWidget(parent)
{
#ifdef DOOBLE_QTCHARTS_PRESENT
  m_chart = new QChart();
  m_chart_view = new QChartView(m_chart);
  m_legend = m_chart->legend();
#endif
  m_menu = nullptr;
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
QChart::AnimationOptions dooble_charts::string_to_chart_animation_options
(const QString &t)
{
  auto text(t.trimmed());

  if(text == tr("All"))
    return QChart::AllAnimations;
  else if(text == tr("Grid Axis"))
    return QChart::GridAxisAnimations;
  else if(text == tr("Series"))
    return QChart::SeriesAnimations;
  else
    return QChart::NoAnimation;
}

QChart::ChartTheme dooble_charts::string_to_chart_theme(const QString &t)
{
  auto text(t.trimmed());

  if(text == tr("Blue Cerulean"))
    return QChart::ChartThemeBlueCerulean;
  else if(text == tr("Blue Icy"))
    return QChart::ChartThemeBlueIcy;
  else if(text == tr("Blue NCS"))
    return QChart::ChartThemeBlueNcs;
  else if(text == tr("Brown Sand"))
    return QChart::ChartThemeBrownSand;
  else if(text == tr("Dark"))
    return QChart::ChartThemeDark;
  else if(text == tr("High Contrast"))
    return QChart::ChartThemeHighContrast;
  else if(text == tr("Light"))
    return QChart::ChartThemeLight;
  else
    return QChart::ChartThemeQt;
}
#endif

QHash<QString, QVariant> dooble_charts::
data_properties_for_database(void) const
{
  QHash<QString, QVariant> hash;
  QHashIterator<dooble_charts::Properties, QVariant> it(data_properties());

  while(it.hasNext())
    {
      it.next();

      auto property(property_to_name(it.key()));

      if(!property.isEmpty())
	hash[property] = it.value();
    }

  return hash;
}

QHash<QString, QVariant> dooble_charts::
legend_properties_for_database(void) const
{
  QHash<QString, QVariant> hash;
  QHashIterator<dooble_charts::Properties, QVariant> it(legend_properties());

  while(it.hasNext())
    {
      it.next();

      auto property(property_to_name(it.key()));

      if(!property.isEmpty())
	hash[property] = it.value();
    }

  return hash;
}

QHash<QString, QVariant> dooble_charts::properties_for_database(void) const
{
  QHash<QString, QVariant> hash;
  QHashIterator<dooble_charts::Properties, QVariant> it(properties());

  while(it.hasNext())
    {
      it.next();

      if(!dooble_charts::properties().contains(it.key()))
	/*
	** Ignore properties of derived classes.
	*/

	continue;

      auto property(property_to_name(it.key()));

      if(!property.isEmpty())
	hash[property] = it.value();
    }

  return hash;
}

QHash<QString, QVariant> dooble_charts::
x_axis_properties_for_database(void) const
{
  QHash<QString, QVariant> hash;
  QHashIterator<dooble_charts::Properties, QVariant> it(x_axis_properties());

  while(it.hasNext())
    {
      it.next();

      auto property(property_to_name(it.key()));

      if(!property.isEmpty())
	hash[property] = it.value();
    }

  return hash;
}

QHash<QString, QVariant> dooble_charts::
y_axis_properties_for_database(void) const
{
  QHash<QString, QVariant> hash;
  QHashIterator<dooble_charts::Properties, QVariant> it(y_axis_properties());

  while(it.hasNext())
    {
      it.next();

      auto property(property_to_name(it.key()));

      if(!property.isEmpty())
	hash[property] = it.value();
    }

  return hash;
}

QHash<dooble_charts::Properties, QVariant>
dooble_charts::data_properties(void) const
{
  QHash<dooble_charts::Properties, QVariant> properties;

  if(m_property_editor)
    {
      properties[dooble_charts::Properties::DATA_EXTRACTION_SCRIPT] =
	m_property_editor->property
	(dooble_charts::Properties::DATA_EXTRACTION_SCRIPT);
      properties[dooble_charts::Properties::DATA_SOURCE_ADDRESS] =
	m_property_editor->property
	(dooble_charts::Properties::DATA_SOURCE_ADDRESS);
      properties[dooble_charts::Properties::DATA_SOURCE_READ_BUFFER_SIZE] =
	m_property_editor->property
	(dooble_charts::Properties::DATA_SOURCE_READ_BUFFER_SIZE);
      properties[dooble_charts::Properties::DATA_SOURCE_READ_RATE] =
	m_property_editor->property
	(dooble_charts::Properties::DATA_SOURCE_READ_RATE);
      properties[dooble_charts::Properties::DATA_SOURCE_TYPE] =
	m_property_editor->property
	(dooble_charts::Properties::DATA_SOURCE_TYPE);
    }
  else
    {
      properties[dooble_charts::Properties::DATA_EXTRACTION_SCRIPT] =
	QVariant();
      properties[dooble_charts::Properties::DATA_SOURCE_ADDRESS] = QVariant();
      properties[dooble_charts::Properties::DATA_SOURCE_READ_BUFFER_SIZE] =
	QVariant();
      properties[dooble_charts::Properties::DATA_SOURCE_READ_RATE] =
	QVariant();
      properties[dooble_charts::Properties::DATA_SOURCE_TYPE] = QVariant();
    }

  return properties;
}

QHash<dooble_charts::Properties, QVariant>
dooble_charts::legend_properties(void) const
{
  QHash<dooble_charts::Properties, QVariant> properties;

#ifdef DOOBLE_QTCHARTS_PRESENT
  properties[dooble_charts::Properties::LEGEND_ALIGNMENT] =
    dooble_ui_utilities::alignment_to_string
    (m_legend->alignment());
  properties[dooble_charts::Properties::LEGEND_BACKGROUND_VISIBLE] =
    m_legend->isBackgroundVisible();
  properties[dooble_charts::Properties::LEGEND_BORDER_COLOR] =
    m_legend->borderColor();
  properties[dooble_charts::Properties::LEGEND_COLOR] = m_legend->color();
  properties[dooble_charts::Properties::LEGEND_FONT] = m_legend->font();
  properties[dooble_charts::Properties::LEGEND_LABEL_COLOR] =
    m_legend->labelColor();
  properties[dooble_charts::Properties::LEGEND_MARKER_SHAPE] =
    legend_marker_shape_to_string(m_legend->markerShape());
  properties[dooble_charts::Properties::LEGEND_REVERSE_MARKERS] =
    m_legend->reverseMarkers();
  properties[dooble_charts::Properties::LEGEND_SHOW_TOOL_TIPS] =
    m_legend->showToolTips();
  properties[dooble_charts::Properties::LEGEND_VISIBLE] =
    m_legend->isVisible();
#endif
  return properties;
}

QHash<dooble_charts::Properties, QVariant> dooble_charts::
properties(void) const
{
  QHash<dooble_charts::Properties, QVariant> properties;

#ifdef DOOBLE_QTCHARTS_PRESENT
  properties[dooble_charts::Properties::CHART_ANIMATION_DURATION] = m_chart->
    animationDuration();
  properties[dooble_charts::Properties::CHART_ANIMATION_OPTIONS] =
    chart_animation_option_to_string(m_chart->animationOptions());
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
  properties[dooble_charts::Properties::CHART_LOCALE] =
    m_chart->locale().name();
  properties[dooble_charts::Properties::CHART_LOCALIZE_NUMBERS] = m_chart->
    localizeNumbers();
  properties[dooble_charts::Properties::CHART_MARGINS_BOTTOM] = m_chart->
    margins().bottom();
  properties[dooble_charts::Properties::CHART_MARGINS_LEFT] = m_chart->
    margins().left();
  properties[dooble_charts::Properties::CHART_MARGINS_RIGHT] = m_chart->
    margins().right();
  properties[dooble_charts::Properties::CHART_MARGINS_TOP] = m_chart->
    margins().top();

  if(m_property_editor)
    properties[dooble_charts::Properties::CHART_NAME] = m_property_editor->
      property(dooble_charts::Properties::CHART_NAME);

  properties[dooble_charts::Properties::CHART_PLOT_AREA_BACKGROUND_VISIBLE] =
    m_chart->isPlotAreaBackgroundVisible();
  properties[dooble_charts::Properties::CHART_THEME] =
    chart_theme_to_string(m_chart->theme());
  properties[dooble_charts::Properties::CHART_TITLE] = m_chart->title();
  properties[dooble_charts::Properties::CHART_TITLE_COLOR] =
    m_chart->titleBrush().color();
  properties[dooble_charts::Properties::CHART_TITLE_FONT] =
    m_chart->titleFont();
  properties[dooble_charts::Properties::DATA_SOURCE_TYPE] = tr("File");
#endif
  return properties;
}

QHash<dooble_charts::Properties, QVariant> dooble_charts::
x_axis_properties(void) const
{
  QHash<dooble_charts::Properties, QVariant> properties;

#ifdef DOOBLE_QTCHARTS_PRESENT
  if(!m_x_axis)
    return properties;

  properties[dooble_charts::Properties::CHART_AXIS_X_ALIGNMENT_HORIZONTAL] =
    dooble_ui_utilities::alignment_to_string
    (Qt::Alignment(Qt::AlignHorizontal_Mask & m_x_axis->alignment()));
  properties[dooble_charts::Properties::CHART_AXIS_X_ALIGNMENT_VERTICAL] =
    dooble_ui_utilities::alignment_to_string
    (Qt::Alignment(Qt::AlignVertical_Mask & m_x_axis->alignment()));
  properties[dooble_charts::Properties::CHART_AXIS_X_COLOR] = m_x_axis->
    linePenColor();
  properties[dooble_charts::Properties::CHART_AXIS_X_GRID_LINE_COLOR] =
    m_x_axis->gridLineColor();
  properties[dooble_charts::Properties::CHART_AXIS_X_GRID_VISIBLE] = m_x_axis->
    isGridLineVisible();
  properties[dooble_charts::Properties::CHART_AXIS_X_LABELS_ANGLE] = m_x_axis->
    labelsAngle();
  properties[dooble_charts::Properties::CHART_AXIS_X_LABELS_COLOR] = m_x_axis->
    labelsColor();
  properties[dooble_charts::Properties::CHART_AXIS_X_LABELS_FONT] = m_x_axis->
    labelsFont();
  properties[dooble_charts::Properties::CHART_AXIS_X_LABELS_VISIBLE] =
    m_x_axis->labelsVisible();
  properties[dooble_charts::Properties::CHART_AXIS_X_LINE_VISIBLE] = m_x_axis->
    isLineVisible();
  properties[dooble_charts::Properties::CHART_AXIS_X_MINOR_GRID_LINE_COLOR] =
    m_x_axis->minorGridLineColor();
  properties[dooble_charts::Properties::CHART_AXIS_X_MINOR_GRID_LINE_VISIBLE] =
    m_x_axis->isMinorGridLineVisible();
  properties[dooble_charts::Properties::CHART_AXIS_X_ORIENTATION] =
    dooble_ui_utilities::orientation_to_string(m_x_axis->orientation());
  properties[dooble_charts::Properties::CHART_AXIS_X_REVERSE] = m_x_axis->
    isReverse();
  properties[dooble_charts::Properties::CHART_AXIS_X_SHADES_BORDER_COLOR] =
    m_x_axis->shadesBorderColor();
  properties[dooble_charts::Properties::CHART_AXIS_X_SHADES_COLOR] = m_x_axis->
    shadesColor();
  properties[dooble_charts::Properties::CHART_AXIS_X_SHADES_VISIBLE] =
    m_x_axis->shadesVisible();
  properties[dooble_charts::Properties::CHART_AXIS_X_TITLE_COLOR] = m_x_axis->
    titleBrush().color();
  properties[dooble_charts::Properties::CHART_AXIS_X_TITLE_FONT] = m_x_axis->
    titleFont();
  properties[dooble_charts::Properties::CHART_AXIS_X_TITLE_TEXT] = m_x_axis->
    titleText();
  properties[dooble_charts::Properties::CHART_AXIS_X_TITLE_VISIBLE] =
    m_x_axis->isTitleVisible();
  properties[dooble_charts::Properties::CHART_AXIS_X_VISIBLE] = m_x_axis->
    isVisible();
#endif
  return properties;
}

QHash<dooble_charts::Properties, QVariant> dooble_charts::
y_axis_properties(void) const
{
  QHash<dooble_charts::Properties, QVariant> properties;

#ifdef DOOBLE_QTCHARTS_PRESENT
  if(!m_y_axis)
    return properties;

  properties[dooble_charts::Properties::CHART_AXIS_Y_ALIGNMENT_HORIZONTAL] =
    dooble_ui_utilities::alignment_to_string
    (Qt::Alignment(Qt::AlignHorizontal_Mask & m_y_axis->alignment()));
  properties[dooble_charts::Properties::CHART_AXIS_Y_ALIGNMENT_VERTICAL] =
    dooble_ui_utilities::alignment_to_string
    (Qt::Alignment(Qt::AlignVertical_Mask & m_y_axis->alignment()));
  properties[dooble_charts::Properties::CHART_AXIS_Y_COLOR] = m_y_axis->
    linePenColor();
  properties[dooble_charts::Properties::CHART_AXIS_Y_GRID_LINE_COLOR] =
    m_y_axis->gridLineColor();
  properties[dooble_charts::Properties::CHART_AXIS_Y_GRID_VISIBLE] = m_y_axis->
    isGridLineVisible();
  properties[dooble_charts::Properties::CHART_AXIS_Y_LABELS_ANGLE] = m_y_axis->
    labelsAngle();
  properties[dooble_charts::Properties::CHART_AXIS_Y_LABELS_COLOR] = m_y_axis->
    labelsColor();
  properties[dooble_charts::Properties::CHART_AXIS_Y_LABELS_FONT] = m_y_axis->
    labelsFont();
  properties[dooble_charts::Properties::CHART_AXIS_Y_LABELS_VISIBLE] =
    m_y_axis->labelsVisible();
  properties[dooble_charts::Properties::CHART_AXIS_Y_LINE_VISIBLE] = m_y_axis->
    isLineVisible();
  properties[dooble_charts::Properties::CHART_AXIS_Y_MINOR_GRID_LINE_COLOR] =
    m_y_axis->minorGridLineColor();
  properties[dooble_charts::Properties::CHART_AXIS_Y_MINOR_GRID_LINE_VISIBLE] =
    m_y_axis->isMinorGridLineVisible();
  properties[dooble_charts::Properties::CHART_AXIS_Y_ORIENTATION] =
    dooble_ui_utilities::orientation_to_string(m_y_axis->orientation());
  properties[dooble_charts::Properties::CHART_AXIS_Y_REVERSE] = m_y_axis->
    isReverse();
  properties[dooble_charts::Properties::CHART_AXIS_Y_SHADES_BORDER_COLOR] =
    m_y_axis->shadesBorderColor();
  properties[dooble_charts::Properties::CHART_AXIS_Y_SHADES_COLOR] = m_y_axis->
    shadesColor();
  properties[dooble_charts::Properties::CHART_AXIS_Y_SHADES_VISIBLE] =
    m_y_axis->shadesVisible();
  properties[dooble_charts::Properties::CHART_AXIS_Y_TITLE_COLOR] = m_y_axis->
    titleBrush().color();
  properties[dooble_charts::Properties::CHART_AXIS_Y_TITLE_FONT] = m_y_axis->
    titleFont();
  properties[dooble_charts::Properties::CHART_AXIS_Y_TITLE_TEXT] = m_y_axis->
    titleText();
  properties[dooble_charts::Properties::CHART_AXIS_Y_TITLE_VISIBLE] =
    m_y_axis->isTitleVisible();
  properties[dooble_charts::Properties::CHART_AXIS_Y_VISIBLE] = m_y_axis->
    isVisible();
#endif
  return properties;
}

#ifdef DOOBLE_QTCHARTS_PRESENT
QLegend::MarkerShape dooble_charts::string_to_legend_marker_shape
(const QString &t)
{
  auto text(t.trimmed());

  if(text == tr("Circle"))
    return QLegend::MarkerShapeCircle;
  else if(text == tr("From Series"))
    return QLegend::MarkerShapeFromSeries;
  else if(text == tr("Rectangle"))
    return QLegend::MarkerShapeRectangle;
  else
    return QLegend::MarkerShapeDefault;
}
#endif

QMenu *dooble_charts::menu(void)
{
  return m_menu;
}

#ifdef DOOBLE_QTCHARTS_PRESENT
QString dooble_charts::chart_animation_option_to_string
(const QChart::AnimationOptions chart_animation_options)
{
  switch(chart_animation_options)
    {
    case QChart::AllAnimations:
      {
	return tr("All");
      }
    case QChart::GridAxisAnimations:
      {
	return tr("Grid Axis");
      }
    case QChart::SeriesAnimations:
      {
	return tr("Series");
      }
    default:
      {
	return tr("None");
      }
    }
}

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

QString dooble_charts::legend_marker_shape_to_string
(const QLegend::MarkerShape marker_shape)
{
  switch(marker_shape)
    {
    case QLegend::MarkerShapeCircle:
      {
	return tr("Circle");
      }
    case QLegend::MarkerShapeFromSeries:
      {
	return tr("From Series");
      }
    case QLegend::MarkerShapeRectangle:
      {
	return tr("Rectangle");
      }
    default:
      {
	return tr("Default");
      }
    }
}
#endif

QString dooble_charts::property_to_name
(const dooble_charts::Properties property) const
{
  switch(property)
    {
    case dooble_charts::Properties::CHART_ANIMATION_DURATION:
    case dooble_charts::Properties::CHART_ANIMATION_OPTIONS:
    case dooble_charts::Properties::CHART_BACKGROUND_COLOR:
    case dooble_charts::Properties::CHART_BACKGROUND_ROUNDNESS:
    case dooble_charts::Properties::CHART_BACKGROUND_VISIBLE:
    case dooble_charts::Properties::CHART_CHART_TYPE:
    case dooble_charts::Properties::CHART_DROP_SHADOW_ENABLED:
    case dooble_charts::Properties::CHART_LOCALE:
    case dooble_charts::Properties::CHART_LOCALIZE_NUMBERS:
    case dooble_charts::Properties::CHART_MARGINS_BOTTOM:
    case dooble_charts::Properties::CHART_MARGINS_LEFT:
    case dooble_charts::Properties::CHART_MARGINS_RIGHT:
    case dooble_charts::Properties::CHART_MARGINS_TOP:
    case dooble_charts::Properties::CHART_NAME:
    case dooble_charts::Properties::CHART_PLOT_AREA_BACKGROUND_VISIBLE:
    case dooble_charts::Properties::CHART_THEME:
    case dooble_charts::Properties::CHART_TITLE:
    case dooble_charts::Properties::CHART_TITLE_COLOR:
    case dooble_charts::Properties::CHART_TITLE_FONT:
      {
	return s_chart_properties_strings[property];
      }
    case dooble_charts::Properties::CHART_AXIS_X_ALIGNMENT_HORIZONTAL:
    case dooble_charts::Properties::CHART_AXIS_X_ALIGNMENT_VERTICAL:
    case dooble_charts::Properties::CHART_AXIS_X_COLOR:
    case dooble_charts::Properties::CHART_AXIS_X_GRID_LINE_COLOR:
    case dooble_charts::Properties::CHART_AXIS_X_GRID_VISIBLE:
    case dooble_charts::Properties::CHART_AXIS_X_LABELS_ANGLE:
    case dooble_charts::Properties::CHART_AXIS_X_LABELS_COLOR:
    case dooble_charts::Properties::CHART_AXIS_X_LABELS_FONT:
    case dooble_charts::Properties::CHART_AXIS_X_LABELS_VISIBLE:
    case dooble_charts::Properties::CHART_AXIS_X_LINE_VISIBLE:
    case dooble_charts::Properties::CHART_AXIS_X_MINOR_GRID_LINE_COLOR:
    case dooble_charts::Properties::CHART_AXIS_X_MINOR_GRID_LINE_VISIBLE:
    case dooble_charts::Properties::CHART_AXIS_X_ORIENTATION:
    case dooble_charts::Properties::CHART_AXIS_X_REVERSE:
    case dooble_charts::Properties::CHART_AXIS_X_SHADES_BORDER_COLOR:
    case dooble_charts::Properties::CHART_AXIS_X_SHADES_COLOR:
    case dooble_charts::Properties::CHART_AXIS_X_SHADES_VISIBLE:
    case dooble_charts::Properties::CHART_AXIS_X_TITLE_COLOR:
    case dooble_charts::Properties::CHART_AXIS_X_TITLE_FONT:
    case dooble_charts::Properties::CHART_AXIS_X_TITLE_TEXT:
    case dooble_charts::Properties::CHART_AXIS_X_TITLE_VISIBLE:
    case dooble_charts::Properties::CHART_AXIS_X_VISIBLE:
      {
	return s_axis_properties_strings
	  [property - dooble_charts::Properties::CHART_TITLE_FONT - 1];
      }
    case dooble_charts::Properties::CHART_AXIS_Y_ALIGNMENT_HORIZONTAL:
    case dooble_charts::Properties::CHART_AXIS_Y_ALIGNMENT_VERTICAL:
    case dooble_charts::Properties::CHART_AXIS_Y_COLOR:
    case dooble_charts::Properties::CHART_AXIS_Y_GRID_LINE_COLOR:
    case dooble_charts::Properties::CHART_AXIS_Y_GRID_VISIBLE:
    case dooble_charts::Properties::CHART_AXIS_Y_LABELS_ANGLE:
    case dooble_charts::Properties::CHART_AXIS_Y_LABELS_COLOR:
    case dooble_charts::Properties::CHART_AXIS_Y_LABELS_FONT:
    case dooble_charts::Properties::CHART_AXIS_Y_LABELS_VISIBLE:
    case dooble_charts::Properties::CHART_AXIS_Y_LINE_VISIBLE:
    case dooble_charts::Properties::CHART_AXIS_Y_MINOR_GRID_LINE_COLOR:
    case dooble_charts::Properties::CHART_AXIS_Y_MINOR_GRID_LINE_VISIBLE:
    case dooble_charts::Properties::CHART_AXIS_Y_ORIENTATION:
    case dooble_charts::Properties::CHART_AXIS_Y_REVERSE:
    case dooble_charts::Properties::CHART_AXIS_Y_SHADES_BORDER_COLOR:
    case dooble_charts::Properties::CHART_AXIS_Y_SHADES_COLOR:
    case dooble_charts::Properties::CHART_AXIS_Y_SHADES_VISIBLE:
    case dooble_charts::Properties::CHART_AXIS_Y_TITLE_COLOR:
    case dooble_charts::Properties::CHART_AXIS_Y_TITLE_FONT:
    case dooble_charts::Properties::CHART_AXIS_Y_TITLE_TEXT:
    case dooble_charts::Properties::CHART_AXIS_Y_TITLE_VISIBLE:
    case dooble_charts::Properties::CHART_AXIS_Y_VISIBLE:
      {
	return s_axis_properties_strings
	  [property - dooble_charts::Properties::CHART_AXIS_X_VISIBLE - 1];
      }
    case dooble_charts::Properties::DATA_EXTRACTION_SCRIPT:
    case dooble_charts::Properties::DATA_SOURCE_ADDRESS:
    case dooble_charts::Properties::DATA_SOURCE_READ_BUFFER_SIZE:
    case dooble_charts::Properties::DATA_SOURCE_READ_RATE:
    case dooble_charts::Properties::DATA_SOURCE_TYPE:
      {
	return s_data_properties_strings
	  [property - dooble_charts::Properties::CHART_AXIS_Y_VISIBLE - 1];
      }
    case dooble_charts::Properties::LEGEND_ALIGNMENT:
    case dooble_charts::Properties::LEGEND_BACKGROUND_VISIBLE:
    case dooble_charts::Properties::LEGEND_BORDER_COLOR:
    case dooble_charts::Properties::LEGEND_COLOR:
    case dooble_charts::Properties::LEGEND_FONT:
    case dooble_charts::Properties::LEGEND_LABEL_COLOR:
    case dooble_charts::Properties::LEGEND_MARKER_SHAPE:
    case dooble_charts::Properties::LEGEND_REVERSE_MARKERS:
    case dooble_charts::Properties::LEGEND_SHOW_TOOL_TIPS:
    case dooble_charts::Properties::LEGEND_VISIBLE:
      {
	return s_legend_properties_strings
	  [property - dooble_charts::Properties::DATA_SOURCE_TYPE - 1];
      }
    default:
      {
	break;
      }
    }

  return "";
}

QWidget *dooble_charts::view(void) const
{
#ifdef DOOBLE_QTCHARTS_PRESENT
  return m_chart_view;
#else
  return nullptr;
#endif
}

void dooble_charts::save(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto database_name(dooble_database_utilities::database_name());

  {
    auto db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_charts.db");

    if(db.open())
      {
	QSqlQuery query(db);
	auto name(properties().value(dooble_charts::Properties::CHART_NAME).
		  toString().toUtf8());

	query.exec("CREATE TABLE IF NOT EXISTS dooble_charts ("
		   "name TEXT NOT NULL, "
		   "property TEXT NOT NULL, "
		   "subset_name TEXT NOT NULL, "
		   "value TEXT, "
		   "PRIMARY KEY (name, property, subset_name))");

	{
	  QHashIterator<QString, QVariant> it
	    (data_properties_for_database());

	  while(it.hasNext())
	    {
	      it.next();
	      query.prepare
		("INSERT OR REPLACE INTO dooble_charts "
		 "(name, property, subset_name, value) "
		 "VALUES (?, ?, ?, ?)");
	      query.addBindValue(name);
	      query.addBindValue(it.key().toUtf8());
	      query.addBindValue("data");
	      query.addBindValue(it.value());
	      query.exec();
	    }
	}

	{
	  QHashIterator<QString, QVariant> it
	    (legend_properties_for_database());

	  while(it.hasNext())
	    {
	      it.next();
	      query.prepare
		("INSERT OR REPLACE INTO dooble_charts "
		 "(name, property, subset_name, value) "
		 "VALUES (?, ?, ?, ?)");
	      query.addBindValue(name);
	      query.addBindValue(it.key().toUtf8());
	      query.addBindValue("legend");
	      query.addBindValue(it.value());
	      query.exec();
	    }
	}

	{
	  QHashIterator<QString, QVariant> it(properties_for_database());

	  while(it.hasNext())
	    {
	      it.next();
	      query.prepare
		("INSERT OR REPLACE INTO dooble_charts "
		 "(name, property, subset_name, value) "
		 "VALUES (?, ?, ?, ?)");
	      query.addBindValue(name);
	      query.addBindValue(it.key().toUtf8());
	      query.addBindValue("properties");
	      query.addBindValue(it.value());
	      query.exec();
	    }
	}

	{
	  QHashIterator<QString, QVariant> it
	    (x_axis_properties_for_database());

	  while(it.hasNext())
	    {
	      it.next();
	      query.prepare
		("INSERT OR REPLACE INTO dooble_charts "
		 "(name, property, subset_name, value) "
		 "VALUES (?, ?, ?, ?)");
	      query.addBindValue(name);
	      query.addBindValue(it.key().toUtf8());
	      query.addBindValue("x_axis_properties");
	      query.addBindValue(it.value());
	      query.exec();
	    }
	}

	{
	  QHashIterator<QString, QVariant> it
	    (y_axis_properties_for_database());

	  while(it.hasNext())
	    {
	      it.next();
	      query.prepare
		("INSERT OR REPLACE INTO dooble_charts "
		 "(name, property, subset_name, value) "
		 "VALUES (?, ?, ?, ?)");
	      query.addBindValue(name);
	      query.addBindValue(it.key().toUtf8());
	      query.addBindValue("y_axis_properties");
	      query.addBindValue(it.value());
	      query.exec();
	    }
	}
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}

void dooble_charts::slot_item_changed(QStandardItem *item)
{
  if(!item)
    return;

#ifdef DOOBLE_QTCHARTS_PRESENT
  if(!m_x_axis || !m_y_axis)
    return;

  auto property = dooble_charts::Properties
    (item->data(Qt::ItemDataRole(Qt::UserRole + 1)).toInt());

  switch(property)
    {
    case dooble_charts::Properties::CHART_ANIMATION_DURATION:
      {
	m_chart->setAnimationDuration(item->text().toInt());
	break;
      }
    case dooble_charts::Properties::CHART_ANIMATION_OPTIONS:
      {
	m_chart->setAnimationOptions
	  (string_to_chart_animation_options(item->text()));
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_COLOR:
      {
	m_x_axis->setLinePenColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_GRID_LINE_COLOR:
      {
	m_x_axis->setGridLineColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_GRID_VISIBLE:
      {
	m_x_axis->setGridLineVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_LABELS_ANGLE:
      {
	m_x_axis->setLabelsAngle(item->text().toInt());
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_LABELS_COLOR:
      {
	m_x_axis->setLabelsColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_LABELS_FONT:
      {
	QFont font;

	if(font.fromString(item->text()))
	  m_x_axis->setLabelsFont(font);

	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_LABELS_VISIBLE:
      {
	m_x_axis->setLabelsVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_LINE_VISIBLE:
      {
	m_x_axis->setLineVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_MINOR_GRID_LINE_COLOR:
      {
	m_x_axis->setMinorGridLineColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_MINOR_GRID_LINE_VISIBLE:
      {
	m_x_axis->setMinorGridLineVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_REVERSE:
      {
	m_x_axis->setReverse(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_SHADES_BORDER_COLOR:
      {
	m_x_axis->setShadesBorderColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_SHADES_COLOR:
      {
	m_x_axis->setShadesColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_SHADES_VISIBLE:
      {
	m_x_axis->setShadesVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_TITLE_COLOR:
      {
	auto brush(m_chart->titleBrush());

	brush.setColor(QColor(item->text()));
	m_x_axis->setTitleBrush(brush);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_TITLE_FONT:
      {
	QFont font;

	if(font.fromString(item->text()))
	  m_x_axis->setTitleFont(font);

	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_TITLE_TEXT:
      {
	m_x_axis->setTitleText(item->text());
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_TITLE_VISIBLE:
      {
	m_x_axis->setTitleVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_X_VISIBLE:
      {
	m_x_axis->setVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_COLOR:
      {
	m_y_axis->setLinePenColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_GRID_LINE_COLOR:
      {
	m_y_axis->setGridLineColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_GRID_VISIBLE:
      {
	m_y_axis->setGridLineVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_LABELS_ANGLE:
      {
	m_y_axis->setLabelsAngle(item->text().toInt());
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_LABELS_COLOR:
      {
	m_y_axis->setLabelsColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_LABELS_FONT:
      {
	QFont font;

	if(font.fromString(item->text()))
	  m_y_axis->setLabelsFont(font);

	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_LABELS_VISIBLE:
      {
	m_y_axis->setLabelsVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_LINE_VISIBLE:
      {
	m_y_axis->setLineVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_MINOR_GRID_LINE_COLOR:
      {
	m_y_axis->setMinorGridLineColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_MINOR_GRID_LINE_VISIBLE:
      {
	m_y_axis->setMinorGridLineVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_REVERSE:
      {
	m_y_axis->setReverse(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_SHADES_BORDER_COLOR:
      {
	m_y_axis->setShadesBorderColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_SHADES_COLOR:
      {
	m_y_axis->setShadesColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_SHADES_VISIBLE:
      {
	m_y_axis->setShadesVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_TITLE_COLOR:
      {
	auto brush(m_chart->titleBrush());

	brush.setColor(QColor(item->text()));
	m_y_axis->setTitleBrush(brush);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_TITLE_FONT:
      {
	QFont font;

	if(font.fromString(item->text()))
	  m_y_axis->setTitleFont(font);

	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_TITLE_TEXT:
      {
	m_y_axis->setTitleText(item->text());
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_TITLE_VISIBLE:
      {
	m_y_axis->setTitleVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_AXIS_Y_VISIBLE:
      {
	m_y_axis->setVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_BACKGROUND_COLOR:
      {
	auto brush(m_chart->backgroundBrush());

	brush.setColor(QColor(item->text()));
	m_chart->setBackgroundBrush(brush);
	break;
      }
    case dooble_charts::Properties::CHART_BACKGROUND_ROUNDNESS:
      {
	m_chart->setBackgroundRoundness(QVariant(item->text()).toReal());
	break;
      }
    case dooble_charts::Properties::CHART_BACKGROUND_VISIBLE:
      {
	m_chart->setBackgroundVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_CHART_TYPE:
      {
	break;
      }
    case dooble_charts::Properties::CHART_DROP_SHADOW_ENABLED:
      {
	m_chart->setDropShadowEnabled(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_LOCALE:
      {
	m_chart->setLocale(QLocale(item->text()));
	break;
      }
    case dooble_charts::Properties::CHART_LOCALIZE_NUMBERS:
      {
	m_chart->setLocalizeNumbers(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_MARGINS_BOTTOM:
      {
	auto margins(m_chart->margins());

	margins.setBottom(item->text().toInt());
	m_chart->setMargins(margins);
	break;
      }
    case dooble_charts::Properties::CHART_MARGINS_LEFT:
      {
	auto margins(m_chart->margins());

	margins.setLeft(item->text().toInt());
	m_chart->setMargins(margins);
	break;
      }
    case dooble_charts::Properties::CHART_MARGINS_RIGHT:
      {
	auto margins(m_chart->margins());

	margins.setRight(item->text().toInt());
	m_chart->setMargins(margins);
	break;
      }
    case dooble_charts::Properties::CHART_MARGINS_TOP:
      {
	auto margins(m_chart->margins());

	margins.setTop(item->text().toInt());
	m_chart->setMargins(margins);
	break;
      }
    case dooble_charts::Properties::CHART_PLOT_AREA_BACKGROUND_VISIBLE:
      {
	m_chart->setPlotAreaBackgroundVisible
	  (item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::CHART_THEME:
      {
	m_chart->setTheme(string_to_chart_theme(item->text()));

	if(m_property_editor)
	  m_chart->setDropShadowEnabled
	    (m_property_editor->
	     property(dooble_charts::Properties::CHART_DROP_SHADOW_ENABLED).
	     toBool());

	break;
      }
    case dooble_charts::Properties::CHART_TITLE:
      {
	m_chart->setTitle(item->text().trimmed());
	break;
      }
    case dooble_charts::Properties::CHART_TITLE_COLOR:
      {
	auto brush(m_chart->titleBrush());

	brush.setColor(QColor(item->text()));
	m_chart->setTitleBrush(brush);
	break;
      }
    case dooble_charts::Properties::CHART_TITLE_FONT:
      {
	QFont font;

	if(font.fromString(item->text()))
	  m_chart->setTitleFont(font);

	break;
      }
    case dooble_charts::Properties::LEGEND_ALIGNMENT:
      {
	m_legend->setAlignment
	  (dooble_ui_utilities::string_to_alignment(item->text()));
	break;
      }
    case dooble_charts::Properties::LEGEND_BACKGROUND_VISIBLE:
      {
	m_legend->setBackgroundVisible(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::LEGEND_BORDER_COLOR:
      {
	m_legend->setBorderColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::LEGEND_COLOR:
      {
	m_legend->setColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::LEGEND_FONT:
      {
	QFont font;

	if(font.fromString(item->text()))
	  m_legend->setFont(font);

	break;
      }
    case dooble_charts::Properties::LEGEND_LABEL_COLOR:
      {
	m_legend->setLabelColor(QColor(item->text()));
	break;
      }
    case dooble_charts::Properties::LEGEND_MARKER_SHAPE:
      {
	m_legend->setMarkerShape(string_to_legend_marker_shape(item->text()));
	break;
      }
    case dooble_charts::Properties::LEGEND_REVERSE_MARKERS:
      {
	m_legend->setReverseMarkers(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::LEGEND_SHOW_TOOL_TIPS:
      {
	m_legend->setShowToolTips(item->checkState() == Qt::Checked);
	break;
      }
    case dooble_charts::Properties::LEGEND_VISIBLE:
      {
	m_chart->legend()->setVisible(item->checkState() == Qt::Checked);
	break;
      }
    default:
      {
	break;
      }
    }
#endif
}
