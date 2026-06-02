/***************************************************************************
                          copycontrolfiledialog.h  -  description
                             -------------------
    begin                : Tue Sep 03 08:00:00 CEST 2013
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

#ifndef PROJECTSELECTDIALOG_H
#define PROJECTSELECTDIALOG_H

#include <QDialog>
#include <QUuid>
#include "projectWidget/projectwidget.h"

namespace Ui {
class ProjectSelectDialog;
}

class ProjectSelectDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ProjectSelectDialog(QWidget *parent = 0);
    ~ProjectSelectDialog();

    void setFiles(const QList<ProjectWidget *> &, const QList<QUuid> &);
    void setCurrentUid(const QUuid &);
    QList<QUuid> getChecked();

    void showFollowActiveBox(bool);
    void setFollowActiveRefinement(bool);
    bool getFollowActiveRefinement();
    
private:
    Ui::ProjectSelectDialog *ui;
    QUuid currentUid;

private slots:
    void checkAll();
    void checkCurrent();
    void checkNone();
};

#endif // PROJECTSELECTDIALOG_H
