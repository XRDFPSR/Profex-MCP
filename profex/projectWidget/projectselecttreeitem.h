/***************************************************************************
                          projecselecttreetitem.h  -  description
                             -------------------
    begin                : Thu Mai 11 18:00:00 CEST 2023
    copyright            : (C) 2023 by Nicola Doebelin
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


#ifndef PROJECTSELECTTREEITEM_H
#define PROJECTSELECTTREEITEM_H

#include <QTreeWidgetItem>
#include <QUuid>
// #include "projectwidget.h"
#include "../libXrdIO/structs.h"

class ProjectWidget;

class ProjectSelectTreeItem : public QTreeWidgetItem
{
public:
    explicit ProjectSelectTreeItem(const QUuid &u, ProjectWidget *p);
    inline ProjectWidget *projectWidget() {return _pw;}
    inline QUuid getUid() const  {return _uid;}

    void setProjectName(const QString &s);
    void setProjectType(const QString &s);
    void setProjectFileName(const QString &s);
    void setProjectFilePath(const QString &s);
    void setProjectSampleId(const QString &s);
    void setStatus(const global::RefinementStatus &s);
    void setRefinementStats(int ci, int mi, double rwp, double rexp);

    QString getProjectName() const;
    QString getProjectType() const;
    QString getProjectFileName() const;
    QString getProjectFilePath() const;
    QString getProjectSampleId() const;

private:
    QUuid _uid;
    ProjectWidget *_pw;
    QTreeWidgetItem *itemProjectType;
    QTreeWidgetItem *itemProjectFileName;
    QTreeWidgetItem *itemProjectFilePath;
    QTreeWidgetItem *itemProjectSampleId;

    global::RefinementStatus _refStatus;
    QString _refStatusText;
    double _chi2;
    int    _curIteration;
    int    _maxIterations;

    QColor chiColor(double chival, bool light);
    void updateStatusText();
};


#endif // PROJECTSELECTTREEITEM_H
