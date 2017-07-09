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

#ifndef _ddownloadprompt_h_
#define _ddownloadprompt_h_

#include <QDialog>

#include "ui_ddownloadPrompt.h"

class ddownloadprompt: public QDialog
{
  Q_OBJECT

 public:
  enum DialogType
    {
      MultipleChoice,
      SingleChoice
    };

  Q_ENUMS(DialogType);

 public:
  static const int MULTIPLE_CHOICE = 2;
  static const int SINGLE_CHOICE = 1;
  ddownloadprompt(QWidget *parent, const QString &fileName,
		  const DialogType dialogType);
  ~ddownloadprompt();
  int exec(void);

 private:
  QString m_suffix;
  Ui_downloadPromptDialog ui;

 private slots:
  void slotApplicationPulldownActivated(int index);
  void slotRadioButtonToggled(bool state);

 signals:
  void suffixUpdated(const QString &suffix, const QString &action);
  void suffixesAdded(const QMap<QString, QString> &suffixes);
};

#endif
