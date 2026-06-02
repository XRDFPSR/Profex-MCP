/***************************************************************************
                          fphighlighter.h  -  description
                             -------------------
    begin                : Sun Oct 07 22:00:00 CEST 2013
    copyright            : (C) 2013 by Nicola Doebelin
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

#ifndef FPHIGHLIGHTER_H
#define FPHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

class FpHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit FpHighlighter(QObject *parent = 0);

protected:
     void highlightBlock(const QString &text);

private:
     struct HighlightingRule
     {
         QRegularExpression pattern;
         QTextCharFormat format;
     };

     QVector<HighlightingRule> highlightingRules;
     QTextCharFormat fmt;
};

#endif // FPHIGHLIGHTER_H
