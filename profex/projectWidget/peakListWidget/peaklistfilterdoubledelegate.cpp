/***************************************************************************
                          peaklistfilterdoubledelegate.cpp  -  description
                             -------------------
    begin                : Tue Apr 27 19:15:00 CEST 2023
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

#include "peaklistfilterdoubledelegate.h"
#include <QDoubleSpinBox>

PeakListFilterDoubleDelegate::PeakListFilterDoubleDelegate(int dec, double low, double up, double st, QObject *parent)
    : QStyledItemDelegate{parent}, _decimals(dec), _lower(low), _upper(up), _step(st)
{

}

QWidget *PeakListFilterDoubleDelegate::createEditor(QWidget *parent,
                                                 const QStyleOptionViewItem &/* option */,
                                                 const QModelIndex & index) const
{
    if (index.column() == 0) return nullptr;

    QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
    editor->setFrame(false);
    editor->setMinimum(_lower);
    editor->setMaximum(_upper);
    editor->setDecimals(_decimals);
    editor->setSingleStep(_step);

    return editor;
}

void PeakListFilterDoubleDelegate::setEditorData(QWidget *editor,
                                           const QModelIndex &index) const
{
    double value = index.model()->data(index, Qt::EditRole).toDouble();

    QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
    spinBox->setValue(value);
}

void PeakListFilterDoubleDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                          const QModelIndex &index) const
{
    if (index.column() == 0) {
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }

    QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
    spinBox->interpretText();
    double value = spinBox->value();

    model->setData(index, value, Qt::EditRole);
}

void PeakListFilterDoubleDelegate::updateEditorGeometry(QWidget *editor,
                                                  const QStyleOptionViewItem &option,
                                                  const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

QString PeakListFilterDoubleDelegate::displayText(const QVariant &value,
                                            const QLocale &locale) const
{
    bool ok;
    double v = value.toDouble(&ok);
    return ok ? locale.toString(v, 'f', _decimals) : value.toString();
}

void PeakListFilterDoubleDelegate::setLimits(double lo, double up)
{
    _lower = lo;
    _upper = up;
}
