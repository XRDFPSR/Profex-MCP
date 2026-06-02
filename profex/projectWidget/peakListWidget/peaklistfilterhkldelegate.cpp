/***************************************************************************
                          peaklistfilterhkldelegate.cpp  -  description
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

#include "peaklistfilterhkldelegate.h"
#include <QSpinBox>

PeakListFilterHklDelegate::PeakListFilterHklDelegate(int low, int up, QObject *parent)
    : QStyledItemDelegate{parent}, _lower(low), _upper(up)
{

}

QWidget *PeakListFilterHklDelegate::createEditor(QWidget *parent,
                                                 const QStyleOptionViewItem &/* option */,
                                                 const QModelIndex & index) const
{
    if (index.column() == 0) return nullptr;

    QSpinBox *editor = new QSpinBox(parent);
    editor->setFrame(false);
    editor->setMinimum(_lower);
    editor->setMaximum(_upper);

    return editor;
}

void PeakListFilterHklDelegate::setEditorData(QWidget *editor,
                                           const QModelIndex &index) const
{
    int value = index.model()->data(index, Qt::EditRole).toInt();

    QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
    spinBox->setValue(value);
}

void PeakListFilterHklDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                          const QModelIndex &index) const
{
    if (index.column() == 0) {
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }

    QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
    spinBox->interpretText();
    int value = spinBox->value();

    model->setData(index, value, Qt::EditRole);
}

void PeakListFilterHklDelegate::updateEditorGeometry(QWidget *editor,
                                                  const QStyleOptionViewItem &option,
                                                  const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

QString PeakListFilterHklDelegate::displayText(const QVariant &value,
                                            const QLocale &locale) const
{
    bool ok;
    int v = value.toInt(&ok);
    return ok ? locale.toString(v) : value.toString();
}

void PeakListFilterHklDelegate::setLimits(int lo, int up)
{
    _lower = lo;
    _upper = up;
}
