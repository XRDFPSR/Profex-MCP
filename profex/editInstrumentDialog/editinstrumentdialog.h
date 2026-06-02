/***************************************************************************
                          editinstrumentdialog.h  -  description
                             -------------------
    begin                : Wed Feb 30 11:00:00 CEST 2013
    copyright            : (C) 2013 by Nicola Doebelin
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

#ifndef EDITINSTRUMENTDIALOG_H
#define EDITINSTRUMENTDIALOG_H

#include "projectWidget/processhandler.h"
#include "instrumentscene.h"
#include "savtemplatemanager.h"
#include <QString>
#include <QMap>
#include <QList>
#include <QStringList>
#include <QDialog>
#include <QTreeWidgetItem>
#include "../libXrdIO/settingsmanager.h"
#include "abstractopticsconfigpage.h"
#include "instrumentraytracermt.h"

namespace Ui {
class EditInstrumentDialog;
}

enum RunState {idle, running, complete, aborted};

class EditInstrumentDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit EditInstrumentDialog(QWidget *parent = 0);
    ~EditInstrumentDialog();

    void resetDialog();
    void openInstrumentSavFile(const QString &, const QString &);
    inline void setWorkingDir(const QString &s)     {workingDir = s;}

    QString getWorkingDir() {return workingDir;}
    
private:
    Ui::EditInstrumentDialog *ui;

    SettingsManager *settings;
    InstrumentScene *scene;
    InstrumentRayTracerMT *rayTracer;
    SavTemplateManager templManager;
    QString workingDir;
    QString instrSavFile;    
    QString origInstrSavFileContent;
    QString projectSavFile;
    QString header;
    bool useWine;
    bool abort;
    QMap<QString, AbstractOpticsConfigPage *> configPageMap;
    QMap<QString, QTreeWidgetItem *> treeWidgetMap;
    QMap<QString, QString> configParamHelpText;
    QAction *loadCurrentAction;
    QAction *saveCurrentAction;

    QString getParameterFromPage(const QString &page, const QString &param, bool &ok);
    void initConfigPages();
    void initSettings();
    void saveSettings();
    void showEvent(QShowEvent *);
    void closeEvent(QCloseEvent *);
    void save(const QString &);
    void saveTemplate(const QString &s);
    bool checkOutputNames();
    void fixOutputNames();
    void setUiStateRunning(RunState);
    void updateTemplateGui();
    QMap<QString, QString> parseControlFile();
    QString updateControlFile();
    void blockToggleButtonSignals(bool);
    void setSavTemplateFile(const QString &);

    void raiseGraphicalEditor();
    void raiseControlFileEditor();
    void raiseOriginalFileEditor();

private slots:
    void toggleItemInstalled(QTreeWidgetItem*, int);
    void toggleGraphEditor(bool);
    void toggleControlFileEditor(bool);
    void toggleOrigFileEditor(bool);

    void runCalculation();
    void calculationComplete();
    void fileSave();
    void fileSaveAs();
    void fileReset();
    void instrumentItemSelected(QString);
    void textDocumentChanged();
    void loadFile();
    void configItemChanged(QTreeWidgetItem*, QTreeWidgetItem*);
    void wordUnderCursorChanged(QStringList);

    void showHelp();

signals:
    void helpText(QString);
};

#endif // EDITINSTRUMENTDIALOG_H
