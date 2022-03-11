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
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
using namespace QtCharts;
#endif
#endif

#include <QMap>
#include <QPointer>

#include "ui_dooble_charts.h"

class QPrinter;
class QStandardItem;
class dooble_charts_iodevice;
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
     CHART_NAME,
     CHART_PLOT_AREA_BACKGROUND_VISIBLE,
     CHART_THEME,
     CHART_TITLE,
     CHART_TITLE_COLOR,
     CHART_TITLE_FONT,

     /*
     ** Chart Axis Properties
     */

     CHART_AXIS_X_ALIGNMENT_HORIZONTAL,
     CHART_AXIS_X_ALIGNMENT_VERTICAL,
     CHART_AXIS_X_COLOR,
     CHART_AXIS_X_GRID_LINE_COLOR,
     CHART_AXIS_X_GRID_VISIBLE,
     CHART_AXIS_X_LABELS_ANGLE,
     CHART_AXIS_X_LABELS_COLOR,
     CHART_AXIS_X_LABELS_FONT,
     CHART_AXIS_X_LABELS_VISIBLE,
     CHART_AXIS_X_LINE_VISIBLE,
     CHART_AXIS_X_MINOR_GRID_LINE_COLOR,
     CHART_AXIS_X_MINOR_GRID_LINE_VISIBLE,
     CHART_AXIS_X_ORIENTATION,
     CHART_AXIS_X_REVERSE,
     CHART_AXIS_X_SHADES_BORDER_COLOR,
     CHART_AXIS_X_SHADES_COLOR,
     CHART_AXIS_X_SHADES_VISIBLE,
     CHART_AXIS_X_TITLE_COLOR,
     CHART_AXIS_X_TITLE_FONT,
     CHART_AXIS_X_TITLE_TEXT,
     CHART_AXIS_X_TITLE_VISIBLE,
     CHART_AXIS_X_VISIBLE,
     CHART_AXIS_Y_ALIGNMENT_HORIZONTAL,
     CHART_AXIS_Y_ALIGNMENT_VERTICAL,
     CHART_AXIS_Y_COLOR,
     CHART_AXIS_Y_GRID_LINE_COLOR,
     CHART_AXIS_Y_GRID_VISIBLE,
     CHART_AXIS_Y_LABELS_ANGLE,
     CHART_AXIS_Y_LABELS_COLOR,
     CHART_AXIS_Y_LABELS_FONT,
     CHART_AXIS_Y_LABELS_VISIBLE,
     CHART_AXIS_Y_LINE_VISIBLE,
     CHART_AXIS_Y_MINOR_GRID_LINE_COLOR,
     CHART_AXIS_Y_MINOR_GRID_LINE_VISIBLE,
     CHART_AXIS_Y_ORIENTATION,
     CHART_AXIS_Y_REVERSE,
     CHART_AXIS_Y_SHADES_BORDER_COLOR,
     CHART_AXIS_Y_SHADES_COLOR,
     CHART_AXIS_Y_SHADES_VISIBLE,
     CHART_AXIS_Y_TITLE_COLOR,
     CHART_AXIS_Y_TITLE_FONT,
     CHART_AXIS_Y_TITLE_TEXT,
     CHART_AXIS_Y_TITLE_VISIBLE,
     CHART_AXIS_Y_VISIBLE,

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
     XY_SERIES_NAME,
     XY_SERIES_OPACITY,
     XY_SERIES_POINTS_VISIBLE,
     XY_SERIES_POINT_LABELS_CLIPPING,
     XY_SERIES_POINT_LABELS_COLOR,
     XY_SERIES_POINT_LABELS_FONT,
     XY_SERIES_POINT_LABELS_FORMAT,
     XY_SERIES_POINT_LABELS_VISIBLE,
     XY_SERIES_USE_OPENGL,
     XY_SERIES_VISIBLE,
     XY_SERIES_X_AXIS,
     XY_SERIES_X_AXIS_LABEL_FORMAT,
     XY_SERIES_X_AXIS_MAX,
     XY_SERIES_X_AXIS_MIN,
     XY_SERIES_X_AXIS_MINOR_TICK_COUNT,
     XY_SERIES_X_AXIS_TICK_ANCHOR,
     XY_SERIES_X_AXIS_TICK_COUNT,
     XY_SERIES_X_AXIS_TICK_INTERVAL,
     XY_SERIES_X_AXIS_TICK_TYPE,
     XY_SERIES_Y_AXIS,
     XY_SERIES_Y_AXIS_LABEL_FORMAT,
     XY_SERIES_Y_AXIS_MAX,
     XY_SERIES_Y_AXIS_MIN,
     XY_SERIES_Y_AXIS_MINOR_TICK_COUNT,
     XY_SERIES_Y_AXIS_TICK_ANCHOR,
     XY_SERIES_Y_AXIS_TICK_COUNT,
     XY_SERIES_Y_AXIS_TICK_INTERVAL,
     XY_SERIES_Y_AXIS_TICK_TYPE,
    };

  dooble_charts(QWidget *parent);
  virtual ~dooble_charts();
  static const QString s_axis_properties_strings[];
  static const QString s_chart_properties_strings[];
  static const QString s_data_properties_strings[];
  static const QString s_legend_properties_strings[];
  QHash<dooble_charts::Properties, QVariant> data_properties(void) const;
  QHash<dooble_charts::Properties, QVariant> legend_properties(void) const;
  QHash<dooble_charts::Properties, QVariant> x_axis_properties(void) const;
  QHash<dooble_charts::Properties, QVariant> y_axis_properties(void) const;
  QMenu *menu(void);
  QPixmap pixmap(void) const;
  QString name(void) const;
  QWidget *frame(void) const;
  QWidget *view(void) const;
#ifdef DOOBLE_QTCHARTS_PRESENT
  static QChart::AnimationOptions string_to_chart_animation_options
    (const QString &t);
  static QChart::ChartTheme string_to_chart_theme(const QString &t);
  static QLegend::MarkerShape string_to_legend_marker_shape(const QString &t);
  static QString chart_animation_option_to_string
    (const QChart::AnimationOptions chart_animation_options);
  static QString chart_theme_to_string(const QChart::ChartTheme chart_theme);
  static QString chart_type_to_string(const QChart::ChartType chart_type);
  static QString legend_marker_shape_to_string
    (const QLegend::MarkerShape marker_shape);
#endif
  static QString property_to_name(const dooble_charts::Properties property);
  static QString type_from_database(const QString &name);
  static void purge(void);
  virtual QHash<dooble_charts::Properties, QVariant> properties(void) const;
  virtual void decouple(void);
  virtual void open(const QString &name);
  virtual void save(QString &error);

 private:
  QHash<QString, QVariant> data_properties_for_database(void) const;
  QHash<QString, QVariant> legend_properties_for_database(void) const;
  QHash<QString, QVariant> properties_for_database(void) const;
  QHash<QString, QVariant> x_axis_properties_for_database(void) const;
  QHash<QString, QVariant> y_axis_properties_for_database(void) const;
  QMultiHash<dooble_charts::Properties, QVariant> all_properties(void) const;
  virtual void create_default_device(void);

 protected:
#ifdef DOOBLE_QTCHARTS_PRESENT
  QChart *m_chart;
  QChartView *m_chart_view;
  QLegend *m_legend;
#endif
#ifdef DOOBLE_QTCHARTS_PRESENT
  QMap<int, QPointer<QAbstractSeries> > m_series;
#endif
  QMap<int, QPointer<dooble_charts_iodevice> > m_iodevices;
  QMenu *m_menu;
#ifdef DOOBLE_QTCHARTS_PRESENT
  QPointer<QAbstractAxis> m_x_axis;
  QPointer<QAbstractAxis> m_y_axis;
#endif
  Ui_dooble_charts m_ui;
  bool m_print_preview;
  dooble_charts_property_editor *m_property_editor;

 protected slots:
  virtual void slot_clear(void);
  virtual void slot_data_ready(const QVector<double> &vector,
			       const int index) = 0;
  virtual void slot_item_changed(QStandardItem *item);
  virtual void slot_play(void);
  virtual void slot_pause(void);
  virtual void slot_print(void);
  virtual void slot_print_preview(QPrinter *printer);
  virtual void slot_print_preview(void);
  virtual void slot_save(void);
  virtual void slot_stop(void);
  void slot_apply_properties_after_theme_changed(void);
};

#endif
