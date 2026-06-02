/***************************************************************************
                          projectsearchmatchwidget.h  -  description
                             -------------------
    begin                : Tue Jan 22 21:00:00 CEST 2019
    copyright            : (C) 2019 by Nicola Doebelin
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

#ifndef PEAKLISTWIDGET_H
#define PEAKLISTWIDGET_H

#include <QWidget>
#include <QTreeWidgetItem>
#include <QStandardItemModel>
#include "../graphWidget/abstractgraphview.h"
#include "../bgmnrefstructuremanager.h"
#include "../../../libXrdIO/structs.h"
#include "hklsortfilterproxymodel.h"

namespace Ui {
class PeakListWidget;
}

class PeakListWidget : public AbstractGraphView
{
    Q_OBJECT

public:
    explicit PeakListWidget(GraphDataController *c, QWidget *parent = nullptr);
    ~PeakListWidget();

    void getPreset(QDomDocument &) override;
    void applyPreset(const QDomElement &) override;

    void clearAll();

    QString getCsvDataAll(bool header) const;
    QString getCsvDataFiltered(bool header) const;
    QString getQualxDdata() const;
    QString getQualxTdata() const;

    QDomElement getFilterParameters() const;
    void setFilterParameters(const QDomElement &);
    void hasPeakSelection(int &, int &) const;

    QList<Hkl> getDataAll(const global::PositionUnit &) const;
    QList<Hkl> getDataFiltered(const global::PositionUnit &) const;
    QList<Hkl> getDataSelected(const global::PositionUnit &) const;

public slots:
    void updateView() override;
    void updateData(bool force = false);

private:
    Ui::PeakListWidget *ui;
    BgmnRefStructureManager *refStrManager;
    QStringList hklHeaderLabels;
    QStandardItemModel *peakDataModel;
    HklSortFilterProxyModel *peakDataProxy;
    QHash<QString, int> columns;
    QMenu *contextMenuHeader;
    QMenu *contextMenuTable;
    QMenu *toolsMenu;
    bool saveSettingsRequested;
    int prevNumberOfScans;
    QList<int> previousSplitterSizes;

    void initSettings();

    void appendDataToModel(const Scan *, double, double wl);

    void saveToCsv(const QString &);
    void saveToQualxD(const QString &);
    void saveToQualxT(const QString &);
    QList<int> hklToInt(const QString &);
    int indexOfHkl(const Scan *, const QUuid &);
    QList<Hkl> getHklFromModel(const QAbstractItemModel *, const QModelIndexList &, const global::PositionUnit &) const;
    void keyPressEvent(QKeyEvent *) override;

private slots:
    void resultSelected(QTreeWidgetItem *);
    void saveHklList();
    void clearHklSelection();
    void selectHkl(const QItemSelection &, const QItemSelection &);
    void removeCurrentHkl();
    void reloadData();
    void openCustomHeaderContextMenu(const QPoint &);
    void openCustomTableContextMenu(const QPoint &);
    void toggleColumnHidden(QAction *);
    void headerResized();
    void splitterResized(int, int);
    void toggleFilterWidget();
    void copyText();
    void showHelp();

signals:
    void resultsSelectionChanged(const QString &);
    void helpText(QString);
    void applyFiltersToAll();
};

#endif // PEAKLISTWIDGET_H
