/***************************************************************************
                          scanlistwidgetdelegate.h  -  description
                             -------------------
    begin                : Wed Apr 26 23:27:00 CEST 2023
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

#ifndef SCANLISTWIDGETDELEGATE_H
#define SCANLISTWIDGETDELEGATE_H

#include <QObject>
#include <QStyledItemDelegate>

class ScanListWidgetDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ScanListWidgetDelegate(int, double, double, double, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;

    QString displayText(const QVariant &value,
                        const QLocale &locale) const override;

private:
    int _decimals;
    double _lower;
    double _upper;
    double _step;
};

#endif // SCANLISTWIDGETDELEGATE_H
