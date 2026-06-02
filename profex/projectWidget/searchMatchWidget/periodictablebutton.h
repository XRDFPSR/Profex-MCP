/***************************************************************************
                          periodictablebutton.h  -  description
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

#ifndef PERIODICTABLEBUTTON_H
#define PERIODICTABLEBUTTON_H

#include <QObject>
#include <QToolButton>

class PeriodicTableButton : public QToolButton
{
    Q_OBJECT

public:
    PeriodicTableButton(const QString &e, int r, int c, QWidget *parent = nullptr);

    inline int status() const {return _status;}
    inline QString element() const {return _element;}
    inline int row() const {return _row;}
    inline int col() const {return _col;}

    void toggle();
    void setStatus(int);

public slots:
    void resetStatus();

private:
    int _status;
    QString _element;
    QColor _colDefault;
    int _row;
    int _col;

    void setColor();

private slots:
    void wasClicked();
};

#endif // PERIODICTABLEBUTTON_H
