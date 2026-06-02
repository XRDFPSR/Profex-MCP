/***************************************************************************
                          abstractgraphview.cpp  -  description
                             -------------------
    begin                : Tue Feb 17 16:40:00 CEST 2020
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

#include "abstractgraphview.h"
#include "graphdatacontroller.h"

AbstractGraphView::AbstractGraphView(GraphDataController *c, QWidget *parent)
    : QWidget(parent)
{
    scanControl = c;
    settings = SettingsManager::getInstance();
    isShown = false;
    uid = QUuid::createUuid();

    // all widgets update at the end of the refinement
    vModes.insert(global::ViewUpdateMode::RESULTS);
}
