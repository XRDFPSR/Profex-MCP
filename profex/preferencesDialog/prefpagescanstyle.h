/***************************************************************************
                          prefpagescanstyle.h  -  description
                             -------------------
    begin                : Tue May 09 16:00:00 CEST 2017
    copyright            : (C) 2017 by Nicola Doebelin
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

#ifndef PREFPAGESCANSTYLE_H
#define PREFPAGESCANSTYLE_H

#include "prefpagetemplate.h"

#include <QTreeWidgetItem>

namespace Ui {
class PrefPageScanStyle;
}

class PrefPageScanStyle : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageScanStyle(QWidget *parent = 0);
    ~PrefPageScanStyle();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "ScanStyle";}

private:
    Ui::PrefPageScanStyle *ui;
    QStringList pointStyles;

    QColor colIobs;
    QColor colIcalc;
    QColor colIdiff;
    QColor colIbkgr;

    void setColorTable(const QList<QColor> &);
    void setStyleTable(const QList<int> &);
    QList<QColor> getColorTable();
    QList<int> getStyleTable();

private slots:
    void addColor();
    void removeColor();
    void colorChanged(QTreeWidgetItem *, int);
    void itemSelectionChanged();
    void iObsColorChanged();
    void iCalcColorChanged();
    void iDiffColorChanged();
    void iBkgrColorChanged();
    void resetList();
};

#endif // PREFPAGESCANSTYLE_H
