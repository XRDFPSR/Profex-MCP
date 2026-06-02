/***************************************************************************
                          newinstrumentdialog.h  -  description
                             -------------------
    begin                : Sun Nov 30 2014
    copyright            : (C) 2003-2014 by Nicola Doebelin
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

#ifndef NEWINSTRUMENTDIALOG_H
#define NEWINSTRUMENTDIALOG_H

#include "../libXrdIO/import/genericimport.h"
#include "../libXrdIO/import/brukerrawimport.h"
#include "../libXrdIO/import/panalyticalxrdmlimport.h"
#include "../libXrdIO/import/brukerbrmlimport.h"
#include "../libXrdIO/settingsmanager.h"

#include <QDialog>

struct Variable{
    QString name;        // BGMN variable name
    QString value;       // value (in text format)
    QString description; // description to be shown in the GUI
    QString comment;     // comment string added to the SAV file

    Variable() : name(), value(), description(), comment() {}
    Variable(QString n, QString v, QString d, QString c) : name(n), value(v), description(d), comment(c) {}
};

namespace Ui {
class NewInstrumentDialog;
}

class NewInstrumentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewInstrumentDialog(QWidget *parent = 0);
    ~NewInstrumentDialog();

    inline const QString getSavFileName()      {return savFile;}

    inline void setWorkingDir(const QString s) {workingDir = s;}
    bool openFile();

private:
    Ui::NewInstrumentDialog *ui;

    SettingsManager *settings;
    QString savFile;
    QString scanFile;
    QString workingDir;
    QMap<QString, Variable> varList;
    QString separ;
    int dsType; // 0 = fds, 1 = ads

    BrukerRawImport *bruRawImport;
    PanalyticalXrdmlImport *panXrdmlImport;
    BrukerBrmlImport *bruBrmlImport;

    void parseBrukerRaw(const QString &);
    void parseBrukerBRML(const QString &);
    void parsePanalyticalXRDML(const QString &);

    void parseBrukerRawV4(const QVariantHash &);

    void initVarList();
    void createTable();
    void dumpToText(const QVariantHash &);
    QString generateFileString(const QString &);
    QString box(const QString &);

private slots:
    void accept();
    void reject();
};

#endif // NEWINSTRUMENTDIALOG_H
