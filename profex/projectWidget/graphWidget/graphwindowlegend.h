/***************************************************************************
                          graphwindowlegend.h  -  description
                             -------------------
    begin                : Mon Feb 01 2021
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

#ifndef GRAPHWINDOWLEGEND_H
#define GRAPHWINDOWLEGEND_H

#include <QVector>
#include <QMenu>
#include "abstractgraphview.h"

class GraphWindowLegend : public AbstractGraphView
{
    Q_OBJECT
public:
    explicit GraphWindowLegend(GraphDataController *c, QWidget *parent = nullptr);

    inline void getPreset(QDomDocument &) override {};
    inline void applyPreset(const QDomElement &) override {};

    void initSettings();
    void redraw(QPainter &p, const QFont &f, const QColor &bg, const QRect &plt);
    const QRect boundingRect() const {return bRect;}
    void showContextMenu(QMouseEvent *);
    inline bool isVisible() {return isDrawn;}
    inline bool isBottomUp() {return sortBottomUp;}

public slots:
    void updateView() override;

private:
    QList<QColor> standardColors;
    QList<QColor> colors;
    QList<int> lineWidths;
    QStringList labels;
    QRect bRect;
    QColor bgColor;
    QMenu *contextMenuLegend;
    QAction *actLegendShow;
    QAction *actLegendFrame;
    QAction *actLegendFill;
    QAction *actSortBottomUp;

    int lineWidth;
    bool isDrawn;
    bool boundingBoxIsDrawn;
    bool backgroundIsFilled;
    bool drawPhaseLabels;
    bool sortBottomUp;

    int lTextWidth; // length of the longest text entry
    int lLineSpacing;  // distance between lines in the legend
    int lTopBoundingBox;   // offset from top
    int lRightBoundingBox; // offset from the right
    int lLineLength;   // length of the lines
    int lHorizontalSpacing;   // distance between lines and text

    int lLeftBoundingBox; // left end of the legend
    int lLeftText; // left end of the text
    int lTopText;  // base line of the first line

    void updateBoundingRect(const QFontMetrics &, const QRect &);
    void drawBackground(QPainter &p);
    void drawContent(QPainter &p, const QFont &, const QFontMetrics &);

private slots:
    void toggleLegendVisibility(bool);
    void toggleLegendFrame(bool);
    void toggleLegendFill(bool);
    void toggleSortBottomUp(bool);

signals:
    void sigRequestRedraw();
};

#endif // GRAPHWINDOWLEGEND_H
