/***************************************************************************
                          periodictabletogglegroupbutton.cpp  -  description
                             -------------------
    begin                : Mon Aug 03 20:55:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#include "periodictabletogglegroupbutton.h"
#include <QFontMetrics>

PeriodicTableToggleGroupButton::PeriodicTableToggleGroupButton(const QString &d, int r, int c, QWidget *parent)
    : QToolButton(parent), _direction(d), _row(r), _col(c)
{
    QFontMetrics f(font());
    setMinimumWidth(f.horizontalAdvance("NN"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    _status = 0;

    connect(this, SIGNAL(clicked(bool)), this, SLOT(wasClicked()));

    if (r == 0) {
        if (c < 3) setText(QString("%1").arg(c));
        else       setText(QString("%1").arg(c - 1));
    } else if (c == 0) {
        if (r < 8) setText(QString("%1").arg(r));
        else if (r == 9)  setText("L");
        else if (r == 10) setText("A");
    }
}

void PeriodicTableToggleGroupButton::wasClicked()
{
    ++_status;
    if (_status > 3) _status = 0;

    emit groupButtonClicked(_direction, _direction == "c" ? _col : _row, status());
}

void PeriodicTableToggleGroupButton::resetStatus()
{
    _status = 0;
}

