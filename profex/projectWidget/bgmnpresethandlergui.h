/***************************************************************************
                          bgmnpresethandlergui.h  -  description
                             -------------------
    begin                : Tue Jul 22 20:30:00 CEST 2025
    copyright            : (C) 2015 by Nicola Doebelin
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

#ifndef BGMNPRESETHANDLERGUI_H
#define BGMNPRESETHANDLERGUI_H

#include <QObject>
#include <QDomDocument>
#include <QString>

class BgmnProjectWidget;
class ControlFileEdit;

class BgmnPresetHandlerGui : public QObject
{
    Q_OBJECT
public:
    explicit BgmnPresetHandlerGui(BgmnProjectWidget *_projectWidget, ControlFileEdit *_editor);

    void savePreset(const QString &file);
    int applyPreset(const QString &file, bool overwrite = true);

    QDomDocument createBackupXmlFile(const QString &file);
    int restoreBackup(const QString &xmlFileName);

private:
    BgmnProjectWidget *project;
    ControlFileEdit *editor;

    void getModuleDataState(bool &, bool &, bool &, bool &, bool &, bool &);
    void getPresetDocumentXML(QDomDocument &doc, bool wrtIntegrals, bool wrtCurveFit, bool wrtExcelExp, bool wrtBaseLines, bool wrtPeakListFilters);
    void getBackupDocumentXML(QDomDocument &doc, bool wrtIntegrals, bool wrtCurveFit, bool wrtExcelExp, bool wrtPeakListFilters);

signals:
    void sigUpdatePresetMenu();
};

#endif // BGMNPRESETHANDLERGUI_H
