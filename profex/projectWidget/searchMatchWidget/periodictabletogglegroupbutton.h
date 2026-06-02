/***************************************************************************
                          periodictabletogglegroupbutton.h  -  description
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

#ifndef PERIODICTABLETOGGLEGROUPBUTTON_H
#define PERIODICTABLETOGGLEGROUPBUTTON_H

#include <QObject>
#include <QToolButton>

class PeriodicTableToggleGroupButton : public QToolButton
{
    Q_OBJECT

public:
    PeriodicTableToggleGroupButton(const QString &d, int r, int c, QWidget *parent = nullptr);

    inline int status() const {return _status;}
    inline QString direction() const {return _direction;}
    inline int row() const {return _row;}
    inline int col() const {return _col;}

public slots:
    void resetStatus();

private:
    int _status;
    QString _direction;
    int _row;
    int _col;

private slots:
    void wasClicked();

signals:
    void groupButtonClicked(QString, int, int);
};

#endif // PERIODICTABLETOGGLEGROUPBUTTON_H
