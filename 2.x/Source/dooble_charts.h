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

#ifndef dooble_charts_h
#define dooble_charts_h

#ifdef DOOBLE_QTCHARTS_PRESENT
#include <QtCharts>
using namespace QtCharts;
#endif

#include "ui_dooble_charts.h"

class dooble_charts_property_editor;

class dooble_charts: public QWidget
{
  Q_OBJECT

 public:
  enum Properties
  {
   /*
   ** Chart Properties
   */

   CHART_ANIMATION_DURATION = 0,
   CHART_ANIMATION_OPTIONS,
   CHART_BACKGROUND_COLOR,
   CHART_BACKGROUND_ROUNDNESS,
   CHART_BACKGROUND_VISIBLE,
   CHART_CHART_TYPE,
   CHART_DROP_SHADOW_ENABLED,
   CHART_LOCALE,
   CHART_LOCALIZE_NUMBERS,
   CHART_MARGINS,
   CHART_MARGINS_BOTTOM,
   CHART_MARGINS_LEFT,
   CHART_MARGINS_RIGHT,
   CHART_MARGINS_TOP,
   CHART_PLOT_AREA_BACKGROUND_VISIBLE,
   CHART_THEME,
   CHART_TITLE,
   CHART_TITLE_COLOR,
   CHART_TITLE_FONT,

   /*
   ** Chart Axis Properties
   */

   CHART_AXIS_ALIGNMENT,
   CHART_AXIS_COLOR,
   CHART_AXIS_GRID_LINE_COLOR,
   CHART_AXIS_GRID_VISIBLE,
   CHART_AXIS_LABELS_ANGLE,
   CHART_AXIS_LABELS_COLOR,
   CHART_AXIS_LABELS_FONT,
   CHART_AXIS_LABELS_VISIBLE,
   CHART_AXIS_LINE_VISIBLE,
   CHART_AXIS_MINOR_GRID_LINE_COLOR,
   CHART_AXIS_MINOR_GRID_VISIBLE,
   CHART_AXIS_ORIENTATION,
   CHART_AXIS_REVERSE,
   CHART_AXIS_SHADES_BORDER_COLOR,
   CHART_AXIS_SHADES_COLOR,
   CHART_AXIS_SHADES_VISIBLE,
   CHART_AXIS_TITLE_COLOR,
   CHART_AXIS_TITLE_TEXT,
   CHART_AXIS_TITLE_VISIBLE,
   CHART_AXIS_VISIBLE,

   /*
   ** Data Properties
   */

   DATA_EXTRACTION_SCRIPT,
   DATA_SOURCE_ADDRESS,
   DATA_SOURCE_READ_BUFFER_SIZE,
   DATA_SOURCE_READ_RATE,
   DATA_SOURCE_TYPE,

   /*
   ** Legend Properties
   */

   LEGEND_ALIGNMENT,
   LEGEND_BACKGROUND_VISIBLE,
   LEGEND_BORDER_COLOR,
   LEGEND_COLOR,
   LEGEND_FONT,
   LEGEND_LABEL_COLOR,
   LEGEND_MARKER_SHAPE,
   LEGEND_REVERSE_MARKERS,
   LEGEND_SHOW_TOOL_TIPS,
   LEGEND_VISIBLE,

   /*
   ** XY Series Properties
   */

   XY_SERIES_COLOR,
   XY_SERIES_POINTS_VISIBLE,
   XY_SERIES_POINT_LABELS_CLIPPING,
   XY_SERIES_POINT_LABELS_COLOR,
   XY_SERIES_POINT_LABELS_FONT,
   XY_SERIES_POINT_LABELS_FORMAT,
   XY_SERIES_POINT_LABELS_VISIBLE
  };

  dooble_charts(QWidget *parent);
  virtual ~dooble_charts();
  QHash<dooble_charts::Properties, QVariant> properties(void) const;
#ifdef DOOBLE_QTCHARTS_PRESENT
  static QChart::AnimationOption string_to_chart_animation_option
    (const QString &t);
  static QChart::ChartTheme string_to_chart_theme(const QString &t);
  static QString chart_animation_option_to_string
    (const QChart::AnimationOptions chart_animation_options);
  static QString chart_theme_to_string(const QChart::ChartTheme chart_theme);
  static QString chart_type_to_string(const QChart::ChartType chart_type);
#endif
  static const QString s_axis_properties_strings[];
  static const QString s_chart_properties_strings[];
  static const QString s_data_properties_strings[];
  static const QString s_legend_properties_strings[];

 protected:
#ifdef DOOBLE_QTCHARTS_PRESENT
  QChart *m_chart;
  QChartView *m_chart_view;
  QLineSeries *m_series;
  QValueAxis *m_x_axis;
  QValueAxis *m_y_axis;
#endif
  Ui_dooble_charts m_ui;
  dooble_charts_property_editor *m_property_editor;

 protected slots:
  virtual void slot_item_changed(QStandardItem *item);
};

#endif
