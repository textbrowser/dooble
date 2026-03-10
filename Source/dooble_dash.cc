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
#include "dooble_settings.h"

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
  append_with_prompt("");
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
    append_with_prompt("");

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
    case Qt::Key_Escape:
      {
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
	if((QGuiApplication::keyboardModifiers() & Qt::ControlModifier) &&
	   (verticalScrollBar()))
	  {
	    verticalScrollBar()->setValue(0);
	    return;
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
  m_prompt_length = 2 + text.length();
  m_working_directory = text;
}

void dooble_dash_textedit::showEvent(QShowEvent *event)
{
  QTextEdit::showEvent(event);
  setFocus();
}

dooble_dash::dooble_dash(QWidget *parent):QDialog(parent)
{
  m_process.setProcessChannelMode(QProcess::SeparateChannels);
  m_process.setProgram(dooble_settings::setting("shell").toString().trimmed());
  m_process.setWorkingDirectory(QDir::currentPath());
  m_shell_command_option = dooble_settings::setting("shell_command_option").
    toString().trimmed();
  m_ui.setupUi(this);
  m_ui.text->setCursorWidth(10);
  m_ui.text->setUndoRedoEnabled(false);
  new QShortcut(QKeySequence(tr("Ctrl+=")),
		this,
		SLOT(slot_zoom_in(void)));
  new QShortcut(QKeySequence(tr("Ctrl+-")),
		this,
		SLOT(slot_zoom_out(void)));
  new QShortcut(QKeySequence(tr("Ctrl+0")),
		this,
		SLOT(slot_zoom_reset(void)));
  connect(&m_process,
	  SIGNAL(finished(int, QProcess::ExitStatus)),
	  this,
	  SLOT(slot_process_finished(int, QProcess::ExitStatus)),
	  Qt::QueuedConnection);
  connect(&m_process,
	  SIGNAL(readyReadStandardError(void)),
	  this,
	  SLOT(slot_process_ready_read_standard_error(void)),
	  Qt::QueuedConnection);
  connect(&m_process,
	  SIGNAL(readyReadStandardOutput(void)),
	  this,
	  SLOT(slot_process_ready_read_standard_output(void)),
	  Qt::QueuedConnection);
  connect(m_ui.text,
	  SIGNAL(interrupt(void)),
	  this,
	  SLOT(slot_interrupt(void)));
  connect(m_ui.text,
	  SIGNAL(process_command(const QString &)),
	  this,
	  SLOT(slot_process_command(const QString &)),
	  Qt::QueuedConnection);
  m_process.start();
}

dooble_dash::~dooble_dash()
{
  m_process.closeWriteChannel();
  m_process.kill();
  m_process.terminate();
  m_process.waitForFinished();
}

void dooble_dash::slot_interrupt(void)
{
  m_process.blockSignals(true);
  m_process.closeWriteChannel();
  m_process.kill();
  m_process.terminate();
  m_process.waitForFinished();
  m_process.blockSignals(false);
  m_process.start();
}

void dooble_dash::slot_process_command(const QString &command)
{
  if(command.trimmed().isEmpty())
    return;

  /*
  ** Some commands will not write to standard output.
  */

  m_process.write((command + " && pwd").toUtf8().constData());
  m_process.write("\n");
  m_process.waitForBytesWritten();
  m_process.waitForReadyRead(250);
}

void dooble_dash::slot_process_finished
(int exit_code, QProcess::ExitStatus exit_status)
{
  Q_UNUSED(exit_code);
  Q_UNUSED(exit_status);
  deleteLater();
}

void dooble_dash::slot_process_ready_read_standard_error(void)
{
  QByteArray bytes;
  int i = 500;

  do
    {
      auto const b(m_process.readAllStandardError());

      if(b.isEmpty())
	{
	  QApplication::processEvents();
	  i -= 1;
	}
      else
	bytes.append(b);
    }
  while(i >= 0);

  if(!bytes.isEmpty())
    m_ui.text->append_with_prompt(bytes);
}

void dooble_dash::slot_process_ready_read_standard_output(void)
{
  QByteArray bytes;
  int i = 10;

  do
    {
      auto const b(m_process.readAllStandardOutput());

      if(b.isEmpty())
	{
	  i -= 1;
	  m_process.waitForReadyRead(1);
	}
      else
	bytes.append(b);
    }
  while(i >= 0);

  if(bytes.trimmed().isEmpty())
    return;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  auto const list(QString(bytes.trimmed()).split('\n'));
#else
  auto const list(QString(bytes.trimmed()).split('\n'));
#endif

  for(int i = 0; i < list.size(); i++)
    if(i != list.size() - 1)
      m_ui.text->append(list[i]);
    else if(QFileInfo(list[i]).isDir())
      {
	m_process.setWorkingDirectory(list[i]);
	m_ui.text->set_working_directory(m_process.workingDirectory());
	m_ui.text->append_with_prompt("");
      }
    else
      m_ui.text->append(list[i]);

  zoom(2); // Text color.
}

void dooble_dash::slot_zoom_in(void)
{
  zoom(1);
}

void dooble_dash::slot_zoom_out(void)
{
  zoom(-1);
}

void dooble_dash::slot_zoom_reset(void)
{
  zoom(0);
}

void dooble_dash::zoom(const int value)
{
  QTextCharFormat format;
  auto cursor(m_ui.text->textCursor());
  auto font(m_ui.text->currentFont());

  switch(value)
    {
    case -1:
      {
	font.setPointSizeF(qMax(8.0, -1.0 + font.pointSizeF()));
	break;
      }
    case 0:
      {
	font.setPointSizeF(11.5);
	break;
      }
    case 1:
      {
	font.setPointSizeF(1.0 + font.pointSizeF());
      }
    default:
      {
	break;
      }
    }

  cursor.movePosition(QTextCursor::Start);
  cursor.select(QTextCursor::Document);
  format.setFont(font);
  format.setForeground(QColor(0, 0, 139));
  cursor.setCharFormat(format);
  m_ui.text->setCurrentFont(font);
  m_ui.text->setTextColor(format.foreground().color());
}
