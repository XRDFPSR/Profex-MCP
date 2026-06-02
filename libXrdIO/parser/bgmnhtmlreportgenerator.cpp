/***************************************************************************
                          bgmnreporthtmlgenerator.cpp  -  description
                             -------------------
    begin                : Jun 28 18:55:07 CET 2019
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

#include "bgmnhtmlreportgenerator.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QAbstractTextDocumentLayout>
#include <QFileInfo>
#include <QChar>
#include <QDomDocument>
#include <QStringList>
#include <QVariant>
#include <QDir>
#include <QStringList>
#include <QDebug>
#include <QVector>
#include <math.h>
#include "bgmnparparser.h"
#include "bgmnfileio.h"
#include "hkl.h"
#include "functions.h"

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

BgmnHtmlReportGenerator::BgmnHtmlReportGenerator()
{
    settings = SettingsManager::getInstance();
}

BgmnHtmlReportGenerator::BgmnHtmlReportGenerator(const QString &f)
{
    settings = SettingsManager::getInstance();
    setLstFile(f);
}

BgmnHtmlReportGenerator::BgmnHtmlReportGenerator(const BgmnLstParser &l)
{
    settings = SettingsManager::getInstance();
    lparser = l;
}

void BgmnHtmlReportGenerator::setDocumentStructure(const QString &xml)
{
    docStructure.setContent(xml);

    QDomNodeList nl = docStructure.documentElement().elementsByTagName("skipErrors");

    if (!nl.size()) {
        skipErrors = settings->value("bgmnProject/report/skipErros", false).toBool();
    } else {
        skipErrors = nl.at(0).toElement().attribute("status", "false") == "true";
    }
}

void BgmnHtmlReportGenerator::setLstFile(const QString &f)
{
    lparser.load(f);
}

QString BgmnHtmlReportGenerator::getContents()
{
    return contents;
}

void BgmnHtmlReportGenerator::setProjectData(const QMap<QString, QVariant> &m)
{
    projectData = m;
}

void BgmnHtmlReportGenerator::generateDocument()
{
    contents = htmlHeader();
    contents += headerLogo();

    QDomElement docEl = docStructure.documentElement();
    QDomElement secEl = docEl.firstChildElement();

    while (!secEl.isNull()) {
        if (secEl.tagName() == "pageBreak") {
            contents += "<div class=\"page-break\"></div>\n";
        }

        if (secEl.tagName() != "pageBreak") {
            QString ident = secEl.attribute("ident");
            if (ident == "strucSampleInfo")  contents += sampleInfoTable(secEl);
            if (ident == "strucGlobalGoals") contents += globalGoalsTable(secEl);
            if (ident == "strucDiffPattern") contents += diffPatternSection(secEl);
            if (ident == "strucLocalGoals")  contents += localGoalsTable(secEl);
            if (ident == "strucChemComp")    contents += getChemistryTable(secEl);
            if (ident == "strucPeakList")    contents += getHklListTable(secEl);
            if (ident == "strucCustom") {
                if (secEl.attribute("status") == "true") contents += secEl.text();
            }
        }

        secEl = secEl.nextSiblingElement();
    }

    contents += "  </body>\n</html>";
}

double BgmnHtmlReportGenerator::patternAspectRatio()
{
    QDomElement docEl = docStructure.documentElement();
    QDomNodeList subsections = docEl.elementsByTagName("subsection");

    for (int i = 0; i < subsections.size(); ++i) {
        if (subsections.at(i).toElement().attribute("ident") == "strucDiffPatternAspect") {
            bool ok;
            double d = subsections.at(i).toElement().attribute("display").toDouble(&ok);
            if (ok) return d;
        }
    }

    return 1.414;
}

ChemistryMode BgmnHtmlReportGenerator::chemistryTableMode()
{
    QMap<QString, ChemistryMode> map;
    map["Elements by weight-%"] = ChemistryMode::ELEMENT;
    map["Elements by atom-%"] = ChemistryMode::ATOMIC;
    map["Oxides by weight-%"] = ChemistryMode::OXIDE;

    QDomElement docEl = docStructure.documentElement();
    QDomNodeList subsections = docEl.elementsByTagName("subsection");

    for (int i = 0; i < subsections.size(); ++i) {
        if (subsections.at(i).toElement().attribute("ident") == "strucChemTableMode") {
            QString s = subsections.at(i).toElement().attribute("display");
            return map.value(s, ChemistryMode::ELEMENT);
        }
    }

    return ChemistryMode::ELEMENT;
}

QString BgmnHtmlReportGenerator::htmlHeader()
{
    QString css(composeStyleSheed());

    QString s("<!DOCTYPE html>\n<html>\n");
    s += "<head>\n";
    s += "  <title>Profex Refinement Report</title>\n";
    s += "    <style>\n" + css + "\n</style>\n";
    s += "</head>\n";
    s += "<body>";
    return s;
}

QString BgmnHtmlReportGenerator::headerLogo()
{
    QDomNodeList nl = docStructure.documentElement().elementsByTagName("banner");
    if (!nl.size()) return QString();

    QString bannerFile = nl.at(0).toElement().attribute("fileName", QString());
    QFile f(bannerFile.isEmpty() ? ":/resources/profex-header.svg" : bannerFile);
    if (!f.open(QIODevice::ReadOnly)) return QString();

    QDomDocument doc;
    doc.setContent(f.readAll());
    QDomElement docElem = doc.documentElement();

    docElem.removeAttribute("width");
    docElem.removeAttribute("height");

    QString s;
    QTextStream str(&s);
    docElem.save(str, 4);
    str.flush();

    return s += "<br>\n";
}

QString BgmnHtmlReportGenerator::sampleInfoTable(const QDomElement &element)
{
    if (element.attribute("status") == "false") return QString();

    bool addStats = true;
    bool addOperator = true;
    QString uname;

    QDomNodeList subsections = element.elementsByTagName("subsection");

    for (int i = 0; i < subsections.size(); ++i) {
        QDomElement subEl = subsections.at(i).toElement();

        if (subEl.attribute("ident") == "strucOpName") {
            addOperator = (subEl.attribute("status") == "true");
            uname = subEl.attribute("display");
        }

        if (subEl.attribute("ident") == "strucStatistics") {
            addStats = (subEl.attribute("status") == "true");
        }
    }

    if (uname.isEmpty()) uname = qgetenv("USER");
    if (uname.isEmpty()) uname = qgetenv("USERNAME");

    QDateTime dt(lparser.refinementDateTime());
    QString date(QLocale::system().toString(dt.date(), "dddd MMMM dd, yyyy"));
    QFileInfo fi(projectData["rawFileName"].toString());

    double r_wp = lparser.getRwp();
    double r_exp = lparser.getRexp();
    double chi_2 = lparser.getChi2();

    QString table;
    table += QString("<h1>%1</h1>\n\n").arg(element.attribute("display"));
    table += QString("<table id=\"tableInfo\">\n");
    table += QString("  <th colspan=\"5\">%1</th>\n").arg(projectData["sampleId"].toString());
    table += QString("  <tr>\n");
    table += QString("    <td width=\"24%\">File Name</td>\n");
    table += QString("    <td colspan=\"4\">%1</td>\n").arg(fi.fileName());
    table += QString("  </tr>\n");
    table += QString("  <tr>\n");
    table += QString("      <td>Instrument configuration</td>\n");
    table += QString("      <td colspan=\"4\">%1</td>\n").arg(projectData.value("geqFile").toString());
    table += QString("  </tr>\n");
    table += QString("  <tr>\n");
    table += QString("      <td>Wavelength</td>\n");
    table += QString("      <td colspan=\"4\">%1 (%2 &#8491;)</td>\n").arg(projectData["wavelengthFile"].toString()).arg(projectData.value("wavelength", 1.54056).toDouble());
    table += QString("  </tr>\n");
    table += QString("  <tr>\n");
    table += QString("    <td>Directory</td>\n");
    table += QString("    <td colspan=\"4\">%1</td>\n").arg(projectData["workingDir"].toString());
    table += QString("  </tr>\n");
    table += QString("  <tr>\n");
    table += QString("    <td>Date of Refinement</td>\n");
    table += QString("    <td colspan=\"4\">%1</td>\n").arg(date);
    table += QString("  </tr>\n");

    if (addOperator) {
    table += QString("  <tr>\n");
    table += QString("    <td>Operator</td>\n");
    table += QString("    <td colspan=\"4\">%1</td>\n").arg(uname);
    table += QString("  </tr>\n");
    }

    if (addStats) {
        table += QString("  <tr>\n");
        table += QString("    <td>Statistics</td>\n");
        table += QString("    <td width=\"19%\">R<sub>wp</sub> = %1</td>\n").arg(r_wp, 0, 'f', 2);
        table += QString("    <td width=\"19%\">R<sub>exp</sub> = %1</td>\n").arg(r_exp, 0, 'f', 2);
        table += QString("    <td width=\"19%\">&chi;<sup>2</sup> = %1</td>\n").arg(chi_2, 0, 'f', 4);
        table += QString("    <td width=\"19%\">GoF = %1</td>\n").arg(sqrt(float(chi_2)), 0, 'f', 4);
        table += QString("  </tr>\n");
    }

    table += QString("</table>\n<br>\n");
    return table;
}

QString BgmnHtmlReportGenerator::globalGoalsTable(const QDomElement &element)
{
    if (element.attribute("status") == "false") return QString();
    QList<global::Result> ttable = lparser.getGlobalGoals(projectData["globalGoalsIncludes"].toStringList());

    int rows = ttable.size();
    QString table;
    table += QString("<h1>%1</h1>\n").arg(element.attribute("display"));
    table += QString("<table id=\"tableGlobalGoals\">\n");
    table += QString("  <tr>\n");
    table += QString("    <th>Parameter</th>\n");
    table += QString("    <th>Value</th>\n");
    table += QString("    <th>ESD</th>\n");
    table += QString("  </tr>\n");

    for (int r = 0; r < rows; ++r) {
        global::Result result = ttable.at(r);

        if (result.error == QString()) {
            table += QString("  <tr>\n");
            table += QString("    <td width=\"50%\">%1</td>\n").arg(result.name);
            table += QString("    <td width=\"25%\">%1</td>\n").arg(result.value, 0, 'f', digits(result.esd));
            table += QString("    <td width=\"25%\">%1</td>\n").arg(result.esd, 0, 'f', digits(result.esd));
            table += QString("  </tr>\n");
        } else {
            if (skipErrors) {
                continue;
            } else  {
                table += QString("  <tr>\n");
                table += QString("    <td width=\"50%\">%1</td>\n").arg(result.name);
                table += QString("    <td width=\"25%\">%1</td>\n").arg(result.error);
                table += QString("    <td width=\"25%\"></td>\n");
                table += QString("  </tr>\n");
            }
        }
    }

    table += "</table>\n<br>\n";
    return table;
}

QString BgmnHtmlReportGenerator::localGoalsTable(const QDomElement &element)
{
    if (element.attribute("status") == "false") return QString();

    QStringList phases = lparser.getPhaseNames();
    QString table;
    table += QString("<h1>%1</h1>\n").arg(element.attribute("display"));

    for (int i = 0; i < phases.size(); ++i) {
        QString phase = phases.at(i);

        QList<global::Result> ttable = lparser.getLocalGoals(phase, projectData["localGoalsIncludes"].toStringList());
        int rows = ttable.size();

        table += QString("<h2>%1</h2>\n").arg(phase);
        table += QString("<table id=\"tableLocalGoals\">\n");
        table += QString("  <tr>\n");
        table += QString("    <th>Parameter</th>\n");
        table += QString("    <th>Value</th>\n");
        table += QString("    <th>ESD</th>\n");
        table += QString("  </tr>\n");

        table += QString("  <tr>\n");
        table += QString("    <td width=\"50%\">%1</td>\n").arg("Refined composition");
        table += QString("    <td colspan=\"2\">%1</td>\n").arg(lparser.getSumFormula(phase));
        table += QString("  </tr>\n");

        for (int r = 0; r < rows; ++r) {
            global::Result result = ttable.at(r);

            if (result.error == QString()) {
                table += QString("  <tr>\n");
                table += QString("    <td width=\"50%\">%1</td>\n").arg(result.name);
                table += QString("    <td width=\"25%\">%1</td>\n").arg(result.value, 0, 'f', digits(result.esd));
                table += QString("    <td width=\"25%\">%1</td>\n").arg(result.esd, 0, 'f', digits(result.esd));
                table += QString("  </tr>\n");
            } else {
                if (skipErrors) {
                    continue;
                } else  {
                    table += QString("  <tr>\n");
                    table += QString("    <td width=\"50%\">%1</td>\n").arg(result.name);
                    table += QString("    <td width=\"25%\">%1</td>\n").arg(result.error);
                    table += QString("    <td width=\"25%\"></td>\n");
                    table += QString("  </tr>\n");
                }
            }
        }

        table += QString("</table>\n<br>\n");
    }

    return table;
}

QString BgmnHtmlReportGenerator::getChemistryTable(const QDomElement &element)
{
    if (element.attribute("status") == "false") return QString();

    QString table;
    table += QString("<h1>%1</h1>\n").arg(element.attribute("display"));
    table += projectData["chemistryData"].toString();

    return table;
}

QString BgmnHtmlReportGenerator::getHklListTable(const QDomElement &element)
{
    if (element.attribute("status") == "false") return QString();

    QString parFile(projectData.value("parFileName", QString()).toString());

    if (!QFile::exists(parFile)) return QString();

    bool ok;
    BgmnParParser pparser(parFile, ok);

    if (!ok) return QString();

    // hkl positions will be given in d (nm)
    QVector<Hkl> vec = pparser.getReflections(QString());

    QString table;
    table += QString("<h1>%1</h1>\n").arg(element.attribute("display"));
    table += QString("<table id=\"tablePeakList\">\n");
    table += QString("  <tr>\n");
    table += QString("    <th>Angle [°2&theta;]</th>\n");
    table += QString("    <th>d [nm]</th>\n");
    table += QString("    <th>Phase</th>\n");
    table += QString("    <th>hkl</th>\n");
    table += QString("  </tr>\n");

    double wl = projectData.value("wavelength", 1.54056).toDouble();

    for (int i = 0; i < vec.size(); ++i) {
        double d = 10.0 * vec.at(i).position();
        double tt = global::Functions::dToTwoTheta(d, wl);
        table += QString("  <tr>\n");
        table += QString("    <td>%1</td>\n").arg(tt, 0, 'f', 4);
        table += QString("    <td>%1</td>\n").arg(d, 0, 'f', 4);
        table += QString("    <td>%1</td>\n").arg(vec.at(i).phase());
        table += QString("    <td>%1</td>\n").arg(vec.at(i).hkl());
        table += QString("  </tr>\n");
    }

    table += QString("</table>\n");
    return table;
}

QString BgmnHtmlReportGenerator::diffPatternSection(const QDomElement &element)
{
    if (element.attribute("status") == "false") return QString();

    QString s = QString("<h1>%1</h1>\n").arg(element.attribute("display"));

    QDomDocument doc;
    doc.setContent(projectData["diffpatternSvg"].toByteArray());

    QDomElement docElem = doc.documentElement();
    docElem.setAttribute("class", "diff-pattern");
    docElem.removeAttribute("width");
    docElem.removeAttribute("height");

    QTextStream str(&s);
    docElem.save(str, 4);
    str.flush();

    return s += "\n<br>\n";
}

int BgmnHtmlReportGenerator::digits(double d)
{
    double l = log10(float(d));
    if (l > 0.0) return 0;
    return int(floor(float(abs(l)))) + 1;
}

QString BgmnHtmlReportGenerator::composeStyleSheed()
{
    QFont f("Helvetica", 10);
    QColor hc("#7ec3eb");
    QColor rc("#e5f4fb");
    QColor bc("#797972");

    QDomNodeList nl = docStructure.documentElement().elementsByTagName("style");

    if (nl.size()) {
        QDomElement styleEl = nl.at(0).toElement();
        bool useCss = styleEl.attribute("useStyleSheet", "1") == "1" ? true : false;
        QString cssFile = styleEl.attribute("styleSheet", QString());

        if (cssFile.isEmpty()) cssFile = ":/resources/report.css";
        if (useCss) return BgmnFileIO::readTextFile(cssFile);

        if (styleEl.hasAttribute("styleFont"))        f.fromString(styleEl.attribute("styleFont", f.toString()));
        if (styleEl.hasAttribute("styleColorHeader")) hc = QColor(styleEl.attribute("styleColorHeader", hc.name()));
        if (styleEl.hasAttribute("styleColorRow"))    rc = QColor(styleEl.attribute("styleColorRow",    rc.name()));
        if (styleEl.hasAttribute("styleColorBorder")) bc = QColor(styleEl.attribute("styleColorBorder", bc.name()));
    }

    QString fontBase("Sans-serif");
    if (f.styleHint() == QFont::Serif)     fontBase = "Serif";
    if (f.styleHint() == QFont::Monospace) fontBase = "Monospace";

    QString fontString = QString("    font-family: %1, ").arg(f.family());
    if (f.substitutes(f.family()).size()) fontString += QString("%1, ").arg(f.substitutes(f.family()).join(", "));
    fontString += fontBase + ";\n";

    QString css;
    css += "html, body {\n";
    css += "    width: 210mm;\n";
    css += "    height: 297mm;\n";
    css += "    margin-left: auto;\n";
    css += "    margin-right: auto;\n";
    css += "}\n\n";

    css += "body {\n";
    css += "    -webkit-print-color-adjust: exact;\n";
    css += "    color-adjust: exact;\n";
    css += "}\n\n";

    css += QString("p {\n");
    css += fontString;
    css += QString("    font-size: %1pt;\n").arg(f.pointSize());
    css += QString("    font-weight: normal;\n");
    css += QString("}\n\n");

    css += QString("h1 {\n");
    css += fontString;
    css += QString("    font-size: %1pt;\n").arg(f.pointSize() + 2);
    css += QString("    font-weight: bold;\n");
    css += QString("    margin-top: %1pt;\n").arg(f.pointSize() * 2);
    css += QString("}\n\n");

    css += QString("h2 {\n");
    css += fontString;
    css += QString("    font-size: %1pt;\n").arg(f.pointSize());
    css += QString("    font-weight: bold;\n");
    css += QString("    margin-top: %1pt;\n").arg(int(f.pointSize() * 1.6));
    css += QString("}\n\n");

    css += QString("table, th, td {\n");
    css += fontString;
    css += QString("    font-size: %1pt;\n").arg(f.pointSize());
    css += QString("    border: 1px solid %1;\n").arg(bc.name());
    css += QString("    border-collapse: collapse;\n");
    css += QString("}\n\n");

    css += "table {\n";
    css += "    width: 100%;\n";
    css += "    text-align: left;\n";
    css += "}\n\n";

    css += "th, td {\n";
    css += "    padding: 1mm;\n";
    css += "}\n\n";

    css += QString("tr:nth-child(even) {\n");
    css += QString("    background-color: %1;\n").arg(rc.name());
    css += QString("}\n\n");

    css += QString("th {\n");
    css += QString("    text-align: left;\n");
    css += QString("    background-color: %1;\n").arg(hc.name());
    css += QString("    font-weight: bold;\n");
    css += QString("}\n\n");

    css += QString(".diff-pattern {\n");
    css += QString("    max-width: 100%;\n");
    css += QString("    height: auto;\n");
    css += QString("    display: block;\n");
    css += QString("    width: 100%;\n");
    css += QString("    margin-left: auto;\n");
    css += QString("    margin-right: auto;\n");
    css += QString("}\n");

    css += "@media all {\n";
    css += "    .page-break {\n";
    css += "        display: none;\n";
    css += "    }\n";
    css += "}\n";

    css += "@media print {\n";
    css += "    .page-break {\n";
    css += "        display: block;\n";
    css += "        page-break-before: always;\n";
    css += "    }\n";
    css += "}\n";


    return css;
}
