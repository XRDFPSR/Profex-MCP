/***************************************************************************
                          stabusbarlabel.cpp  -  description
                             -------------------
    begin                : Wed Jul 06 21:30:00 CEST 2022
    copyright            : (C) 2022 by Nicola Doebelin
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

#include "statusbarlabel.h"
#include <QFontMetrics>

StatusBarLabel::StatusBarLabel(QWidget *parent, Qt::WindowFlags f)
    : QLabel(parent, f)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
}

void StatusBarLabel::setMinWidth(const QString &s)
{
    setMinimumWidth(fontMetrics().boundingRect(s).width());
}

void StatusBarLabel::mouseDoubleClickEvent(QMouseEvent *)
{
    emit sigDoubleClicked();
}
