/***************************************************************************
                          learnprofiledialog.h  -  description
                             -------------------
    begin                : Fri Jan 08 09:25:00 CET 2016
    copyright            : (C) 2016 by Nicola Doebelin
    email                : ndoebelin@gmx.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LEARNPROFILEDIALOG_H
#define LEARNPROFILEDIALOG_H

#include "projectWidget/processhandler.h"
#include "projectWidget/syntaxHighlighter/bgmnhighlighter.h"
#include "../libXrdIO/settingsmanager.h"
#include <QDialog>
#include <QPlainTextEdit>

namespace Ui {
class LearnProfileDialog;
}

class LearnProfileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LearnProfileDialog(QWidget *parent = 0);
    ~LearnProfileDialog();

private:
    Ui::LearnProfileDialog *ui;

    SettingsManager *settings;
    QString parFileName;
    QString savFileName;
    QString parWorkingDir;
    QString savWorkingDir;
    ProcessHandler *verzerr;
    ProcessHandler *makegeq;
    QMap<QPlainTextEdit*, QSyntaxHighlighter*> highlighters;

    void initProcesses();
    QString readTextFileFromDisk(const QString &fn);
    QString unifyPar(const QString &s);
    bool saveTextFile(const QString &fn, const QString &str);

private slots:
    void openPar();
    void saveAsPar();
    void openSav();
    void saveAsSav();
    void run();
    void pollOutput();
    void processAborted();
    void verzerrComplete();
    void makegeqComplete();

};

#endif // LEARNPROFILEDIALOG_H
