/***************************************************************************
                          abstracttooldialog.h  -  description
                             -------------------
    begin                : Wed Feb 19 11:00:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#ifndef ABSTRACTTOOLDIALOG_H
#define ABSTRACTTOOLDIALOG_H

#include "projectWidget/projectwidget.h"
#include "../libXrdIO/settingsmanager.h"
#include <QObject>
#include <QWidget>
#include <QPointer>

class AbstractToolDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AbstractToolDialog(QWidget *parent = 0);

    virtual void setProject(ProjectWidget *);
    virtual void clearProject();

protected:
    SettingsManager *settings;
    QPointer<const ProjectWidget> graphProject;
    QPointer<GraphWindow>         graphView;
    QPointer<GraphDataController> graphControl;
    Scan                          *temporaryScan;
    QMetaObject::Connection       updateConnection;

    inline virtual void preSetProject(ProjectWidget *) {}
    inline virtual void postSetProject(ProjectWidget *) {}
    inline virtual void initSettings() {}
    inline virtual void saveSettings() {}
    virtual bool checkBackend();
    inline virtual void keepTemporary() {}
    inline virtual void clearTemporary() {}
    inline virtual void clearGui() {}

    int tempScanIndex();

protected slots:
    inline virtual void updateView() {}
    void closeAndAccept();
    void closeAndReject();
};

#endif // ABSTRACTTOOLDIALOG_H
