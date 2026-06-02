/***************************************************************************
                          graphwindowkeyeventhandler.cpp  -  description
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

#include <QToolTip>
#include <QDebug>
#include "graphwindowkeyeventhandler.h"
#include "graphwindow.h"
#include "../libXrdIO/structs.h"

GraphWindowKeyEventHandler::GraphWindowKeyEventHandler(GraphWindow *g)
{
    graph = g;
}

void GraphWindowKeyEventHandler::keyPressed(QKeyEvent *e)
{
    // building a key sequence is active, capture all key strokes for the sequence
    if (!_keyCommand.isEmpty()) {
        updateKeyCommand(e);
        return;
    }

    switch (e->key()) {
    case Qt::Key_Left:
        keysArrows(e);
        break;
    case Qt::Key_Right:
        keysArrows(e);
        break;
    case Qt::Key_Up:
        keysArrows(e);
        break;
    case Qt::Key_Down:
        keysArrows(e);
        break;
    case Qt::Key_Plus:
        keysPlusMinus(e);
        break;
    case Qt::Key_Minus:
        keysPlusMinus(e);
        break;
    case Qt::Key_Space:
        keySpace(e);
        break;
    case Qt::Key_0:
        key0(e);
        break;
    case Qt::Key_C:
        keyC(e);
        break;
    case Qt::Key_I:
        keyI(e);
        break;
    case Qt::Key_N:
        keyN(e);
        break;
    case Qt::Key_Q:
        keyQ(e);
        break;
    case Qt::Key_S:
        keyS(e);
        break;
    case Qt::Key_W:
        keyW(e);
        break;
    case Qt::Key_Z:
        keyZ(e);
        break;
    case Qt::Key_Escape:
        keyEsc(e);
        break;
    case Qt::Key_Home:
        keyHome(e);
        break;
    case Qt::Key_End:
        keyEnd(e);
        break;
    default:
        e->ignore();
        break;
    }
}

void GraphWindowKeyEventHandler::keyReleased(QKeyEvent *e)
{
    if (graph->dragging) {
        graph->dragging = false;
        e->accept();
        graph->update();
    } else {
        e->ignore();
    }
}

void GraphWindowKeyEventHandler::updateKeyCommand(QKeyEvent *e)
{
    if (e->isAutoRepeat()) {e->ignore(); return;}

    switch (e->key()) {
    case Qt::Key_Return:
        graph->applyKeySequence(_keyCommand);
        _keyCommand.clear();
        break;
    case Qt::Key_Enter:
        graph->applyKeySequence(_keyCommand);
        _keyCommand.clear();
        break;
    case Qt::Key_Backspace:
        _keyCommand.remove(_keyCommand.size() - 1, 1);
        break;
    case Qt::Key_Escape:
        _keyCommand.clear();
        break;
    default:
        _keyCommand.append(e->text());
        break;
    }

    e->accept();
    graph->update();
}

void GraphWindowKeyEventHandler::keysArrows(QKeyEvent *e)
{
    // warning: Ctrl + arrows is used by MainWindow
    if (e->modifiers() & Qt::ControlModifier) {
        e->ignore();
        return;
    }

    double frac = 0.1;
    if (e->modifiers() & Qt::AltModifier)   frac = 1.0;
    if (e->modifiers() & Qt::ShiftModifier) frac = 0.01;

    switch (e->key()) {
    case Qt::Key_Left:
        graph->moveLeft(frac);
        e->accept();
        break;
    case Qt::Key_Right:
        graph->moveRight(frac);
        e->accept();
        break;
    case Qt::Key_Up:
        graph->moveUp(frac);
        e->accept();
        break;
    case Qt::Key_Down:
        graph->moveDown(frac);
        e->accept();
        break;
    default:
        e->ignore();
        break;
    }
}

void GraphWindowKeyEventHandler::keysPlusMinus(QKeyEvent *e)
{
    double step = 0.2;
    if (e->modifiers() & Qt::AltModifier)   step = 0.5;
    if (e->modifiers() & Qt::ShiftModifier) step = 0.02;
    bool vertical = (e->modifiers() & Qt::ControlModifier);

    switch (e->key()) {
    case Qt::Key_Plus:
        if (vertical) graph->zoomStepIntensity(1.0 - step);
        else          graph->zoomStepAngle(1.0 - step);
        e->accept();
        break;
    case Qt::Key_Minus:
        if (vertical) graph->zoomStepIntensity(1.0 + step);
        else          graph->zoomStepAngle(1.0 + step);
        e->accept();
        break;
    default:
        e->ignore();
        break;
    }
}

void GraphWindowKeyEventHandler::key0(QKeyEvent *e)
{
    if (e->isAutoRepeat()) {e->ignore(); return;}

    if (e->modifiers() & Qt::ControlModifier) {
        graph->togglePhaseVisibility();
        e->accept();
    } else {
        e->ignore();
    }
}

void GraphWindowKeyEventHandler::keyC(QKeyEvent *e)
{
    if (e->isAutoRepeat()) {e->ignore(); return;}

    graph->crossHair = !graph->crossHair;
    graph->changeCursor();
    e->accept();
    graph->update();
}

void GraphWindowKeyEventHandler::keyN(QKeyEvent *e)
{
    if (e->isAutoRepeat()) {e->ignore(); return;}

    graph->noiseCursor = !graph->noiseCursor;
    graph->changeCursor();
    e->accept();
    graph->update();

}

void GraphWindowKeyEventHandler::keyS(QKeyEvent *e)
{
    if (e->isAutoRepeat()) {e->ignore(); return;}

    graph->specLines = !graph->specLines;
    graph->changeCursor();
    e->accept();
    graph->update();
}

void GraphWindowKeyEventHandler::keyI(QKeyEvent *e)
{
    if (e->isAutoRepeat()) {e->ignore(); return;}

    graph->inspector = !graph->inspector;
    graph->changeCursor();
    e->accept();
    graph->update();
}

void GraphWindowKeyEventHandler::keyQ(QKeyEvent *e)
{
    if (e->isAutoRepeat()) {e->ignore(); return;}

    // pressing q starts building a key sequence for centering
    _keyCommand = "q";
    e->accept();
    graph->update();
}

void GraphWindowKeyEventHandler::keyW(QKeyEvent *e)
{
    if (e->isAutoRepeat()) {e->ignore(); return;}

    // pressing w starts building a key sequence for width adjustment
    _keyCommand = "w";
    e->accept();
    graph->update();
}

void GraphWindowKeyEventHandler::keyZ(QKeyEvent *e)
{
    if (e->isAutoRepeat()) {e->ignore(); return;}

    // pressing z starts building a key sequence for zooming
    _keyCommand = "z";
    e->accept();
    graph->update();
}

void GraphWindowKeyEventHandler::keyEsc(QKeyEvent *e)
{
    if ((graph->peakPreviewMode != PPMNONE) || (graph->rangeSelectMode)) {
        graph->firstDoubleClickPoint = QPoint();
        e->accept();
        graph->update();
        return;
    } else {
        e->ignore();
    }
}

void GraphWindowKeyEventHandler::keySpace(QKeyEvent *e)
{
    // warning: Ctrl + Space is used by MainWindow
    if (e->modifiers() & Qt::ControlModifier) {
        e->ignore();
        return;
    }

    if (e->isAutoRepeat()) {e->ignore(); return;}

    graph->resetZoom();
    graph->forceUpdate();
    e->accept();
}

void GraphWindowKeyEventHandler::keyHome(QKeyEvent *e)
{
    if (e->isAutoRepeat()) {e->ignore(); return;}
    graph->moveToStart();
    e->accept();
}

void GraphWindowKeyEventHandler::keyEnd(QKeyEvent *e)
{
    if (e->isAutoRepeat()) {e->ignore(); return;}
    graph->moveToEnd();
    e->accept();
}
