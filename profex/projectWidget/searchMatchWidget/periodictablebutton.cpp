/***************************************************************************
                          periodictablebutton.cpp  -  description
                             -------------------
    begin                : Mon Aug 03 18:21:00 CEST 2020
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

#include "periodictablebutton.h"
#include <QGuiApplication>
#include <QString>
#include <QFontMetrics>
#include <QStyleFactory>
#include <QDebug>

PeriodicTableButton::PeriodicTableButton(const QString &e, int r, int c, QWidget *parent)
    : QToolButton(parent), _element(e), _row(r), _col(c)
{
    QFontMetrics f(font());
    setMinimumWidth(f.horizontalAdvance("NN"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setAutoFillBackground(true);
    _colDefault = palette().color(QPalette::Button);
    setStyle(QStyleFactory::create("fusion"));

    setText(_element.left(1).toUpper() + (_element.length() > 1 ? _element.mid(1, 1).toLower() : QString()));
    _status = 0;

    connect(this, SIGNAL(clicked(bool)), this, SLOT(wasClicked()));
}

void PeriodicTableButton::wasClicked()
{
    toggle();
}

void PeriodicTableButton::toggle()
{
    ++_status;
    if (_status > 3) _status = 0;
    setColor();
}

void PeriodicTableButton::setStatus(int n)
{
    _status = n;
    setColor();
}

void PeriodicTableButton::resetStatus()
{
    _status = 0;
    setColor();
}

void PeriodicTableButton::setColor()
{
    QPalette pal = palette();

    if      (_status == 1) pal.setColor(QPalette::Button, QColor("#7eeb86"));
    else if (_status == 2) pal.setColor(QPalette::Button, QColor("#7ec3eb"));
    else if (_status == 3) pal.setColor(QPalette::Button, QColor("#eb7e7e"));
    else                   pal.setColor(QPalette::Button, _colDefault);

    setPalette(pal);
}
