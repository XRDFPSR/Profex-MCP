/***************************************************************************
                          abstracttooldialog.cpp  -  description
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

#include "abstracttooldialog.h"

AbstractToolDialog::AbstractToolDialog(QWidget *parent) :
    QDialog(parent)
{
    settings = SettingsManager::getInstance();
    graphProject = nullptr;
    graphView = nullptr;
    graphControl = nullptr;
    temporaryScan = nullptr;
}

void AbstractToolDialog::setProject(ProjectWidget *p)
{
    preSetProject(p);

    if (updateConnection) disconnect(updateConnection);

    if (p) {
        graphProject = p;

        graphView = p->scanView();
        graphControl = p->dataControl();

        updateConnection = connect(graphControl, &GraphDataController::dataUpdated, this, &AbstractToolDialog::updateView);
        postSetProject(p);
    }
}

bool AbstractToolDialog::checkBackend()
{
    if (!graphView || !graphControl) {
        return false;
    }

    return true;
}

void AbstractToolDialog::clearProject()
{
    graphView = nullptr;
    graphControl = nullptr;
    clearGui();
}

void AbstractToolDialog::closeAndAccept()
{
    keepTemporary();
    saveSettings();
    QDialog::close();
}

void AbstractToolDialog::closeAndReject()
{
    clearTemporary();
    QDialog::close();
}

int AbstractToolDialog::tempScanIndex()
{
    if (!checkBackend()) return -1;
    if (!graphControl->hasData()) return -1;

    for (int i = 0; i < graphControl->count(); ++i) {
        if (graphControl->at(i)->isTemporary()) return i;
    }

    return -1;

}
