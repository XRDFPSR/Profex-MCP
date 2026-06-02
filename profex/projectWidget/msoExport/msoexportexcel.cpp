/***************************************************************************
                          msoexportexcel.cpp  -  description
                             -------------------
    begin                : Sun Aug 09 08:25:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#include "msoexportexcel.h"
#include "../libXrdIO/parser/bgmnlstparser.h"
#include "../libXrdIO/parser/bgmnsavparser.h"
#include "../libXrdIO/scan.h"
#include <QRegularExpression>
#include <QProgressDialog>
#include <QApplication>
#include <QDebug>

const QChar MsoExportExcel::letters[] = {
    'A', 'B', 'C', 'D',
    'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L',
    'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X',
    'Y', 'Z'
};

MsoExportExcel::MsoExportExcel(GraphDataController *c, QObject *parent)
    : QObject(parent), graphControl(c)
{
#ifdef Q_OS_WIN
    m_excelApplication = nullptr;
    m_sheet = nullptr;
    m_sheets = nullptr;
    m_workbook = nullptr;
    m_workbooks = nullptr;
    m_excelApplication = nullptr;
#endif
}

bool MsoExportExcel::hasData() const
{
    return _config.size() > 0;
}

void MsoExportExcel::getPreset(QDomDocument &doc)
{
    if (!_config.size()) return;

    QDomElement elAxExWorkbook = doc.createElement("excelWorkbook");
    doc.documentElement().appendChild(elAxExWorkbook);

    if (_config.size()) {
        elAxExWorkbook.setAttribute("File", _config.first().file);

        for (int i = 0; i < _config.size(); ++i) {
            QDomElement elAxExCell = doc.createElement("excelCell");
            elAxExCell.setAttribute("Worksheet", _config.at(i).worksheet);
            elAxExCell.setAttribute("Row",       _config.at(i).row);
            elAxExCell.setAttribute("Column",    _config.at(i).col);
            elAxExCell.setAttribute("Phase",     _config.at(i).phase);
            elAxExCell.setAttribute("Parameter", _config.at(i).parameter);
            elAxExCell.setAttribute("Filter",    _config.at(i).filter);
            elAxExCell.setAttribute("Output",    _config.at(i).output);
            elAxExWorkbook.appendChild(elAxExCell);
        }
    }
}

void MsoExportExcel::applyPreset(const QDomElement &element)
{
    _config.clear();

    QString file = element.attribute("File");
    QDomNodeList l = element.elementsByTagName("excelCell");

    for (int i = 0; i < l.size(); ++i) {
         QDomElement el = l.at(i).toElement();
         if (el.isNull()) continue;

         global::AxObjectExcel ax;
         ax.file      = file;
         ax.worksheet = el.attribute("Worksheet", "0").toInt();
         ax.row       = el.attribute("Row", "0").toInt();
         ax.col       = el.attribute("Column", "0").toInt();
         ax.phase     = el.attribute("Phase");
         ax.parameter = el.attribute("Parameter");
         ax.filter    = el.attribute("Filter");
         ax.output    = el.attribute("Output");

         _config.append(ax);
    }
}

QList<global::AxObjectExcel> MsoExportExcel::getConfig()
{
    return _config;
}

void MsoExportExcel::initStructure(const QList<global::AxObjectExcel> &l)
{
    _config = l;
}

// https://forum.qt.io/topic/16547/how-to-export-excel-in-qt/10
void MsoExportExcel::exportData(const QString &savFile, const QString &lstFile)
{
#ifdef Q_OS_WIN
    if (_config.isEmpty())       return;
    if (!QFile::exists(lstFile)) return;

    QMap<QString, QList<global::Result> > lstPars = parseLstFile(lstFile);
    QMap<QString, QVariant> savPars = parseSavFile(savFile);

    m_excelApplication = new QAxObject("Excel.Application", 0);

    if (m_excelApplication == nullptr) {
        qDebug() << QString("MsoExportExcel::exportData(): Failed to initialize Excel object. Is Excel installed?");
        return;
    }

    QString fileName = _config.first().file;

    if (fileName == "%%project%%") {
        QFileInfo fi(savFile);
        fileName = fi.absolutePath() + "\\" + fi.completeBaseName() + ".xlsx";
    }

    if (fileName != "%%new%%") {
        if (!QFile::exists(fileName)) {
            qDebug() << QString("MsoExportExcel::exportData(): File %1 doesn't exists. Skipping.").arg(fileName);
            return;
        }
    }

    static QRegularExpression rx("([^\\/])[\\/]([^\\/])");
    fileName.replace(rx, "\\1\\\\2");
    qDebug() << QString("MsoExportExcel::exportData(): Exporting to file %1").arg(fileName);

    m_excelApplication->dynamicCall("SetVisible(bool)", true);
    m_excelApplication->setProperty("DisplayAlerts", 1);
    m_workbooks = m_excelApplication->querySubObject("Workbooks");

    if (fileName == "%%new%%") {
        m_workbook = m_workbooks->querySubObject("Add");
    } else {
        m_workbook = m_workbooks->querySubObject("Open(const QString&)", fileName);
    }

    for (int i = 0; i < _config.size(); ++i) {
        m_sheets = m_workbook->querySubObject("Worksheets");
        m_sheet = m_sheets->querySubObject("Item( int )", _config.at(i).worksheet);

        if (_config.at(i).parameter.left(5) == "Scan_") {
            writeScanToWorksheet(_config.at(i));
        } else if (lstContains(lstPars, _config.at(i).phase, _config.at(i).parameter)) {
            QAxObject *cell = m_sheet->querySubObject("Cells(int,int)", _config.at(i).row, _config.at(i).col);
            double val = getLstValue(lstPars, _config.at(i).phase, _config.at(i).parameter);
            cell->setProperty("Value", val);
            delete cell;
        } else if (savPars.contains(_config.at(i).parameter)) {
            QAxObject *cell = m_sheet->querySubObject("Cells(int,int)", _config.at(i).row, _config.at(i).col);
            QString val = savPars.value(_config.at(i).parameter).toString();
            cell->setProperty("Value", getFilteredVal(val, _config.at(i).filter, _config.at(i).output));
            delete cell;
        }
    }

#else
    Q_UNUSED(lstFile);
    Q_UNUSED(savFile);
#endif
}

QMap<QString, QList<global::Result> > MsoExportExcel::parseLstFile(const QString &l)
{
    QMap<QString, QList<global::Result> > results;

    bool ok;
    BgmnLstParser lparser(l, ok);
    if (!ok) return results;

    QStringList phases = lparser.getPhaseNames();
    results.insert("GLOBAL", lparser.getGlobalGoals());
    results["GLOBAL"].append(lparser.getStats());

    for (int i = 0; i < phases.size(); ++i) {
        results.insert(phases.at(i), lparser.getLocalGoals(phases.at(i)));
    }

    return results;
}

QMap<QString, QVariant> MsoExportExcel::parseSavFile(const QString &f)
{
    BgmnSavParser sparser(f);
    QMap<QString, QVariant> savParam;

    savParam.insert("SampleID", QVariant(sparser.sampleId()));
    savParam.insert("WMIN", QVariant(sparser.getWmin()));
    savParam.insert("WMAX", QVariant(sparser.getWmax()));
    savParam.insert("LAMBDA", QVariant(sparser.getLambda()));
    savParam.insert("VERZERR", QVariant(sparser.deviceFile()));

    for (int i = 0; i < sparser.valFile().size(); ++i) {
        savParam.insert(QString("VAL[%1]").arg(i + 1), QVariant(sparser.valFile().at(i)));
    }

    for (int i = 0; i < graphControl->count(); ++i) {
        savParam.insert(QString("Scan_%1").arg(i + 1), QVariant());
    }

    return savParam;
}

bool MsoExportExcel::lstContains(const QMap<QString, QList<global::Result> > &res, const QString &ph, const QString &par)
{
    if (!res.contains(ph)) {
        qDebug() << QString("MsoExportExcel::lstContains(): Phase not found in results: %1").arg(ph);
        return false;
    }

    QString param = par.right(3) == "ESD" ? par.left(par.length() - 3) : par;

    for (int i = 0; i < res.value(ph).size(); ++i) {
        if (res.value(ph).at(i).name == param) return true;
    }

    return false;
}

double MsoExportExcel::getLstValue(const QMap<QString, QList<global::Result> > &res, const QString &ph, const QString &par)
{
    if (!res.contains(ph)) return -1.0;

    bool isEsd = par.right(3) == "ESD";
    QString param = isEsd ? par.left(par.length() - 3) : par;

    for (int i = 0; i < res.value(ph).size(); ++i) {
        if (res.value(ph).at(i).name == param) {
            return isEsd ? res.value(ph).at(i).esd : res.value(ph).at(i).value;
        }
    }

    return -1.0;
}

QString MsoExportExcel::getFilteredVal(const QString &val, const QString &fil, const QString &cap)
{
    if (fil.isEmpty()) return val;

    // replace single escape \ with double
    QString _filter(fil);
    QRegularExpression rxEsc("([^\\])[\\]([^\\])");
    _filter.replace(rxEsc, "\\1\\\\2");

    QRegularExpression rx(_filter);
    QRegularExpressionMatch rm = rx.match(val);
    QStringList captures = rm.capturedTexts();

    QString out = cap;

    // we loop in reverse order to make sure that \1 does not accidentally replace \10 etc.
    for (int i = captures.size() - 1; i >= 0; --i) {
        out.replace(QString("\\%1").arg(i), captures.at(i));
    }

    return out;
}

#ifdef Q_OS_WIN
void MsoExportExcel::writeScanToWorksheet(const global::AxObjectExcel &obj)
{
    int scanNo = 0;
    QRegularExpression rx("Scan_(\\d+)");
    QRegularExpressionMatch rm = rx.match(obj.parameter);
    if (rm.hasMatch()) scanNo = rm.captured(1).toInt() - 1;

    const Scan *scan = graphControl->getScan(scanNo);
    if (!scan) return;

    QList<QVariant> table;

    for (int i = 0; i < scan->size(); ++i) {
        QList<QVariant> row;
        row.append(QVariant(scan->angle(i)));
        row.append(QVariant(scan->intensity(i)));
        table.append(QVariant(row));
    }

    QString rStart(cellIdxToString(obj.row, obj.col));
    QString rEnd(cellIdxToString(obj.row + scan->size(), obj.col + 1));

    QAxObject *range = m_sheet->querySubObject("Range(const QString&, const QString&)", rStart, rEnd);
    QVariant vTable(table);
    range->dynamicCall("SetValue(const QVariant&)", vTable);
}
#endif

void MsoExportExcel::showEditor(const QString &savFile, const QString &lstFile)
{
    MsoEditExcelExportDialog *msoEdlg = new MsoEditExcelExportDialog();
    msoEdlg->initData(parseSavFile(savFile), parseLstFile(lstFile), _config);

    if (msoEdlg->exec() == QDialog::Accepted) {
        _config = msoEdlg->getConfig();
    }

    delete msoEdlg;
}

QString MsoExportExcel::cellIdxToString(int r, int c)
{
    int d = c;
    QString s = "";
    int m;

    while (d > 0) {
        m = (d - 1) % 26;
        s = QString(letters[m]) + s;
        d = int((d - m) / 26);
    }

    return QString("%1%2").arg(s).arg(r);
}
