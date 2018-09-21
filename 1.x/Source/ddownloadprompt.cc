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

#include <QFileDialog>
#include <QSettings>

#include "ddownloadprompt.h"
#include "dmisc.h"
#include "dooble.h"

ddownloadprompt::ddownloadprompt(QWidget *parent,
				 const QString &fileName,
				 const DialogType type):QDialog(parent)
{
  m_suffix = QFileInfo(fileName).suffix().trimmed();
  ui.setupUi(this);
#ifdef Q_OS_MAC
  setAttribute(Qt::WA_MacMetalStyle, false);
#endif
  ui.questionLabel->setText(tr("What would you like Dooble to do with "
			       "%1?").arg(fileName));
  connect(ui.saveRadioButton,
	  SIGNAL(toggled(bool)),
	  this,
	  SLOT(slotRadioButtonToggled(bool)));
  connect(ui.saveAndOpenRadioButton,
	  SIGNAL(toggled(bool)),
	  this,
	  SLOT(slotRadioButtonToggled(bool)));
  connect(ui.applicationComboBox,
	  SIGNAL(activated(int)),
	  this,
	  SLOT(slotApplicationPulldownActivated(int)));
  connect(this,
	  SIGNAL(suffixesAdded(const QMap<QString, QString> &)),
	  dooble::s_settingsWindow,
	  SLOT(slotPopulateApplications(const QMap<QString, QString> &)));
  connect(this,
	  SIGNAL(suffixUpdated(const QString &, const QString &)),
	  dooble::s_settingsWindow,
	  SLOT(slotUpdateApplication(const QString &, const QString &)));

  QReadLocker locker(&dooble::s_applicationsActionsLock);

  if(dooble::s_applicationsActions.contains(m_suffix))
    {
      if(dooble::s_applicationsActions[m_suffix] != "prompt")
	{
	  QFileInfo fileInfo(dooble::s_applicationsActions[m_suffix]);

	  locker.unlock();
	  ui.applicationComboBox->addItem(fileInfo.fileName());
	  ui.applicationComboBox->insertSeparator(1);
	}
    }
  else
    {
      locker.unlock();
      dmisc::setActionForFileSuffix(m_suffix, "prompt");

      QMap<QString, QString> suffixes;

      suffixes[m_suffix] = "prompt";
      emit suffixesAdded(suffixes);
    }

  ui.applicationComboBox->addItem(tr("other"));

  QSettings settings(dooble::s_settings.value("iconSet").toString(),
		     QSettings::IniFormat);

  setWindowIcon(QIcon(settings.value("mainWindow/windowIcon").toString()));

  for(int i = 0; i < ui.buttonBox->buttons().size(); i++)
    if(ui.buttonBox->buttonRole(ui.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::AcceptRole ||
       ui.buttonBox->buttonRole(ui.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::ApplyRole ||
       ui.buttonBox->buttonRole(ui.buttonBox->buttons().at(i)) ==
       QDialogButtonBox::YesRole)
      {
	ui.buttonBox->buttons().at(i)->setIcon
	  (QIcon(settings.value("okButtonIcon").toString()));
	ui.buttonBox->buttons().at(i)->setIconSize(QSize(16, 16));
      }
    else
      {
	ui.buttonBox->buttons().at(i)->setIcon
	  (QIcon(settings.value("cancelButtonIcon").toString()));
	ui.buttonBox->buttons().at(i)->setIconSize(QSize(16, 16));
      }

  if(type == MultipleChoice)
    {
      ui.saveRadioButton->setChecked(true);
      ui.applicationComboBox->setEnabled(false);
    }
  else
    {
      ui.saveRadioButton->setVisible(false);
      ui.saveAndOpenRadioButton->setChecked(true);
      ui.saveAndOpenRadioButton->setText(tr("&Open the file with"));
    }

  resize(sizeHint());
}

ddownloadprompt::~ddownloadprompt()
{
}

int ddownloadprompt::exec(void)
{
  if(QDialog::exec() == QDialog::Accepted)
    {
      if(ui.saveRadioButton->isChecked())
	return 1;
      else
	return 2;
    }
  else
    return QDialog::Rejected;
}

void ddownloadprompt::slotApplicationPulldownActivated(int index)
{
  /*
  ** 0 - Use Other
  **
  ** 0 - Application
  ** 1 - Separator
  ** 2 - Use Other
  */

  QComboBox *comboBox = qobject_cast<QComboBox *> (sender());

  if(comboBox)
    {
      QFileDialog *fileDialog = 0;

      if(comboBox->count() == 1)
	/*
	** An application has not been selected.
	*/

	fileDialog = new QFileDialog(this);
      else
	{
	  /*
	  ** An application was previously selected.
	  */

	  if(index == 2)
	    fileDialog = new QFileDialog(this);
	}

      if(fileDialog)
	{
	  fileDialog->setFilter(QDir::AllDirs | QDir::Files
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_UNIX)
	                        | QDir::Readable | QDir::Executable);
#else
	                       );
#endif
	  fileDialog->setWindowTitle
	    (tr("Dooble Web Browser: Select Application"));
	  fileDialog->setFileMode(QFileDialog::ExistingFile);
	  fileDialog->setDirectory(QDir::homePath());
	  fileDialog->setLabelText(QFileDialog::Accept, tr("Select"));
	  fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
#ifdef Q_OS_MAC
	  fileDialog->setAttribute(Qt::WA_MacMetalStyle, false);
#endif
	  fileDialog->setOption
	    (QFileDialog::DontUseNativeDialog,
	     !dooble::s_settings.value("settingsWindow/useNativeFileDialogs",
				       false).toBool());

	  if(fileDialog->exec() == QDialog::Accepted)
	    {
	      QString action(fileDialog->selectedFiles().value(0));

	      if(comboBox->count() == 1)
		{
		  comboBox->insertItem
		    (0, QFileInfo(fileDialog->selectedFiles().value(0)).
		     fileName());
		  comboBox->insertSeparator(1);
		  ui.buttonBox->button(QDialogButtonBox::Ok)->
		    setEnabled(true);
		}
	      else
		comboBox->setItemText
		  (0, QFileInfo(fileDialog->selectedFiles().value(0)).
		   fileName());

	      dmisc::setActionForFileSuffix(m_suffix, action);
	      emit suffixUpdated(m_suffix, action);
	    }

	  comboBox->setCurrentIndex(0);
	  fileDialog->deleteLater();
	}
    }
}

void ddownloadprompt::slotRadioButtonToggled(bool state)
{
  QRadioButton *radioButton = qobject_cast<QRadioButton *> (sender());

  if(radioButton == ui.saveRadioButton)
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(state);
  else if(radioButton == ui.saveAndOpenRadioButton)
    {
      ui.applicationComboBox->setEnabled(state);

      if(ui.applicationComboBox->count() == 1)
	ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
      else if(ui.applicationComboBox->currentIndex() == 0)
	ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(state);
      else
	ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}
