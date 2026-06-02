/***************************************************************************
                          prefpagesbgmnreport.cpp  -  description
                             -------------------
    begin                : Jun 25 18:55:07 CET 2019
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

#include <QFileDialog>
#include <QSvgWidget>
#include <QSvgRenderer>
#include <QPainter>
#include <QFontDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QSpinBox>
#include <QInputDialog>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include "../libXrdIO/bgmnfileio.h"
#include "prefpagebgmnreport.h"
#include "ui_prefpagebgmnreport.h"

PrefPageBgmnReport::PrefPageBgmnReport(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageBgmnReport)
{
    ui->setupUi(this);
    customDlg = nullptr;
}

PrefPageBgmnReport::~PrefPageBgmnReport()
{
    delete ui;
}

void PrefPageBgmnReport::initUi()
{
    QStringList headings;
    headings << tr("Section") << tr("Text / Value");
    ui->treeWidgetStructure->setColumnCount(headings.count());
    ui->treeWidgetStructure->setHeaderLabels(headings);

    ui->toolButtonAddCustom->setEnabled(true);
    ui->toolButtonRemoveCustom->setEnabled(false);
    ui->toolButtonPageBreakAdd->setEnabled(false);
    ui->toolButtonPageBreakRemove->setEnabled(false);
    ui->toolButtonItemUp->setEnabled(false);
    ui->toolButtonItemDown->setEnabled(false);

    connect(ui->treeWidgetStructure, SIGNAL(itemSelectionChanged()), this, SLOT(updateStructureButtonsEnabled()));
    connect(ui->treeWidgetStructure, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(structureItemChanged(QTreeWidgetItem*,int)));
    connect(ui->treeWidgetStructure, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(changeStructureText(QTreeWidgetItem*, int)));

    initSettings();
}

void PrefPageBgmnReport::initSettings()
{
    // init default font
    reportFont.setFamily("Helvetica");
    reportFont.setPointSize(10);
    reportFont.fromString(settings->value("bgmnProject/report/font", reportFont.toString()).toString());

    ui->checkBoxSkipErrors->setChecked(settings->value("bgmnProject/report/skipErros", false).toBool());
    parseXmlSettings(settings->value("bgmnProject/report/documentStructure", BgmnFileIO::readTextFile(":/resources/report-structure.xml")).toString());
    ui->treeWidgetStructure->header()->restoreState(settings->value("preferencesDialog/reportHeaderState", QByteArray()).toByteArray());
}

void PrefPageBgmnReport::initSettingsHeader(const QString &logoFile)
{
    if (logoFile == ":/resources/profex-header.svg") {
        ui->lineEditLogo->clear();
    } else {
        ui->lineEditLogo->setText(logoFile);
    }

    previewImage(logoFile);
}

void PrefPageBgmnReport::initSettingsStyle(const QString &cssFile, bool useCss, const QFont &rFont, const QString &hC, const QString &rC, const QString &bC)
{
    hCol = hC;
    rCol = rC;
    bCol = bC;

    ui->pushButtonColorHeader->setStyleSheet(QString("background-color:%1").arg(hCol));
    ui->pushButtonColorRows->setStyleSheet(QString("background-color:%1").arg(rCol));
    ui->pushButtonColorBorder->setStyleSheet(QString("background-color:%1").arg(bCol));

    reportFont = rFont;
    ui->lineEditFont->setText(QString("%1 %2 pt").arg(reportFont.family()).arg(reportFont.pointSize()));

    ui->checkBoxUseCss->setChecked(useCss);
    ui->lineEditCssFile->setEnabled(useCss);
    ui->toolButtonOpenCss->setEnabled(useCss);
    ui->groupBoxStyle->setEnabled(!useCss);

    if (cssFile == ":/resources/report.css") {
        ui->lineEditCssFile->clear();
    } else {
        ui->lineEditCssFile->setText(cssFile);
    }

    // set the font asynchronously, because it can delay the start of the widget significantly
    QTimer::singleShot(100, [=]() {
        ui->lineEditFont->setFont(reportFont);
    });
}

void PrefPageBgmnReport::saveSettings()
{
    settings->setValue("preferencesDialog/reportHeaderState", ui->treeWidgetStructure->header()->saveState());
    settings->setValue("bgmnProject/report/documentStructure", composeXmlSettings());
    settings->setValue("bgmnProject/report/skipErros", ui->checkBoxSkipErrors->isChecked());
}

void PrefPageBgmnReport::parseXmlSettings(const QString &xml)
{
    QString uname = qgetenv("USER");
    if (uname.isEmpty()) uname = qgetenv("USERNAME");

    bool oldState = ui->treeWidgetStructure->blockSignals(true);
    ui->treeWidgetStructure->clear();

    QDomDocument doc("XmlReportStruc");
    doc.setContent(xml);

    QDomElement docEl = doc.documentElement();
    QDomElement secEl = docEl.firstChildElement();

    while (!secEl.isNull()) {
        if (secEl.tagName() == "pageBreak") {
            QTreeWidgetItem *pbIt = new QTreeWidgetItem(QStringList() << "<Page break>", PAGEBREAK);
            pbIt->setData(1, Qt::UserRole, "false");
            pbIt->setFlags(pbIt->flags() & (~Qt::ItemIsUserCheckable));
            ui->treeWidgetStructure->addTopLevelItem(pbIt);
        }

        if (secEl.tagName() == "section") {
            qDebug() << QString("  element: %1").arg(secEl.attribute("title"));
            QTreeWidgetItem *secIt = new QTreeWidgetItem(ui->treeWidgetStructure, PERMANENT);
            secIt->setData(0, Qt::UserRole, secEl.attribute("ident"));
            secIt->setData(1, Qt::UserRole, secEl.attribute("doubleClickable"));
            secIt->setText(0, secEl.attribute("title"));
            secIt->setText(1, secEl.attribute("display"));

            if (secEl.attribute("checkable") == "false") {
                secIt->setFlags(secIt->flags() & (~Qt::ItemIsUserCheckable));
            } else {
                secIt->setCheckState(0, secEl.attribute("status") == "true" ? Qt::Checked : Qt::Unchecked);
            }

            QDomNodeList subsections = secEl.elementsByTagName("subsection");
            for (int j = 0; j < subsections.size(); ++j) {
                QDomElement subEl = subsections.at(j).toElement();
                qDebug() << QString("    sub element: %1").arg(subEl.attribute("title"));
                QTreeWidgetItem *subIt = new QTreeWidgetItem(secIt, PERMANENT);

                subIt->setData(0, Qt::UserRole, subEl.attribute("ident"));
                subIt->setData(1, Qt::UserRole, subEl.attribute("doubleClickable"));
                subIt->setText(0, subEl.attribute("title"));
                subIt->setText(1, subEl.attribute("display"));

                if (subEl.attribute("ident") == "strucOpName" && subEl.attribute("display").isEmpty()) {
                    subIt->setText(1, uname);
                }

                if (subEl.attribute("checkable") == "false") {
                    subIt->setFlags(subIt->flags() & (~Qt::ItemIsUserCheckable));
                } else {
                    subIt->setCheckState(0, subEl.attribute("status") == "true" ? Qt::Checked : Qt::Unchecked);
                }
            }
        }

        if (secEl.tagName() == "custom") {
            QTreeWidgetItem *secIt = new QTreeWidgetItem(ui->treeWidgetStructure, CUSTOM);
            secIt->setData(0, Qt::UserRole, secEl.attribute("ident"));
            secIt->setData(1, Qt::UserRole, secEl.attribute("doubleClickable"));
            secIt->setData(1, Qt::UserRole+1, secEl.text());
            secIt->setText(0, secEl.attribute("title"));
            secIt->setText(1, secEl.attribute("display"));

            if (secEl.attribute("checkable") == "false") {
                secIt->setFlags(secIt->flags() & (~Qt::ItemIsUserCheckable));
            } else {
                secIt->setCheckState(0, secEl.attribute("status") == "true" ? Qt::Checked : Qt::Unchecked);
            }
        }

        if (secEl.tagName() == "banner") {
            initSettingsHeader(secEl.attribute("fileName", QString()));
        }

        if (secEl.tagName() == "style") {
            ui->lineEditCssFile->setText(secEl.attribute("styleSheet", QString()));

            initSettingsStyle(secEl.attribute("styleSheet", QString()),
                              secEl.attribute("useStyleSheet", "1") == "1" ? true : false,
                              secEl.attribute("styleFont", reportFont.toString()),
                              secEl.attribute("styleColorHeader", "#7ec3eb"),
                              secEl.attribute("styleColorRow", "#e5f4fb"),
                              secEl.attribute("styleColorBorder", "#797972"));
        }

        if (secEl.tagName() == "skipErrors") {
            bool se = secEl.attribute("status", "false") == "true";
            ui->checkBoxSkipErrors->setChecked(se);
        }

        secEl = secEl.nextSiblingElement();
    }

    ui->treeWidgetStructure->expandAll();
    ui->treeWidgetStructure->blockSignals(oldState);
}

QString PrefPageBgmnReport::composeXmlSettings()
{
    QDomDocument doc("profexReportStructure");
    QDomElement root = doc.createElement("profexReportStructure");
    doc.appendChild(root);

    for (int i = 0; i < ui->treeWidgetStructure->topLevelItemCount(); ++i) {
        QTreeWidgetItem *secIt = ui->treeWidgetStructure->topLevelItem(i);

        if (static_cast<ItemType>(secIt->type()) == PAGEBREAK) {
            QDomElement pbEl = doc.createElement("pageBreak");
            root.appendChild(pbEl);
        }

        if (static_cast<ItemType>(secIt->type()) == PERMANENT) {
            QDomElement secEl = doc.createElement("section");
            secEl.setAttribute("ident", secIt->data(0, Qt::UserRole).toString());
            secEl.setAttribute("doubleClickable", secIt->data(1, Qt::UserRole).toString());
            secEl.setAttribute("title", secIt->text(0));
            secEl.setAttribute("display", secIt->text(1));

            if (secIt->flags().testFlag(Qt::ItemIsUserCheckable)) {
                secEl.setAttribute("checkable", "true");
                secEl.setAttribute("status", secIt->checkState(0) == Qt::Checked ? "true" : "false");
            } else {
                secEl.setAttribute("checkable", "false");
            }

            root.appendChild(secEl);

            for (int j = 0; j < secIt->childCount(); ++j) {
                QTreeWidgetItem *subIt = secIt->child(j);
                QDomElement subEl = doc.createElement("subsection");
                subEl.setAttribute("ident", subIt->data(0, Qt::UserRole).toString());
                subEl.setAttribute("doubleClickable", subIt->data(1, Qt::UserRole).toString());
                subEl.setAttribute("title", subIt->text(0));
                subEl.setAttribute("display", subIt->text(1));

                if (subIt->flags().testFlag(Qt::ItemIsUserCheckable)) {
                    subEl.setAttribute("checkable", "true");
                    subEl.setAttribute("status", subIt->checkState(0) == Qt::Checked ? "true" : "false");
                } else {
                    subEl.setAttribute("checkable", "false");
                }

                secEl.appendChild(subEl);
            }
        }

        if (static_cast<ItemType>(secIt->type()) == CUSTOM) {
            QDomElement secEl = doc.createElement("custom");
            secEl.setAttribute("ident", secIt->data(0, Qt::UserRole).toString());
            secEl.setAttribute("doubleClickable", "true");
            secEl.setAttribute("title", secIt->text(0));
            secEl.setAttribute("display", secIt->text(1));
            secEl.setAttribute("checkable", "true");
            secEl.setAttribute("status", secIt->checkState(0) == Qt::Checked ? "true" : "false");

            QDomText secText = doc.createTextNode(secIt->data(1, Qt::UserRole+1).toString());
            secEl.appendChild(secText);

            root.appendChild(secEl);
        }
    }

    QDomElement bannerEl = doc.createElement("banner");
    bannerEl.setAttribute("fileName", ui->lineEditLogo->text().isEmpty() ? ":/resources/profex-header.svg" : ui->lineEditLogo->text());
    root.appendChild(bannerEl);

    QDomElement styleEl = doc.createElement("style");
    styleEl.setAttribute("styleSheet", ui->lineEditCssFile->text().isEmpty() ? ":/resources/report.css" : ui->lineEditCssFile->text());
    styleEl.setAttribute("useStyleSheet", ui->checkBoxUseCss->isChecked() ? "1" : "0");
    styleEl.setAttribute("styleFont", reportFont.toString());
    styleEl.setAttribute("styleColorHeader", hCol);
    styleEl.setAttribute("styleColorRow", rCol);
    styleEl.setAttribute("styleColorBorder", bCol);
    root.appendChild(styleEl);

    QDomElement skipErrorEl = doc.createElement("skipErrors");
    skipErrorEl.setAttribute("status", ui->checkBoxSkipErrors->isChecked() ? "true" : "false");
    root.appendChild(skipErrorEl);

    return doc.toString();
}

void PrefPageBgmnReport::updateStructureButtonsEnabled()
{
    QList<QTreeWidgetItem*> l = ui->treeWidgetStructure->selectedItems();

    if (!l.size()) {
        ui->toolButtonItemUp->setEnabled(false);
        ui->toolButtonItemDown->setEnabled(false);
        ui->toolButtonPageBreakAdd->setEnabled(false);
        ui->toolButtonPageBreakRemove->setEnabled(false);
        ui->toolButtonRemoveCustom->setEnabled(false);
        return;
    }

    QTreeWidgetItem *it = l.first(); // we ignore all other items, but selection mode is single anyway

    if (static_cast<ItemType>(it->type()) == PAGEBREAK) {
        ui->toolButtonPageBreakAdd->setEnabled(false);
        ui->toolButtonPageBreakRemove->setEnabled(true);
        ui->toolButtonItemUp->setEnabled(false);
        ui->toolButtonItemDown->setEnabled(false);
        return;
    }

    if (static_cast<ItemType>(it->type()) == CUSTOM) {
        ui->toolButtonRemoveCustom->setEnabled(true);
    } else {
        ui->toolButtonRemoveCustom->setEnabled(false);
    }

    ui->toolButtonPageBreakAdd->setEnabled(true);
    ui->toolButtonPageBreakRemove->setEnabled(false);

    int idx = 0;

    if (!it->parent()) {
        // it's a toplevel item
        idx = ui->treeWidgetStructure->indexOfTopLevelItem(it);
    } else {
        // it's a child item
        idx = it->parent()->indexOfChild(it);
    }

    if (idx == 0) {
        ui->toolButtonItemUp->setEnabled(false);
        ui->toolButtonItemDown->setEnabled(true);
    } else if (idx == ui->treeWidgetStructure->topLevelItemCount() - 1) {
        ui->toolButtonItemUp->setEnabled(true);
        ui->toolButtonItemDown->setEnabled(false);
    } else {
        ui->toolButtonItemUp->setEnabled(true);
        ui->toolButtonItemDown->setEnabled(true);
    }
}

void PrefPageBgmnReport::itemUp()
{
    moveCurrentItem(-1);
}

void PrefPageBgmnReport::itemDown()
{
    moveCurrentItem(1);
}

void PrefPageBgmnReport::moveCurrentItem(int d)
{
    QList<QTreeWidgetItem*> l = ui->treeWidgetStructure->selectedItems();
    if (!l.size()) return;

    QTreeWidgetItem *it = l.first();
    QTreeWidgetItem *par = it->parent();

    if (par) {
        // it's a child item
        int idx = par->indexOfChild(it);
        it = par->takeChild(idx);
        par->insertChild(idx+d, it);
        ui->treeWidgetStructure->setCurrentItem(it);
    } else {
        // it's a toplevel item
        int idx = ui->treeWidgetStructure->indexOfTopLevelItem(it);
        it = ui->treeWidgetStructure->takeTopLevelItem(idx);
        ui->treeWidgetStructure->insertTopLevelItem(idx+d, it);
        ui->treeWidgetStructure->setCurrentItem(it);
    }
}

void PrefPageBgmnReport::customAdd()
{
    if (!customDlg) customDlg = new ReportCustomSectionDialog(this);
    customDlg->clearData();

    if (customDlg->exec() == QDialog::Rejected) return;

    int idx = 0;
    QList<QTreeWidgetItem*> l = ui->treeWidgetStructure->selectedItems();

    if (l.size()) {
        if (l.first()->parent()) idx = ui->treeWidgetStructure->indexOfTopLevelItem(l.first()->parent()) + 1;
        else                     idx = ui->treeWidgetStructure->indexOfTopLevelItem(l.first()) + 1;
    }

    QTreeWidgetItem *secIt = new QTreeWidgetItem(CUSTOM);
    secIt->setData(0, Qt::UserRole, "strucCustom");
    secIt->setData(1, Qt::UserRole, "true");
    secIt->setText(0, customDlg->getName());
    secIt->setText(1, "");
    secIt->setData(1, Qt::UserRole+1, customDlg->getText());
    secIt->setCheckState(0, Qt::Checked);
    ui->treeWidgetStructure->insertTopLevelItem(idx, secIt);
}

void PrefPageBgmnReport::customRemove()
{
    QList<QTreeWidgetItem*> l = ui->treeWidgetStructure->selectedItems();
    if (!l.size()) return;

    if (static_cast<ItemType>(l.first()->type()) == CUSTOM) {
        QTreeWidgetItem *it = ui->treeWidgetStructure->takeTopLevelItem(ui->treeWidgetStructure->indexOfTopLevelItem(l.first()));
        delete it;
    }
}

void PrefPageBgmnReport::pageBreakAdd()
{
    QList<QTreeWidgetItem*> l = ui->treeWidgetStructure->selectedItems();
    if (!l.size()) return;

    if (static_cast<ItemType>(l.first()->type()) != PAGEBREAK) {
        QTreeWidgetItem *pbIt = new QTreeWidgetItem(PAGEBREAK);
        pbIt->setText(0, "<Page break>");
        pbIt->setData(1, Qt::UserRole, "false");
        pbIt->setFlags(pbIt->flags() & (~Qt::ItemIsUserCheckable));
        ui->treeWidgetStructure->insertTopLevelItem(ui->treeWidgetStructure->indexOfTopLevelItem(l.first()) + 1, pbIt);
    }
}

void PrefPageBgmnReport::pageBreakRemove()
{
    QList<QTreeWidgetItem*> l = ui->treeWidgetStructure->selectedItems();
    if (!l.size()) return;

    if (static_cast<ItemType>(l.first()->type()) == PAGEBREAK) {
        QTreeWidgetItem *it = ui->treeWidgetStructure->takeTopLevelItem(ui->treeWidgetStructure->indexOfTopLevelItem(l.first()));
        delete it;
    }
}

void PrefPageBgmnReport::structureItemChanged(QTreeWidgetItem *it, int c)
{
    if (c > 0) return;

    if (it->checkState(0) == Qt::Unchecked) {
        for (int i = 0; i < it->childCount(); ++i) {
            it->child(i)->setCheckState(0, Qt::Unchecked);
        }
    }
}

void PrefPageBgmnReport::changeStructureText(QTreeWidgetItem *it, int c)
{
    Q_UNUSED(c);

    if (static_cast<ItemType>(it->type()) == PAGEBREAK) {
        return;
    }

    if (static_cast<ItemType>(it->type()) == PERMANENT) {
        QString ident = it->data(0, Qt::UserRole).toString();
        if (ident == "strucStatistics") return;

        bool ok = true;

        if (ident == "strucDiffPatternAspect") {
            double d = it->text(1).toDouble(&ok);
            if (!ok) d = 1.5;
            d = QInputDialog::getDouble(this, tr("Change aspect ratio"), tr("Aspect ratio"), d, 0.5, 4.0, 4, &ok);
            if (!ok) return;
            it->setText(1, QString("%1").arg(d, 0, 'f', 4));
        } else if (ident == "strucChemTableMode") {
            QString s = it->text(1);
            QStringList options = QStringList() << "Elements by weight-%" << "Elements by atom-%" << "Oxides by weight-%";
            int cur = options.indexOf(s);
            s = QInputDialog::getItem(this, tr("Change chemistry table mode"), tr("Mode"), options, cur < 0 ? 0 : cur, false, &ok);
            if (!ok) return;
            it->setText(1, s);
        } else {
            QString s = it->text(1);
            s = QInputDialog::getText(this, tr("Change display text"), tr("Display text"), QLineEdit::Normal, s, &ok);
            if (!ok) return;
            it->setText(1, s);
        }
    }

    if (static_cast<ItemType>(it->type()) == CUSTOM) {
        if (!customDlg) customDlg = new ReportCustomSectionDialog(this);

        customDlg->setContent(it->text(0), it->data(1, Qt::UserRole+1).toString());
        if (customDlg->exec() == QDialog::Accepted) {
            it->setText(0, customDlg->getName());
            it->setText(1, "");
            it->setData(1, Qt::UserRole+1, customDlg->getText());
        }
    }
}

void PrefPageBgmnReport::openCss()
{
    QFileInfo fi(ui->lineEditCssFile->text());
    QString f = QFileDialog::getOpenFileName(this, tr("Select CSS file"), fi.absolutePath(), tr("Styls sheets (*.css *.CSS)"));

    if (QFile::exists(f)) {
        ui->lineEditCssFile->setText(f);
    }
}

void PrefPageBgmnReport::openHeaderLogo()
{
    QFileInfo fi(ui->lineEditLogo->text());
    QString f = QFileDialog::getOpenFileName(this, tr("Select header logo"), fi.absolutePath(), tr("Vector graphics (*.svg *.SVG)"));

    if (QFile::exists(f)) {
        ui->lineEditLogo->setText(f);
        previewImage(f);
    }
}

void PrefPageBgmnReport::logoTextChanged()
{
    previewImage(ui->lineEditLogo->text());
}

void PrefPageBgmnReport::previewImage(const QString &s)
{
    QString pm = s;

    if (!QFile::exists(pm)) {
        pm = ":/resources/profex-header.svg";
    }

    int dw = int(200.0 * 150.0 / 25.4); // = 200mm at a resolution of 150 dpi

    QSvgRenderer *renderer = new QSvgRenderer(pm);

    int w = renderer->defaultSize().width();
    int h = renderer->defaultSize().height();

    // we want the pixmap to be at least "dw" pixels wide. Increase the size if necessary
    if (w < dw) {
        double s = double(dw) / double (w);
        w = dw;
        h = int(h * s);
    }

    QPixmap pixmap(w, h);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    renderer->render(&painter);

    ui->previewLabel->setPixmap(pixmap);
}

void PrefPageBgmnReport::selectHeaderColor()
{
    QColor newHCol = QColorDialog::getColor(QColor(hCol), this, tr("Table header color"));

    if (newHCol.isValid()) {
        hCol = newHCol.name();
        ui->pushButtonColorHeader->setStyleSheet(QString("background-color:%1").arg(hCol));
    }
}

void PrefPageBgmnReport::selectRowColor()
{
    QColor newRCol = QColorDialog::getColor(QColor(rCol), this, tr("Table row color"));

    if (newRCol.isValid()) {
        rCol = newRCol.name();
        ui->pushButtonColorRows->setStyleSheet(QString("background-color:%1").arg(rCol));
    }
}

void PrefPageBgmnReport::selectBorderColor()
{
    QColor newBCol = QColorDialog::getColor(QColor(bCol), this, tr("Table border color"));

    if (newBCol.isValid()) {
        bCol = newBCol.name();
        ui->pushButtonColorBorder->setStyleSheet(QString("background-color:%1").arg(bCol));
    }
}

void PrefPageBgmnReport::selectFont()
{
    bool ok;
    QFont newFont = QFontDialog::getFont(&ok, reportFont, this, tr("Report font"));

    if (ok) {
        reportFont = newFont;
        ui->lineEditFont->setFont(reportFont);
        ui->lineEditFont->setText(QString("%1 %2 pt").arg(reportFont.family()).arg(reportFont.pointSize()));
    }
}

void PrefPageBgmnReport::toggleCss(bool b)
{
    ui->groupBoxStyle->setEnabled(!b);
    ui->lineEditCssFile->setEnabled(b);
    ui->toolButtonOpenCss->setEnabled(b);
}

void PrefPageBgmnReport::readFromFile()
{
    QString s = QFileDialog::getOpenFileName(this, tr("Import settings from file"), QDir::homePath(), tr("XML files (*.xml *.XML)"));
    if (s.isEmpty()) return;

    parseXmlSettings(BgmnFileIO::readTextFile(s));
    qDebug() << QString("PrefPageBgmnReport::readFromFile(): Report settings imported from file %1").arg(s);
}

void PrefPageBgmnReport::saveToFile()
{
    QString s = QFileDialog::getSaveFileName(this, tr("Export settings to file"), QDir::homePath(), tr("XML files (*.xml *.XML)"));
    if (s.isEmpty()) return;

    if (BgmnFileIO::writeTextFile(s, composeXmlSettings())) {
        qDebug() << QString("PrefPageBgmnReport::saveToFile(): Report structure exported to %1").arg(s);
    } else {
        qDebug() << QString("PrefPageBgmnReport::saveToFile(): Could not export report structure to file %1").arg(s);
    }
}

void PrefPageBgmnReport::resetStructure()
{
    if (QMessageBox::question(this, tr("Reset Report"),
                               tr("Do you want to reset the report structure to the default?"))
            != QMessageBox::Yes) return;

    parseXmlSettings(settings->value("bgmnProject/report/documentStructure", BgmnFileIO::readTextFile(":/resources/report-structure.xml")).toString());
}

/************************** pixmap label subclass ********************************/

PrefPagePixmapLabel::PrefPagePixmapLabel(QWidget *parent) :
QLabel(parent)
{
    this->setMinimumSize(1, 1);
    setScaledContents(false);
}

void PrefPagePixmapLabel::setPixmap(const QPixmap & p)
{
    pixmap = p;
    QLabel::setPixmap(scaledPixmap());
}

int PrefPagePixmapLabel::heightForWidth(int w) const
{
    if (pixmap.isNull()) return this->height();
    return int(double(pixmap.height()) * double(w) / double(pixmap.width()));
}

QSize PrefPagePixmapLabel::sizeHint() const
{
    int w = this->width();
    return QSize(w, heightForWidth(w));
}

QPixmap PrefPagePixmapLabel::scaledPixmap() const
{
    return pixmap.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void PrefPagePixmapLabel::resizeEvent(QResizeEvent *)
{
     if(!pixmap.isNull()) QLabel::setPixmap(scaledPixmap());
}

