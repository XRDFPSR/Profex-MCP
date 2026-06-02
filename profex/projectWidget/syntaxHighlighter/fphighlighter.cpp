/***************************************************************************
                          fphighlighter.cpp  -  description
                             -------------------
    begin                : Mon Oct 07 22:00:00 CEST 2013
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

#include "fphighlighter.h"
#include <QDebug>

FpHighlighter::FpHighlighter(QObject *parent) :
    QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // comments
    fmt.setForeground(Qt::darkRed);
    fmt.setFontWeight(QFont::Normal);
    fmt.setFontItalic(true);
    rule.pattern = QRegularExpression("![^\n]*$");
    rule.format = fmt;
    highlightingRules.append(rule);
}

void FpHighlighter::highlightBlock(const QString &text)
 {
     foreach (const HighlightingRule &rule, highlightingRules) {
         QRegularExpression expression(rule.pattern);
         QRegularExpressionMatchIterator it = expression.globalMatch(text);

         while (it.hasNext()) {
            QRegularExpressionMatch rm = it.next();
            setFormat(rm.capturedStart(1), rm.capturedLength(1), rule.format);
         }
     }
}
