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

#ifndef _dooble_dash_h_
#define _dooble_dash_h_

#include <QDir>
#include <QProcess>
#include <QTextEdit>

class dooble_dash_textedit: public QTextEdit
{
  Q_OBJECT

 public:
  dooble_dash_textedit(QWidget *parent):QTextEdit(parent)
  {
    auto font(this->font());

    font.setFamily("Courier");
    font.setPointSizeF(11.5);
    setFont(font);
    m_history_index = 0;
    m_prompt_block_number = 0;
    m_prompt_length = 2 + QDir::currentPath().length();
    m_working_directory = QDir::currentPath();
    display_prompt();
  }

  void append_with_prompt(const QString &text)
  {
    if(text.isEmpty())
      QTextEdit::append("");
    else
      QTextEdit::append(text + (text.endsWith('\n') ? + "" : "\n"));

    display_prompt();
  }

  QString history(const int index) const;
  void display_prompt(void);
  void set_working_directory(const QString &text);

 private:
  QString m_working_directory;
  QStringList m_history;
  int m_history_index;
  int m_prompt_block_number;
  int m_prompt_length;
  QString current_command(void) const;
  bool handle_backspace_key(void) const;
  void handle_down_key(void);
  void handle_home_key(void);
  void handle_interrupt(void);
  void handle_return_key(void);
  void handle_tab_key(void);
  void handle_up_key(void);
  void keyPressEvent(QKeyEvent *event);
  void mouseDoubleClickEvent(QMouseEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void replace_current_command(const QString &command);
  void showEvent(QShowEvent *event);

 signals:
  void interrupt(void);
  void process_command(const QString &command);
};

#include "ui_dooble_dash.h"

class dooble_dash: public QDialog
{
  Q_OBJECT

 public:
  dooble_dash(QWidget *parent);
  ~dooble_dash();

 private:
  QProcess m_process;
  QString m_shell_command_option;
  Ui_dooble_dash m_ui;

 private slots:
  void slot_interrupt(void);
  void slot_process_command(const QString &command);
  void slot_process_finished(int exit_code, QProcess::ExitStatus exit_status);
  void slot_process_ready_read_standard_error(void);
  void slot_process_ready_read_standard_output(void);
};

#endif
