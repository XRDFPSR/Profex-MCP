/***************************************************************************
                          scanlistwidgetdelegate.cpp  -  description
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

#include "scanlistwidgetdelegate.h"
#include <QDoubleSpinBox>

ScanListWidgetDelegate::ScanListWidgetDelegate(int dec, double low, double up, double st, QObject *parent)
    : _decimals(dec), _lower(low), _upper(up), _step(st), QStyledItemDelegate{parent}
{

}

QWidget *ScanListWidgetDelegate::createEditor(QWidget *parent,
                                       const QStyleOptionViewItem &/* option */,
                                       const QModelIndex &/* index */) const
{
    QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
    editor->setFrame(false);
    editor->setMinimum(_lower);
    editor->setMaximum(_upper);
    editor->setDecimals(_decimals);
    editor->setSingleStep(_step);

    return editor;
}

void ScanListWidgetDelegate::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
    double value = index.model()->data(index, Qt::EditRole).toDouble();

    QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
    spinBox->setValue(value);
}

void ScanListWidgetDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
    spinBox->interpretText();
    double value = spinBox->value();

    model->setData(index, value, Qt::EditRole);
}

void ScanListWidgetDelegate::updateEditorGeometry(QWidget *editor,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

QString ScanListWidgetDelegate::displayText(const QVariant &value,
                                            const QLocale &locale) const
{
    return locale.toString(value.toDouble(), 'f', _decimals);
}
