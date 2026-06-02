/***************************************************************************
                          graphwindowkeyeventhandler.h  -  description
                             -------------------
    begin                : Die July 19 2023
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

#ifndef GRAPHWINDOWKEYEVENTHANDLER_H
#define GRAPHWINDOWKEYEVENTHANDLER_H

#include <QKeyEvent>

class GraphWindow;

class GraphWindowKeyEventHandler
{
public:
    GraphWindowKeyEventHandler(GraphWindow *);

    void keyPressed(QKeyEvent *);
    void keyReleased(QKeyEvent *);
    inline QString getKeyCommand() const {return _keyCommand;}
    inline bool hasKeyCommand() const {return !_keyCommand.isEmpty();}

private:
    GraphWindow *graph;
    QString _keyCommand;

    void updateKeyCommand(QKeyEvent *);
    void keysArrows(QKeyEvent *);
    void keysPlusMinus(QKeyEvent *);

    void key0(QKeyEvent *);
    void keyC(QKeyEvent *);
    void keyN(QKeyEvent *);
    void keyS(QKeyEvent *);
    void keyI(QKeyEvent *);
    void keyEsc(QKeyEvent *);
    void keyZ(QKeyEvent *);
    void keyQ(QKeyEvent *);
    void keyW(QKeyEvent *);

    void keySpace(QKeyEvent *e);
    void keyHome(QKeyEvent *e);
    void keyEnd(QKeyEvent *e);

};

#endif // GRAPHWINDOWKEYEVENTHANDLER_H
