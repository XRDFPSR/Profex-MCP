/***************************************************************************
                          prefpagesummarytables.h  -  description
                             -------------------
    begin                : Wed May 10 10:00:00 CEST 2017
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

#ifndef PREFPAGESUMMARYTABLES_H
#define PREFPAGESUMMARYTABLES_H

#include "prefpagetemplate.h"

namespace Ui {
class PrefPageSummaryTables;
}

class PrefPageSummaryTables : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageSummaryTables(QWidget *parent = 0);
    ~PrefPageSummaryTables();

    void initUi();
    void initSettings();
    void saveSettings();
    QString name() {return "SummaryTables";}

private:
    Ui::PrefPageSummaryTables *ui;
    QColor lodWarningCol;
    QColor loqWarningCol;

private slots:
    void selectLODColor();
    void selectLOQColor();
    void defaultBgmnLocalGoals();
    void defaultBgmnGlobalGoals();
    void toggleWarningsEnabled(bool);
};

#endif // PREFPAGESUMMARYTABLES_H
