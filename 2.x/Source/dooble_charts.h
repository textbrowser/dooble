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
   ** Generic Properties
   */

   ANIMATION_DURATION = 0,
   ANIMATION_OPTIONS,
   BACKGROUND_COLOR,
   BACKGROUND_ROUNDNESS,
   BACKGROUND_VISIBLE,
   CHART_TYPE,
   DROP_SHADOW_ENABLED,
   LOCALE,
   LOCALIZE_NUMBERS,
   MARGINS,
   PLOT_AREA_BACKGROUND_VISIBLE,
   THEME,
   TITLE,
   TITLE_COLOR,
   TITLE_FONT,
   X_AXIS_TITLE,
   Y_AXIS_TITLE
  };

  enum PropertiesIndices
  {
   /*
   ** Generic Properties
   */
  };

  static const char *PropertiesStrings[];
  dooble_charts(QWidget *parent);
  virtual ~dooble_charts();
  QHash<dooble_charts::Properties, QVariant> properties(void) const;

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
};

#endif
