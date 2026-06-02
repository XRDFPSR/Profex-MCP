/***************************************************************************
                          formatresults.h  -  description
                             -------------------
    begin                : Sat Jun 24 11:00:00 CEST 2017
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

#ifndef FORMATRESULTS_H
#define FORMATRESULTS_H

#include <QDialog>
#include <QSettings>
#include <QStringList>
#include <QList>
#include <QCloseEvent>
#include <QSize>
#include <QFileInfo>
#include <limits.h>
#include "../libXrdIO/settingsmanager.h"

namespace Ui {
class FormatResults;
}

#define meanIndex std::numeric_limits<int>::max() - 1
#define sdIndex std::numeric_limits<int>::max()

struct ResultsDataSet {
    QString file;
    int composition;
    int simulation;
    QStringList parameters;
    QMap<QString, double> values;
    QMap<QString, double> esds;
    bool isEmpty;
};

class FormatResults : public QDialog
{
    Q_OBJECT

public:
    explicit FormatResults(QWidget *parent = 0);
    ~FormatResults();

    void saveSettings();

private:
    Ui::FormatResults *ui;
    SettingsManager *settings;
    QFileInfo loadedFile;
    QStringList skipParams;

    void closeEvent(QCloseEvent *);
    void parseCsv(const QString &);
    QMap<QString, ResultsDataSet> splitCsv(const QString &);
    QMap<QString, QMap<int, QStringList> > transposeData(const QMap<QString, ResultsDataSet> &);
    QMap<QString, QMap<int, QStringList> > addMeanSd(const QMap<QString, QMap<int, QStringList> > &);
    QList<int> getTableSize(const QMap<QString, ResultsDataSet> &);
    ResultsDataSet lineToResultsDataSet(const QStringList &);
    void resizeTable(const QMap<QString, ResultsDataSet> &);
    void createTableHeader();
    void createTableData(const QMap<QString, QMap<int, QStringList> > &);
    double mean(const QVector<double> &);
    double standardDeviation(const QVector<double> &, double );
    QStringList getAllMeans(const QMap<int, QStringList> &);
    QStringList getAllSds(const QMap<int, QStringList> &);

private slots:
    void load();
    void saveAs();
    void reload();
};

#endif // FORMATRESULTS_H
