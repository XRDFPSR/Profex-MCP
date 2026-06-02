/***************************************************************************
                          wavelengthcombobox.h  -  description
                             -------------------
    begin                : Tue Feb 23 19:00:00 CET 2021
    copyright            : (C) 2021 by Nicola Doebelin
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

#include "wavelengthcombobox.h"
#include "../libXrdIO/functions.h"
#include "../libXrdIO/structs.h"

WaveLengthComboBox::WaveLengthComboBox(QWidget *parent)
    : QComboBox(parent)
{
    _showKa2 = false;
    _showKb  = false;
}

void WaveLengthComboBox::showKa2(bool b)
{
    _showKa2 = b;
}

void WaveLengthComboBox::showKb(bool b)
{
    _showKb = b;
}

void WaveLengthComboBox::initData(bool withValues)
{
    QString ct = this->currentText();
    bool oldState = this->blockSignals(true);

    clear();

    QList<global::CharWaveLength> wl = global::Functions::getAllWavelengths();

    for (int i = 0; i < wl.size(); ++i) {
        QString valKa1 = QString(" (%1 %2)").arg(10.0 * wl.at(i).ka1, 0, 'f', 6).arg(global::angstrom);
        QString lblKa1 = QString("%1K%2%3").arg(wl.at(i).element).arg(global::alpha).arg(global::subOne);
        if (withValues) lblKa1 += valKa1;
        this->addItem(lblKa1, wl.at(i).ka1);

        if (_showKa2) {
            QString valKa2 = QString(" (%1 %2)").arg(10.0 * wl.at(i).ka2, 0, 'f', 6).arg(global::angstrom);
            QString lblKa2 = QString("%1K%2%3").arg(wl.at(i).element).arg(global::alpha).arg(global::subTwo);
            if (withValues) lblKa2 += valKa2;
            this->addItem(lblKa2, wl.at(i).ka2);
        }

        if (_showKb) {
            QString valKb = QString(" (%1 %2)").arg(10.0 * wl.at(i).kb, 0, 'f', 6).arg(global::angstrom);
            QString lblKb = QString("%1K%2").arg(wl.at(i).element).arg(global::beta);
            if (withValues) lblKb += valKb;
            this->addItem(lblKb, wl.at(i).kb);
        }
    }

    if (!ct.isEmpty()) this->setCurrentText(ct);
    this->blockSignals(oldState);
}

QStringList WaveLengthComboBox::getLabels() const
{
    QStringList l;

    for (int i = 0; i < this->count(); ++i) {
        l.append(this->itemText(i));
    }

    return l;
}

QList<double> WaveLengthComboBox::getData() const
{
    QList<double> l;

    for (int i = 0; i < this->count(); ++i) {
        l.append(this->itemData(i, Qt::UserRole).toDouble());
    }

    return l;
}
