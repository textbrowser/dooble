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

#include <QCloseEvent>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QSettings>
#include <QTextStream>
#include <QUrl>

#include "dooble.h"
#include "dpagesourcewindow.h"

dpagesourcewindow::dpagesourcewindow(QWidget *parent,
				     const QUrl &url,
				     const QString &html):QMainWindow()
{
  ui.setupUi(this);
#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);
#if QT_VERSION >= 0x050000
  setWindowFlags(windowFlags() & ~Qt::WindowFullscreenButtonHint);
#endif
  statusBar()->setSizeGripEnabled(false);
#endif
  ui.findLineEdit->setPlaceholderText(tr("Search Source"));
  m_findLineEditPalette = ui.findLineEdit->palette();
  ui.textBrowser->setPlainText(html);
  connect(ui.actionPrint, SIGNAL(triggered(void)), this,
	  SLOT(slotPrint(void)));
  connect(ui.actionPrint_Preview, SIGNAL(triggered(void)), this,
	  SLOT(slotPrintPreview(void)));
  connect(ui.actionClose, SIGNAL(triggered(void)), this,
	  SLOT(slotClose(void)));
  connect(ui.actionFind, SIGNAL(triggered(void)), this,
	  SLOT(slotShowFind(void)));
  connect(ui.actionSave_As, SIGNAL(triggered(void)), this,
	  SLOT(slotSavePageAs(void)));
  connect(ui.hideFindToolButton, SIGNAL(clicked(void)), this,
	  SLOT(slotHideFind(void)));
  connect(ui.findLineEdit, SIGNAL(returnPressed(void)), this,
	  SLOT(slotNextFind(void)));
  connect(ui.findLineEdit, SIGNAL(textEdited(const QString &)), this,
	  SLOT(slotNextFind(const QString &)));
  connect(ui.actionWrap_Lines, SIGNAL(toggled(bool)),
	  this, SLOT(slotWrapLines(bool)));
  connect(ui.nextToolButton, SIGNAL(clicked(void)),
	  this, SLOT(slotNextFind(void)));
  connect(ui.previousToolButton, SIGNAL(clicked(void)),
	  this, SLOT(slotPreviousFind(void)));

  if(!url.isEmpty())
    setWindowTitle(tr("Dooble Web Browser - Page Source (") +
		   url.toString(QUrl::StripTrailingSlash) + tr(")"));
  else
    setWindowTitle(tr("Dooble Web Browser - Page Source"));

  if(url.path().isEmpty() || url.path() == "/")
    fileName = "source";
  else if(url.path().contains("/"))
    {
      fileName = url.path();
      fileName = fileName.mid(fileName.lastIndexOf("/") + 1);
    }
  else
    fileName = url.path();

  ui.actionWrap_Lines->setChecked
    (dooble::s_settings.value("pageSource/wrapLines", true).toBool());
  slotWrapLines(ui.actionWrap_Lines->isChecked());
  slotSetIcons();
  slotHideFind();

  if(parent)
    {
      if(parent->height() == height() &&
	 parent->width() == width())
	setGeometry(parent->geometry());
      else
	{
	  QPoint p(parent->pos());
	  int X = 0;
	  int Y = 0;

	  if(parent->width() >= width())
	    X = p.x() + (parent->width() - width()) / 2;
	  else
	    X = p.x() - (width() - parent->width()) / 2;

	  if(parent && parent->height() >= height())
	    Y = p.y() + (parent->height() - height()) / 2;
	  else
	    Y = p.y() - (height() - parent->height()) / 2;

	  move(X, Y);
	}
    }
  else
    move(100, 100);

  show();
}

dpagesourcewindow::~dpagesourcewindow()
{
}

void dpagesourcewindow::slotClose(void)
{
  close();
}

void dpagesourcewindow::closeEvent(QCloseEvent *event)
{
  Q_UNUSED(event);
  deleteLater();
}

void dpagesourcewindow::slotPrint(void)
{
  QPrinter printer(QPrinter::HighResolution);
  QPrintDialog printDialog(&printer, this);

#ifdef Q_OS_MAC
  printDialog.setAttribute(Qt::WA_MacMetalStyle, false);
#endif

  if(printDialog.exec() == QDialog::Accepted)
    ui.textBrowser->print(&printer);
}

void dpagesourcewindow::slotPrintPreview(void)
{
  QPrinter printer(QPrinter::HighResolution);
  QPrintPreviewDialog printDialog(&printer, this);

#ifdef Q_OS_MAC
  printDialog.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  printDialog.setWindowModality(Qt::WindowModal);
  connect(&printDialog,
	  SIGNAL(paintRequested(QPrinter *)),
	  this,
	  SLOT(slotTextEditPrintPreview(QPrinter *)));

  if(printDialog.exec() == QDialog::Accepted)
    ui.textBrowser->print(&printer);
}

void dpagesourcewindow::slotTextEditPrintPreview(QPrinter *printer)
{
  if(printer)
    ui.textBrowser->print(printer);
}

void dpagesourcewindow::slotHideFind(void)
{
  ui.findFrame->setVisible(false);
}

void dpagesourcewindow::slotShowFind(void)
{
  ui.findFrame->setVisible(true);
  ui.findLineEdit->setFocus();
  ui.findLineEdit->selectAll();

#ifdef Q_OS_MAC
  static int fixed = 0; // Pre-Qt 5.x?

  if(!fixed)
    {
      QColor color(255, 255, 255);
      QPalette palette(ui.findLineEdit->palette());

      palette.setColor(ui.findLineEdit->backgroundRole(), color);
      ui.findLineEdit->setPalette(palette);
      fixed = 1;
    }
#endif
}

void dpagesourcewindow::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    ui.findFrame->setVisible(false);

  QMainWindow::keyPressEvent(event);
}

void dpagesourcewindow::slotNextFind(void)
{
  slotNextFind(ui.findLineEdit->text());
}

void dpagesourcewindow::slotNextFind(const QString &text)
{
  QTextDocument::FindFlags findFlags = 0;

  if(ui.matchCaseCheckBox->isChecked())
    findFlags |= QTextDocument::FindCaseSensitively;

  if(ui.textBrowser->find(text, findFlags) || text.isEmpty())
    {
      ui.findLineEdit->setPalette(m_findLineEditPalette);

      if(text.isEmpty())
	ui.textBrowser->moveCursor(QTextCursor::PreviousCharacter);
    }
  else
    {
      if(ui.textBrowser->textCursor().anchor() ==
	 ui.textBrowser->textCursor().position())
	{
	  if(!ui.textBrowser->textCursor().atEnd())
	    {
	      QColor color(240, 128, 128); // Light Coral
	      QPalette palette(ui.findLineEdit->palette());

	      palette.setColor(ui.findLineEdit->backgroundRole(), color);
	      ui.findLineEdit->setPalette(palette);
	    }
	  else
	    ui.textBrowser->moveCursor(QTextCursor::Start);
	}
      else
	{
	  ui.textBrowser->moveCursor(QTextCursor::Start);
	  slotNextFind(text);
	}
    }
}

void dpagesourcewindow::slotPreviousFind(void)
{
  slotPreviousFind(ui.findLineEdit->text());
}

void dpagesourcewindow::slotPreviousFind(const QString &text)
{
  QTextDocument::FindFlags findFlags = QTextDocument::FindBackward;

  if(ui.matchCaseCheckBox->isChecked())
    findFlags |= QTextDocument::FindCaseSensitively;

  if(ui.textBrowser->find(text, findFlags) || text.isEmpty())
    ui.findLineEdit->setPalette(m_findLineEditPalette);
  else
    {
      if(ui.textBrowser->textCursor().anchor() ==
	 ui.textBrowser->textCursor().position())
	{
	  QColor color(240, 128, 128); // Light Coral
	  QPalette palette(ui.findLineEdit->palette());

	  palette.setColor(ui.findLineEdit->backgroundRole(), color);
	  ui.findLineEdit->setPalette(palette);
	}
      else
	{
	  ui.textBrowser->moveCursor(QTextCursor::End);
	  slotPreviousFind(text);
	}
    }
}

void dpagesourcewindow::slotSavePageAs(void)
{
  QString path(dooble::s_settings.value("settingsWindow/myRetrievedFiles",
					"").toString());
  QFileInfo fileInfo(path);
  QFileDialog fileDialog(this);

  if(!fileInfo.isReadable() || !fileInfo.isWritable())
#if QT_VERSION >= 0x050000
    path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
#else
    path = QDesktopServices::storageLocation
      (QDesktopServices::DesktopLocation);
#endif

#ifdef Q_OS_MAC
  fileDialog.setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  fileDialog.setOption
    (QFileDialog::DontUseNativeDialog,
     !dooble::s_settings.value("settingsWindow/useNativeFileDialogs",
			       false).toBool());
  fileDialog.setWindowTitle(tr("Dooble Web Browser: Save Page Source As"));
  fileDialog.setFileMode(QFileDialog::AnyFile);
  fileDialog.setDirectory(path);
  fileDialog.setLabelText(QFileDialog::Accept, tr("Save"));
  fileDialog.setAcceptMode(QFileDialog::AcceptSave);
  fileDialog.selectFile(fileName);

  if(fileDialog.exec() == QDialog::Accepted)
    {
      QStringList list(fileDialog.selectedFiles());

      if(!list.isEmpty())
	{
	  QFile file(list.at(0));

	  if(file.open(QIODevice::WriteOnly | QIODevice::Text))
	    {
	      QTextStream out(&file);

	      out << ui.textBrowser->toPlainText();
	      file.close();
	    }
	}
    }
}

void dpagesourcewindow::slotWrapLines(bool checked)
{
  QSettings settings;

  settings.setValue("pageSource/wrapLines", checked);
  dooble::s_settings["pageSource/wrapLines"] = checked;

  if(checked)
    ui.textBrowser->setLineWrapMode(QTextEdit::WidgetWidth);
  else
    ui.textBrowser->setLineWrapMode(QTextEdit::NoWrap);
}

void dpagesourcewindow::slotSetIcons(void)
{
  QSettings settings
    (dooble::s_settings.value("iconSet").toString(), QSettings::IniFormat);

  settings.beginGroup("pageSourceWindow");
  ui.actionPrint->setIcon(QIcon(settings.value("actionPrint").toString()));
  ui.actionPrint_Preview->setIcon
    (QIcon(settings.value("actionPrint_Preview").toString()));
  ui.actionClose->setIcon(QIcon(settings.value("actionClose").toString()));
  ui.actionFind->setIcon(QIcon(settings.value("actionFind").toString()));
  ui.actionSave_As->setIcon
    (QIcon(settings.value("actionSave_As").toString()));
  ui.hideFindToolButton->setIcon
    (QIcon(settings.value("hideFindToolButton").toString()));
  ui.nextToolButton->setIcon
    (QIcon(settings.value("nextToolButton").toString()));
  ui.previousToolButton->setIcon
    (QIcon(settings.value("previousToolButton").toString()));
  setWindowIcon(QIcon(settings.value("windowIcon").toString()));
}

#ifdef Q_OS_MAC
#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050300
bool dpagesourcewindow::event(QEvent *event)
{
  if(event)
    if(event->type() == QEvent::WindowStateChange)
      if(windowState() == Qt::WindowNoState)
	{
	  /*
	  ** Minimizing the window on OS 10.6.8 and Qt 5.x will cause
	  ** the window to become stale once it has resurfaced.
	  */

	  hide();
	  show();
	  update();
	}

  return QMainWindow::event(event);
}
#else
bool dpagesourcewindow::event(QEvent *event)
{
  return QMainWindow::event(event);
}
#endif
#else
bool dpagesourcewindow::event(QEvent *event)
{
  return QMainWindow::event(event);
}
#endif
