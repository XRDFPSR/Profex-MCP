/***************************************************************************
                          stabusbarlabel.h  -  description
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

#ifndef STATUSBARLABEL_H
#define STATUSBARLABEL_H

#include <QLabel>

class StatusBarLabel : public QLabel
{
    Q_OBJECT

public:
    StatusBarLabel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    void setMinWidth(const QString &);

private:
    void mouseDoubleClickEvent(QMouseEvent *);

signals:
    void sigDoubleClicked();
};

#endif // STATUSBARLABEL_H
