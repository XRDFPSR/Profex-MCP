/***************************************************************************
                          chemtablewidget.h  -  description
                             -------------------
    begin                : Mon Jun 30 14:16:07 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#ifndef CHEMTABLEWIDGET_H
#define CHEMTABLEWIDGET_H

#include "../libXrdIO/structs.h"
#include "../libXrdIO/parser/bgmnlstparser.h"
#include "../libXrdIO/parser/bgmnsavparser.h"
#include "../libXrdIO/chemtabledata.h"
#include "../libXrdIO/chemtablestruct.h"
#include "projectWidget/graphWidget/abstractgraphview.h"
#include <QTableWidget>
#include <QColor>
#include <QComboBox>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>

using namespace global;

namespace Ui {
class ChemTableWidgetForm;
}

class QComboBox;

class ChemTableWidget : public AbstractGraphView
{
    Q_OBJECT
public:
    explicit ChemTableWidget(GraphDataController *, QWidget *parent = 0);
    ~ChemTableWidget();

    enum class Col : int { Goal = 0, Sum = 1, FirstQuantity = 2 };
    static inline int toInt(Col c) { return static_cast<int>(c); }

    inline void getPreset(QDomDocument &) override {};
    inline void applyPreset(const QDomElement &) override {};

    /* update the view when visibility changes */
    void setShown(bool) override;

    /* Reads data from an lst file and updates the table.
     * Normally the table is only updated when the widget is shown.
     * But when gathering chemtable data from all projects for export,
     * we need to force update even if the table is hidden. In that case,
     * set force = true.
     */
    void setData(const QString &savFile, const QString &lstFile, bool force = false);
    void setData(const BgmnSavParser &sparser, const BgmnLstParser &lparser, bool force = false);

    /* must be called after changing the settings, to apply changes
     * of the oxide / element format */
    void initSettings();

    /* clears the table */
    void clearAll();

    /* return true if the table contains valid data */
    bool hasData();

    /* return results in different formats */
    QStringList getCsvList(bool header = false, const QString &sampleId = QString()); // List with one parameter per line
    QString getCsvTable();                   // Table as shown on screen
    QString getHtmlTable();                  // Table as shown on screen

public slots:
    void updateView() override;

private:
    Ui::ChemTableWidgetForm *ui;
    QAction *actSetElement;
    QAction *actSetAtomic;
    QAction *actSetOxide;
    ChemistryMode mode;
    ChemTableData tableData;
    QString savFileName;
    QString lstFileName;
    QString sampleName;
    QHash<QString, QVariant> previousManualGoalAssignments;
    QMenu *contextMenu;
    QHash<QString, QString> phaseFromStrFile; // QHash<STR file, phase>
    QHash<QString, QString> goalFromStrFile;  // QHash<STR file, goal>
    QHash<QString, QString> goalFromPhase;    // QHash<phase, goal>

    bool reload();
    void updateTable();

private slots:
    void copy();
    void toggleMode(QAction *);
    void showContextMenu(const QPoint &);
};

#endif // CHEMTABLEWIDGET_H
