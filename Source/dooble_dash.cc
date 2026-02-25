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

#include <QKeyEvent>
#include <QScrollBar>

#include "dooble_dash.h"

QString dooble_dash_textedit::current_command(void) const
{
  auto cursor(textCursor());

  cursor.movePosition(QTextCursor::StartOfLine);
  cursor.movePosition
    (QTextCursor::Right, QTextCursor::MoveAnchor, m_prompt_length);
  cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

  auto const command(cursor.selectedText().simplified().trimmed());

  cursor.clearSelection();
  return command;
}

QString dooble_dash_textedit::history(const int index) const
{
  if(index >= 0 && index < m_history.count())
    return m_history.at(index);
  else
    return "";
}

bool dooble_dash_textedit::handle_backspace_key(void) const
{
  auto const cursor(textCursor());

  if(cursor.blockNumber() == m_prompt_block_number &&
     cursor.columnNumber() == m_prompt_length)
    return true;

  return false;
}

void dooble_dash_textedit::clear_history(void)
{
  m_history.clear();
  m_history_index = 0;
}

void dooble_dash_textedit::display_prompt(void)
{
  setTextColor(QColor(0, 0, 139));

  auto cursor(textCursor());

  cursor.insertText(m_working_directory);
  cursor.insertText("> ");
  cursor.movePosition(QTextCursor::EndOfLine);
  setTextCursor(cursor);
  m_prompt_block_number = cursor.blockNumber();
}

void dooble_dash_textedit::handle_down_key(void)
{
  if(!m_history.isEmpty())
    {
      auto const command(current_command());

      do
	{
	  if(++m_history_index >= m_history.size())
	    {
	      m_history_index = m_history.size() - 1;
	      break;
	    }
	}
      while(command == m_history.value(m_history_index));

      if(m_history.size() < m_history_index)
	replace_current_command("");
      else
	replace_current_command(m_history.value(m_history_index));
    }
}

void dooble_dash_textedit::handle_home_key(void)
{
  auto cursor(textCursor());

  cursor.movePosition(QTextCursor::StartOfLine);
  cursor.movePosition
    (QTextCursor::Right, QTextCursor::MoveAnchor, m_prompt_length);
  setTextCursor(cursor);
}

void dooble_dash_textedit::handle_interrupt(void)
{
  replace_current_command(current_command() + "^C");
  append("");
  moveCursor(QTextCursor::End);
  emit interrupt();
}

void dooble_dash_textedit::handle_return_key(void)
{
  auto const command(current_command());

  if(!command.isEmpty())
    {
      emit process_command(command);
      m_history << command;
      m_history_index = m_history.size();
    }
  else
    append("");

  moveCursor(QTextCursor::End);
}

void dooble_dash_textedit::handle_tab_key(void)
{
}

void dooble_dash_textedit::handle_up_key(void)
{
  if(!m_history.isEmpty())
    {
      auto const command(current_command());

      do
	{
	  if(m_history_index > 0)
	    m_history_index -= 1;
	  else
	    break;
	}
      while(command == m_history.value(m_history_index));

      replace_current_command(m_history.value(m_history_index));
    }
}

void dooble_dash_textedit::keyPressEvent(QKeyEvent *event)
{
  if(!event)
    {
      QTextEdit::keyPressEvent(event);
      return;
    }

  switch(event->key())
    {
    case Qt::Key_Backspace:
      {
	if(handle_backspace_key())
	  return;

	break;
      }
    case Qt::Key_C:
      {
	auto const modifiers = QGuiApplication::keyboardModifiers();

	if(modifiers & Qt::ControlModifier)
	  {
	    handle_interrupt();
	    return;
	  }

	break;
      }
    case Qt::Key_Down:
      {
	handle_down_key();
	return;
      }
    case Qt::Key_End:
      {
	break;
      }
    case Qt::Key_Enter:
    case Qt::Key_Return:
      {
	handle_return_key();
	return;
      }
    case Qt::Key_Left:
      {
	if(handle_backspace_key())
	  return;

	break;
      }
    case Qt::Key_Home:
      {
	if(verticalScrollBar())
	  {
	    if(QGuiApplication::keyboardModifiers() & Qt::ControlModifier)
	      {
		verticalScrollBar()->setValue(0);
		return;
	      }
	  }

	handle_home_key();
	return;
      }
    case Qt::Key_PageDown:
      {
	if(verticalScrollBar())
	  {
	    verticalScrollBar()->triggerAction
	      (QAbstractSlider::SliderPageStepAdd);
	    return;
	  }

	break;
      }
    case Qt::Key_PageUp:
      {
	if(verticalScrollBar())
	  {
	    verticalScrollBar()->triggerAction
	      (QAbstractSlider::SliderPageStepSub);
	    return;
	  }

	break;
      }
    case Qt::Key_Tab:
      {
	handle_tab_key();
	return;
      }
    case Qt::Key_Up:
      {
	handle_up_key();
	return;
      }
    }

  QTextEdit::keyPressEvent(event);
}

void dooble_dash_textedit::mouseDoubleClickEvent(QMouseEvent *event)
{
  Q_UNUSED(event);
}

void dooble_dash_textedit::mousePressEvent(QMouseEvent *event)
{
  Q_UNUSED(event);
}

void dooble_dash_textedit::print_history(void)
{
  if(m_history.isEmpty())
    {
      m_history << tr("history ");
      m_history_index = 1;
    }

  for(int i = 0; i < m_history.size(); i++)
    append(QString("%1: %2").arg(i + 1).arg(m_history.at(i)));
}

void dooble_dash_textedit::replace_current_command(const QString &command)
{
  auto cursor(textCursor());

  cursor.movePosition(QTextCursor::StartOfLine);
  cursor.movePosition
    (QTextCursor::Right, QTextCursor::MoveAnchor, m_prompt_length);
  cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
  cursor.insertText(command);
}

void dooble_dash_textedit::set_working_directory(const QString &text)
{
  m_prompt_length = 2 + text.trimmed().length();
  m_working_directory = text.trimmed();
}

void dooble_dash_textedit::showEvent(QShowEvent *event)
{
  QTextEdit::showEvent(event);
  setFocus();
}

dooble_dash::dooble_dash(QWidget *parent):QDialog(parent)
{
  m_process.setProcessChannelMode(QProcess::SeparateChannels);
  m_process.setProgram("bash");
  m_process.setWorkingDirectory(QDir::currentPath());
  m_shell = "bash";
  m_shell_command_option = "-c";
  m_ui.setupUi(this);
  m_ui.text->setCursorWidth(10);
  m_ui.text->setUndoRedoEnabled(false);
  connect(&m_process,
	  SIGNAL(finished(int, QProcess::ExitStatus)),
	  this,
	  SLOT(slot_process_finished(int, QProcess::ExitStatus)));
  connect(m_ui.text,
	  SIGNAL(interrupt(void)),
	  this,
	  SLOT(slot_interrupt(void)));
  connect(m_ui.text,
	  SIGNAL(process_command(const QString &)),
	  this,
	  SLOT(slot_process_command(const QString &)));
}

dooble_dash::~dooble_dash()
{
  m_process.kill();
  m_process.terminate();
  m_process.waitForFinished();
}

void dooble_dash::slot_display_process_text(void)
{
}

void dooble_dash::slot_interrupt(void)
{
  m_process.kill();
  m_process.terminate();
}

void dooble_dash::slot_process_command(const QString &command)
{
  if(command.trimmed().isEmpty() ||
     m_process.state() != QProcess::NotRunning)
    {
      m_ui.text->append("");
      return;
    }

  m_process.setArguments(QStringList() << m_shell_command_option << command);
  m_process.start();
}

void dooble_dash::slot_process_finished
(int exit_code, QProcess::ExitStatus exit_status)
{
  Q_UNUSED(exit_code);
  Q_UNUSED(exit_status);
  m_ui.text->set_working_directory(m_process.workingDirectory());

  QByteArray bytes;
  auto appended = false;

  do
    {
      bytes = m_process.readAllStandardError();

      if(!bytes.isEmpty())
	{
	  appended = true;
	  m_ui.text->append(bytes);
	}
    }
  while(!bytes.isEmpty());

  do
    {
      bytes = m_process.readAllStandardOutput();

      if(!bytes.isEmpty())
	{
	  appended = true;
	  m_ui.text->append(bytes);
	}
    }
  while(!bytes.isEmpty());

  if(!appended)
    m_ui.text->append("");
}
