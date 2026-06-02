/***************************************************************************
                          bgmnhighlighter.cpp  -  description
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

#include "bgmnhighlighter.h"
#include <QDebug>
#include <QRegularExpressionMatch>

BgmnHighlighter::BgmnHighlighter(int m, QObject *parent) :
    QSyntaxHighlighter(parent)
{
    mode = LIGHT;
    if (m == 1) mode = DARK;
    if (m == 2) mode = NONE;

    numeric = "[\\+-]?(?:\\d+\\.?\\d*)(?:E[\\+-]\\d+)?";
    init();
}

void BgmnHighlighter::setMode(int m)
{
    if (m == 0) mode = LIGHT;
    if (m == 1) mode = DARK;
    if (m == 2) mode = NONE;
    init();
}

void BgmnHighlighter::init()
{
    highlightingRules.clear();

    if (mode == NONE) return;

    appendKeywords();
    appendAnisoKeywords();
    appendStructureKeywords();
    appendNumericValues();
    appendLimits();
    appendGoals();
    appendParam();
    appendFileNames();
    appendComments();
    appendErrors();
}

void BgmnHighlighter::appendKeywords()
{
    QTextCharFormat fmt;
    QStringList keywords;

    if (mode == LIGHT) fmt.setForeground(Qt::darkBlue);
    if (mode == DARK)  fmt.setForeground(QColor(BCOL, BCOL, 255));

    keywords << "NTHREADS" << "VAL" << "VERZERR" << "STRUC" << "STRUCOUT"
                    << "SimpleSTRUCOUT" << "PDBOUT" << "RESOUT" << "FCFOUT"
                    << "OUTPUT" << "LIST" << "RU" << "UNT" << "UNTC" << "DDM"
                    << "LAMBDA" << "SYNCHROTRON" << "EPS\\d"
                    << "POL" << "PROTOKOLL" << "ONLYISO" << "ITMAX" << "DIAGRAMM"
                    << "PLAN" << "STANDALONEPLAN" << "WMIN" << "WMAX"
                    << "CUT" << "LIMIT\\d+" << "ANISOLIMIT" << "ANISO4LIMIT" << "SAVE";

    foreach (const QString &pattern, keywords) {
        HighlightingRule rule;
        rule.expression = QRegularExpression(QString("(%1(?:\\[\\d+\\])?)").arg(pattern));
        rule.format = fmt;
        highlightingRules.append(rule);
    }
}

void BgmnHighlighter::appendComments()
{
    QTextCharFormat fmt;
    HighlightingRule rule;

    if (mode == LIGHT) fmt.setForeground(Qt::darkGray);
    if (mode == DARK)  fmt.setForeground(Qt::lightGray);

    rule.expression = QRegularExpression("((//|%).*$)");
    rule.format = fmt;
    highlightingRules.append(rule);
}

void BgmnHighlighter::appendAnisoKeywords()
{
    QTextCharFormat fmt;
    QStringList keywords;

    if (mode == LIGHT) fmt.setForeground(Qt::blue);
    if (mode == DARK)  fmt.setForeground(QColor(BCOL, BCOL, 255));

    keywords << "ANISO\\d?" << "ANISOLIN" << "ANISOSQR" << "SPHAR\\d+";

    foreach (const QString &pattern, keywords) {
        HighlightingRule rule;
        rule.expression = QRegularExpression(QString("(%1)").arg(pattern));
        rule.format = fmt;
        highlightingRules.append(rule);
    }
}

void BgmnHighlighter::appendStructureKeywords()
{
    QTextCharFormat fmt;
    QStringList keywords;

    if (mode == LIGHT) fmt.setForeground(Qt::darkBlue);
    if (mode == DARK)  fmt.setForeground(QColor(BCOL, BCOL, 255));

    keywords << "B1" << "B2" << "k1" << "k2" << "k3" << "PHASE" << "Reference" << "Formula"
             << "sk" << "H" << "h" << "k" << "l" << "zweiTheta" << "RP"
             << "SpacegroupNo" << "HermannMauguin" << "GeneralCondition" << "Setting"
             << "Lattice" << "CellChoice" << "UniqueAxis"
             << "A" << "B" << "C" << "ALPHA" << "BETA" << "GAMMA" << "UNIT" << "GEWICHT"
             << "E" << "Wyckoff" << "x" << "y" << "z" << "TDS";

    foreach (const QString &pattern, keywords) {
        HighlightingRule rule;
        rule.expression = QRegularExpression(QString("((\\s|^|=)%1(?:\\[\\d+\\])?=)").arg(pattern));
        rule.format = fmt;
        highlightingRules.append(rule);
    }
}

void BgmnHighlighter::appendNumericValues()
{
    QTextCharFormat fmt;
    HighlightingRule rule;

    if (mode == LIGHT) fmt.setForeground(Qt::darkRed);
    if (mode == DARK)  fmt.setForeground(QColor(255, BCOL, BCOL));

    rule.expression = QRegularExpression(QString("(=%1)").arg(numeric));
    rule.format = fmt;
    highlightingRules.append(rule);
}

void BgmnHighlighter::appendLimits()
{
    QTextCharFormat fmt;
    HighlightingRule ruleLimits;
    HighlightingRule ruleEsd;

    if (mode == LIGHT) fmt.setForeground(Qt::darkRed);
    if (mode == DARK)  fmt.setForeground(QColor(255, BCOL, BCOL));

    ruleLimits.expression = QRegularExpression(QString("((?:_%1\\^%1)|(?:_%1)|(?:\\^%1))").arg(numeric));
    ruleEsd.expression    = QRegularExpression(QString("(\\+-%1)").arg(numeric));

    ruleLimits.format = fmt;
    ruleEsd.format    = fmt;

    highlightingRules.append(ruleLimits);
    highlightingRules.append(ruleEsd);
}

void BgmnHighlighter::appendGoals()
{
    QTextCharFormat fmt;
    HighlightingRule rule;

    if (mode == LIGHT) fmt.setForeground(Qt::darkGreen);
    if (mode == DARK)  fmt.setForeground(QColor(BCOL, 255, BCOL));

    rule.expression = QRegularExpression("(GOAL(?:\\[\\d+\\])?)");
    rule.format = fmt;
    highlightingRules.append(rule);
}

void BgmnHighlighter::appendFileNames()
{
    QTextCharFormat fmt;
    HighlightingRule rule;

    if (mode == LIGHT) fmt.setForeground(Qt::darkCyan);
    if (mode == DARK)  fmt.setForeground(QColor(BCOL, 255, 255));

    rule.expression = QRegularExpression("=(\\S+\\.[a-zA-Z]+)");
    rule.format = fmt;
    highlightingRules.append(rule);
}

void BgmnHighlighter::appendParam()
{
    QTextCharFormat fmt;
    HighlightingRule rule;

    if (mode == LIGHT) fmt.setForeground(Qt::darkMagenta);
    if (mode == DARK)  fmt.setForeground(QColor(255, BCOL, 255));

    rule.expression = QRegularExpression("(PARAM(?:\\[\\d+\\])?=)");
    rule.format = fmt;
    highlightingRules.append(rule);
}

void BgmnHighlighter::appendErrors()
{
    QTextCharFormat fmt;
    fmt.setUnderlineColor(Qt::red);
    fmt.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

    QStringList keywords;
    keywords << "(\\s+=)"
             << "(=\\s+)"
             << "(\\s+=\\s+)"
             << "\\bE=([A-Z]?[a-z])"
             << "\\bE=[A-Z]+([0-9][\\+-]?)"
             << "^(\\s+)\\S";

    foreach (const QString &pattern, keywords) {
        HighlightingRule rule;
        rule.expression = QRegularExpression(pattern);
        rule.format = fmt;
        errorRules.append(rule);
    }
}

void BgmnHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty()) return;

    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegularExpressionMatchIterator iterator = rule.expression.globalMatch(text);

        while (iterator.hasNext()) {
            QRegularExpressionMatch rm = iterator.next();

            QString _string = rm.captured(1);
            int     _start  = rm.capturedStart(1);
            int     _length = rm.capturedLength(1);

            if (_string.left(1) == "=") {
                _start++;
                _length--;
            }

            if (_string.right(1) == "=") {
                _length--;
            }

            setFormat(_start, _length, rule.format);
        }
    }

    foreach (const HighlightingRule &rule, errorRules) {
        QRegularExpressionMatchIterator iterator = rule.expression.globalMatch(text);

        while (iterator.hasNext()) {
            QRegularExpressionMatch rm = iterator.next();
            setFormat(rm.capturedStart(1), rm.capturedLength(1), rule.format);
        }
    }

    QRegularExpression rx(QString("(%1)_(%1)\\^(%1)").arg(numeric));
    QRegularExpressionMatchIterator iterator = rx.globalMatch(text);

    while (iterator.hasNext()) {
        QRegularExpressionMatch rm = iterator.next();
        double val = rm.captured(1).toDouble();
        double low = rm.captured(2).toDouble();
        double upp = rm.captured(3).toDouble();

        if (val < low) setFormat(rm.capturedStart(1), rm.capturedLength(1), errorRules.first().format);
        if (val > upp) setFormat(rm.capturedStart(3), rm.capturedLength(3), errorRules.first().format);
        if (low > upp) setFormat(rm.capturedStart(2), rm.capturedLength(2)+rm.capturedLength(3)+1, errorRules.first().format);
    }
}
