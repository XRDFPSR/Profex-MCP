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

#ifndef WAVELENGTHCOMBOBOX_H
#define WAVELENGTHCOMBOBOX_H

#include <QObject>
#include <QWidget>
#include <QComboBox>

class WaveLengthComboBox : public QComboBox
{
public:
    WaveLengthComboBox(QWidget *parent = nullptr);

    void initData(bool withValues = false);

    void showKa2(bool);
    void showKb(bool);

    QStringList getLabels() const;
    QList<double> getData() const;

private:
    bool _showKa2;
    bool _showKb;
};

#endif // WAVELENGTHCOMBOBOX_H
