/***************************************************************************
                          bgmnhighlighter.h  -  description
                             -------------------
    begin                : Sun Oct 06 11:00:00 CEST 2013
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

#ifndef BGMNHIGHLIGHTER_H
#define BGMNHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

enum HlMode {LIGHT, DARK, NONE};
const int BCOL = 180;

class BgmnHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit BgmnHighlighter(int m, QObject *parent = 0);

    void setMode(int);

protected:
     void highlightBlock(const QString &text);

private:
     struct HighlightingRule
     {
         QRegularExpression expression;
         QTextCharFormat format;
     };

     QVector<HighlightingRule> highlightingRules;
     QVector<HighlightingRule> errorRules;
     QString numeric;
     HlMode mode;

     void init();
     void appendKeywords();
     void appendAnisoKeywords();
     void appendComments();
     void appendStructureKeywords();
     void appendNumericValues();
     void appendLimits();
     void appendGoals();
     void appendFileNames();
     void appendParam();
     void appendErrors();
};

#endif // BGMNHIGHLIGHTER_H
