/***************************************************************************
                          spacegroupdialog.cpp  -  description
                             -------------------
    begin                : Feb 07 11:00:00 CEST 2013
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

#ifndef SPACEGROUPDIALOG_H
#define SPACEGROUPDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QElapsedTimer>

namespace Ui {
class SpacegroupDialog;
}

class SpacegroupDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit SpacegroupDialog(QWidget *parent = 0);
    ~SpacegroupDialog();

    void setDirectory(const QString &);
    bool hasData() {return hdata;}
    void clear();
    
private:
    Ui::SpacegroupDialog *ui;
    bool hdata;
    QElapsedTimer time;

    QStandardItemModel *model;
    QSortFilterProxyModel *spgrModel;
    QSortFilterProxyModel *hmModel;
    QSortFilterProxyModel *wyckModel;
    QSortFilterProxyModel *symmModel;

    void parseModel(const QString &);
    QString getSelectedWyckoffText();
    QModelIndex selectWyckoff(const QString &);

private slots:
    void spgrSelectionChanged(const QModelIndex &current, const QModelIndex &);
    void hmSelectionChanged(const QModelIndex &current, const QModelIndex &);
    void wyckoffSelectionChanged(const QModelIndex &current, const QModelIndex &);
    void spgrFilterChanged(int);
    void hmFilterChanged(int);
};

#endif // SPACEGROUPDIALOG_H

