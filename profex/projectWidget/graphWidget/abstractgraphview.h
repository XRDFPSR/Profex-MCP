/***************************************************************************
                          abstractgraphview.h  -  description
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

#ifndef ABSTRACTGRAPHVIEW_H
#define ABSTRACTGRAPHVIEW_H

#include "../libXrdIO/settingsmanager.h"
#include "../libXrdIO/structs.h"
#include <QWidget>
#include <QUuid>
#include <QDomDocument>

class GraphDataController;

class AbstractGraphView : public QWidget
{
    Q_OBJECT
public:
    explicit AbstractGraphView(GraphDataController *c, QWidget *parent = nullptr);

    /*
     * set the view status:
     */
    virtual bool setStatus(const global::RefinementStatus &) {return false;}

    /*
     * sets the variable isShown.
     * If false, calls of updateView will be ignored.
     * if true, calls of updateView will update the widget.
     */
    virtual inline void setShown(bool b) {isShown = b;}

    /*
     * returns the modes for view updates.
     * global::ViewUpdateMode::RESULTS: Only updates at the end of the refinement or when the results changed
     * global::ViewUpdateMode::DISPLAY: Updates during refinements or when the graph content changes
     */
    inline QSet<global::ViewUpdateMode> viewModes() const {return vModes;}

    /*
     * inserts an xml element to the document to store its settings
     */
    virtual void getPreset(QDomDocument &doc) = 0;

    /*
     * restores its settings from the xml element
     */
    virtual void applyPreset(const QDomElement &element) = 0;

public slots:
    virtual void updateView() = 0;
    virtual void resetView() {}

protected:
    GraphDataController *scanControl;
    SettingsManager *settings;
    bool isShown;
    QUuid uid;
    QSet<global::ViewUpdateMode> vModes;
};

#endif // ABSTRACTGRAPHVIEW_H
