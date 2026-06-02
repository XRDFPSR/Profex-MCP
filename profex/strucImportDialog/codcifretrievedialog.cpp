/***************************************************************************
                          codcifretrievedialog.cpp  -  description
                             -------------------
    begin                : Thu Jan 11 19:42:15 CEST 2021
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

#include "codcifretrievedialog.h"
#include "ui_codcifretrievedialog.h"
#include "../libXrdIO/bgmnfileio.h"
#include <QRegularExpression>
#include <QString>
#include <QFileDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>

CodCifRetrieveDialog::CodCifRetrieveDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CodCifRetrieveDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();
    codManager = CodDbManager::getInstance();
    ui->pushButtonSearch->setDefault(true);
    ui->pushButtonSearch->setEnabled(false);

    QStringList headers = QStringList() << "COD ID" << "Mineral" << "Formula" << "Space Group" << "Year" << "Bibliography";
    ui->tableWidgetMatches->setColumnCount(headers.size());
    ui->tableWidgetMatches->setHorizontalHeaderLabels(headers);
    setDbStatusLabel(false);

    ui->comboBoxTemperature->addItem(tr("Room temperature"));
    ui->comboBoxTemperature->addItem(tr("High temperature"));
    ui->comboBoxTemperature->addItem(tr("Low temperature"));
    ui->comboBoxTemperature->setCurrentIndex(0);

    initSettings();
    checkCodConnection();
}

CodCifRetrieveDialog::~CodCifRetrieveDialog()
{
    delete ui;
}

void CodCifRetrieveDialog::accept()
{
    saveSettings();
    QDialog::accept();
}

void CodCifRetrieveDialog::initSettings()
{
    databaseMode = LOCAL_SQLITE;
    restoreGeometry(settings->value("codQueryDialog/geometry", QByteArray()).toByteArray());
    ui->tableWidgetMatches->horizontalHeader()->restoreState(settings->value("codQueryDialog/tableHeader", QByteArray()).toByteArray());
}

void CodCifRetrieveDialog::checkCodConnection()
{
    bool con = codManager->isConnected();
    ui->pushButtonSearch->setEnabled(con);
    setDbStatusLabel(con);
}

void CodCifRetrieveDialog::saveSettings()
{
    settings->setValue("codQueryDialog/tableHeader", ui->tableWidgetMatches->horizontalHeader()->saveState());
    settings->setValue("codQueryDialog/geometry", saveGeometry());
}

QStringList CodCifRetrieveDialog::getRecords()
{
    QStringList codids;

    for (int i = 0; i < ui->tableWidgetMatches->rowCount(); ++i) {
        if (ui->tableWidgetMatches->item(i, 0)->checkState() == Qt::Checked) {
            codids.append(ui->tableWidgetMatches->item(i, 0)->text());
        }
    }

    return codids;
}

QString CodCifRetrieveDialog::queryString()
{
    QStringList queryList;
    static QRegularExpression rxSplit("[\\s,;]+");
    static QRegularExpression rxCodId("(\\d{7})");

    if (!ui->lineEditCodId->text().isEmpty()) {

        QStringList codIdLst = ui->lineEditCodId->text().split(rxSplit);
        QStringList subQueries;

        for (int i = 0; i < codIdLst.size(); ++i) {
            QRegularExpressionMatch rmCodId = rxCodId.match(codIdLst.at(i));

            if (rmCodId.hasMatch()) {
                subQueries.append(QString("file LIKE '%1'").arg(rmCodId.captured(1)));
            }
        }

        if (subQueries.size() > 1) {
            queryList.append("(" + subQueries.join(" OR ") + ")");
        } else if (subQueries.size() == 1) {
            queryList.append(subQueries.first());
        }
    }

    if (!ui->lineEditMineralName->text().isEmpty()) queryList.append(QString("mineral LIKE '\%%1\%'").arg(ui->lineEditMineralName->text()));
    if (!ui->lineEditDOI->text().isEmpty()) queryList.append(QString("doi LIKE '\%%1\%'").arg(ui->lineEditDOI->text()));

    if (ui->spinBoxElementsMin->value() > 0) queryList.append(QString("nel >= %1").arg(ui->spinBoxElementsMin->value()));
    if (ui->spinBoxElementsMax->value() > 0) queryList.append(QString("nel <= %1").arg(ui->spinBoxElementsMax->value()));

    if (!ui->lineEditHM->text().isEmpty()) queryList.append(QString("sg LIKE '%1'").arg(ui->lineEditHM->text()));

    if (!ui->lineEditElementsInclude->text().isEmpty()) {
        QStringList elementsIncl = ui->lineEditElementsInclude->text().split(rxSplit);

        for (int i = 0; i < elementsIncl.size(); ++i) {
            if (databaseMode == ONLINE_MYSQL) queryList.append(QString("formula REGEXP ' %1[0-9 ]'").arg(elementsIncl.at(i)));
            if (databaseMode == LOCAL_SQLITE) queryList.append(QString("formula REGEXP '.+\\s%1[0-9\\s].+'").arg(elementsIncl.at(i)));
        }
    }

    if (!ui->lineEditElementsExclude->text().isEmpty()) {
        QStringList elementsExcl = ui->lineEditElementsExclude->text().split(rxSplit);

        for (int i = 0; i < elementsExcl.size(); ++i) {
            if (databaseMode == ONLINE_MYSQL) queryList.append(QString("NOT formula REGEXP ' %1[0-9 ]'").arg(elementsExcl.at(i)));
            if (databaseMode == LOCAL_SQLITE) queryList.append(QString("NOT formula REGEXP '.+\\s%1[0-9\\s].+'").arg(elementsExcl.at(i)));
        }
    }

    // conversion of the database from mysql to sqlite replaces NULL with \N. We need to adapt the queries accordingly

    if (ui->checkBoxRestrictTemp->isChecked()) {
        if (databaseMode == ONLINE_MYSQL) {
            if (ui->comboBoxTemperature->currentIndex() == 0) queryList.append("(diffrtemp between 273 and 300 or diffrtemp is null)");
            else if (ui->comboBoxTemperature->currentIndex() == 1) queryList.append("diffrtemp > 300");
            else if (ui->comboBoxTemperature->currentIndex() == 2) queryList.append("diffrtemp < 273");
        } else if (databaseMode == LOCAL_SQLITE) {
            if (ui->comboBoxTemperature->currentIndex() == 0) queryList.append("(diffrtemp between 273 and 300 or diffrtemp is '\\N')");
            else if (ui->comboBoxTemperature->currentIndex() == 1) queryList.append("(diffrtemp > 300 AND diffrtemp IS NOT '\\N')");
            else if (ui->comboBoxTemperature->currentIndex() == 2) queryList.append("(diffrtemp < 273 AND diffrtemp IS NOT '\\N')");
        }
    }

    if (!ui->lineEditText1->text().isEmpty()) queryList.append(QString("title LIKE '\%%1\%'").arg(ui->lineEditText1->text()));
    if (!ui->lineEditText2->text().isEmpty()) queryList.append(QString("title LIKE '\%%1\%'").arg(ui->lineEditText2->text()));

    if (!ui->lineEditAuthors1->text().isEmpty()) queryList.append(QString("authors LIKE '\%%1\%'").arg(ui->lineEditAuthors1->text()));
    if (!ui->lineEditAuthors2->text().isEmpty()) queryList.append(QString("authors LIKE '\%%1\%'").arg(ui->lineEditAuthors2->text()));

    if (!ui->lineEditJournalName->text().isEmpty()) queryList.append(QString("journal LIKE '\%%1\%'").arg(ui->lineEditJournalName->text()));
    if (!ui->lineEditJournalVolume->text().isEmpty()) queryList.append(QString("volume LIKE '\%%1\%'").arg(ui->lineEditJournalVolume->text()));

    if (ui->spinBoxJournalYearMin->value() > 0) queryList.append(QString("year >= %1").arg(ui->spinBoxJournalYearMin->value()));
    if (ui->spinBoxJournalYearMax->value() > 0) queryList.append(QString("year <= %1").arg(ui->spinBoxJournalYearMax->value()));

    return queryList.join(" AND ");
}

void CodCifRetrieveDialog::queryDatabase()
{
    qApp->setOverrideCursor(Qt::WaitCursor);

    clearTable(false);

    QString qstr = queryString();

    if (qstr.simplified().isEmpty()) {
        qApp->restoreOverrideCursor();
        return;
    }

    QString q = QString("SELECT file, mineral, formula, sg, year, text FROM data WHERE %1").arg(qstr);
    qDebug() << QString("CodCifRetrieveDialog::queryDatabase(): Executing query %1").arg(q);

    qApp->processEvents();

    bool qOk;
    QSqlQuery query = codManager->query(q, qOk);

    if (!qOk) {
        qApp->restoreOverrideCursor();
        qDebug() << QString("CodCifRetrieveDialog::queryDatabase(): Query returned an error: %1").arg(query.lastError().text());
        return;
    }

    int n = 0;

    while (query.next()) {
        int r = ui->tableWidgetMatches->rowCount();
        ui->tableWidgetMatches->setRowCount(r+1);

        static QRegularExpression rxFormula("-\\s+([^-]+)\\s+-");
        QRegularExpressionMatch rm = rxFormula.match(query.value(2).toString());
        QString formula = rm.hasMatch() ? rm.captured(1) : query.value(2).toString();
        QString mineral = query.value(1).toString();
        if (mineral == "\\N") mineral = "n.a.";

        ui->tableWidgetMatches->setItem(r, 0, new QTableWidgetItem(query.value(0).toString()));
        ui->tableWidgetMatches->setItem(r, 1, new QTableWidgetItem(mineral));
        ui->tableWidgetMatches->setItem(r, 2, new QTableWidgetItem(formula));
        ui->tableWidgetMatches->setItem(r, 3, new QTableWidgetItem(query.value(3).toString()));
        ui->tableWidgetMatches->setItem(r, 4, new QTableWidgetItem(query.value(4).toString()));
        ui->tableWidgetMatches->setItem(r, 5, new QTableWidgetItem(query.value(5).toString()));

        ui->tableWidgetMatches->item(r, 0)->setCheckState(Qt::Unchecked);
        ++n;
    }

    qDebug() << QString("CodCifRetrieveDialog::queryDatabase(): Query returned %1 matches").arg(n);

    qApp->restoreOverrideCursor();
}

void CodCifRetrieveDialog::checkAll()
{
    for (int i = 0; i < ui->tableWidgetMatches->rowCount(); ++i) {
         ui->tableWidgetMatches->item(i, 0)->setCheckState(Qt::Checked);
    }
}

void CodCifRetrieveDialog::checkNone()
{
    for (int i = 0; i < ui->tableWidgetMatches->rowCount(); ++i) {
         ui->tableWidgetMatches->item(i, 0)->setCheckState(Qt::Unchecked);
    }
}

void CodCifRetrieveDialog::saveQueries()
{
    QString dir = settings->value("codQueryDialog/saveListDir", QDir::homePath()).toString();
    QString file = QFileDialog::getSaveFileName(this, tr("Save match list"), dir, tr("CSV table (*.csv *.CSV)"));
    if (file.isEmpty()) return;

    QFileInfo fi(file);
    settings->setValue("codQueryDialog/saveListDir", fi.absolutePath());

    QStringList out;

    for (int i = 0; i < ui->tableWidgetMatches->rowCount(); ++i) {
        QStringList l;

        for (int j = 0; j < ui->tableWidgetMatches->columnCount(); ++j) {
            l.append(ui->tableWidgetMatches->item(i, j)->text());
        }

        out.append(l.join(";"));
    }

    BgmnFileIO::writeTextFile(fi.absoluteFilePath(), out.join("\n"));
    qDebug() << QString("CodCifRetrieveDialog::saveQueries(): List saved to %1").arg(fi.absoluteFilePath());
}

void CodCifRetrieveDialog::clearForm()
{
    ui->lineEditCodId->clear();
    ui->lineEditMineralName->clear();
    ui->lineEditElementsExclude->clear();
    ui->lineEditElementsInclude->clear();
    ui->spinBoxElementsMax->setValue(0);
    ui->spinBoxElementsMin->setValue(0);
    ui->lineEditHM->clear();
    ui->comboBoxTemperature->setCurrentIndex(0);
    ui->checkBoxRestrictTemp->setChecked(false);

    ui->lineEditText1->clear();
    ui->lineEditText2->clear();
    ui->lineEditAuthors1->clear();
    ui->lineEditAuthors2->clear();
    ui->lineEditJournalName->clear();
    ui->lineEditJournalVolume->clear();
    ui->spinBoxJournalYearMax->setValue(0);
    ui->spinBoxJournalYearMin->setValue(0);
    ui->lineEditDOI->clear();
}

void CodCifRetrieveDialog::clearTable(bool all)
{
    if (all) {
        ui->tableWidgetMatches->clearContents();
        ui->tableWidgetMatches->setRowCount(0);
        return;
    }

    // if all is false, keep the rows that are checked

    for (int i = ui->tableWidgetMatches->rowCount(); i > 0; --i) {
        int n = i - 1;

        if (ui->tableWidgetMatches->item(n, 0)->checkState() == Qt::Unchecked) {
            ui->tableWidgetMatches->removeRow(n);
        }
    }
}

void CodCifRetrieveDialog::setDbStatusLabel(bool b)
{
    if (b) {
        ui->labelDbStatus->setText("<font color=\"Green\">Database connected</font>");
    } else {
        ui->labelDbStatus->setText("<font color=\"Red\">No database connected</font>");
    }
}
