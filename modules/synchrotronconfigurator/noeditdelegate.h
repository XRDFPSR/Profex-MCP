/***************************************************************************
                          noeditdelegate.h  -  description
                             -------------------
    begin                : Thu Jan 09 18:00:00 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#ifndef NOEDITDELEGATE_H
#define NOEDITDELEGATE_H

#include <QStyledItemDelegate>

/*
 * This class is used to disable editing of certain columns in a QTreeWidget.
 * Set the items to Qt::ItemIsEditable, then disable the columns that are not
 * editable with:
 *
 * ui->parameterView->setItemDelegateForColumn(0, new NoEditDelegate(this));
 */

class NoEditDelegate: public QStyledItemDelegate {
public:
    NoEditDelegate(QObject* parent=0): QStyledItemDelegate(parent) {}
    virtual QWidget* createEditor(QWidget *, const QStyleOptionViewItem &, const QModelIndex &) const {
        return nullptr;
    }
};

#endif // NOEDITDELEGATE_H
