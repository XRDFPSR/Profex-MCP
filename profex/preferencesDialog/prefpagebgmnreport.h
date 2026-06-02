/***************************************************************************
                          prefpagesbgmnreport.h  -  description
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

#ifndef PREFPAGEBGMNREPORT_H
#define PREFPAGEBGMNREPORT_H

#include "prefpagetemplate.h"
#include "reportcustomsectiondialog.h"
#include "../libXrdIO/structs.h"
#include <QLabel>
#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace Ui {
class PrefPageBgmnReport;
}

enum ItemType {PERMANENT, PAGEBREAK, CUSTOM};

class PrefPageBgmnReport : public PrefPageTemplate
{
    Q_OBJECT

public:
    explicit PrefPageBgmnReport(QWidget *parent = 0);
    ~PrefPageBgmnReport();

    void initUi() override;
    void initSettings() override;
    void saveSettings() override;
    QString name() {return "BgmnReport";}

private:
    Ui::PrefPageBgmnReport *ui;
    QString hCol;
    QString rCol;
    QString bCol;
    QFont reportFont;
    ReportCustomSectionDialog *customDlg;

    void initSettingsHeader(const QString &logoFile);
    void initSettingsStyle(const QString &cssFile, bool useCss, const QFont &rFont, const QString &hC, const QString &rC, const QString &bC);

    void parseXmlSettings(const QString &);
    QString composeXmlSettings();
    void moveCurrentItem(int);
    void previewImage(const QString &);

private slots:
    void itemUp();
    void itemDown();
    void pageBreakAdd();
    void pageBreakRemove();
    void customAdd();
    void customRemove();
    void updateStructureButtonsEnabled();
    void structureItemChanged(QTreeWidgetItem *, int);
    void changeStructureText(QTreeWidgetItem *, int);
    void openHeaderLogo();
    void logoTextChanged();
    void selectHeaderColor();
    void selectRowColor();
    void selectBorderColor();
    void selectFont();
    void toggleCss(bool);
    void openCss();
    void saveToFile();
    void readFromFile();
    void resetStructure();
};

class PrefPagePixmapLabel : public QLabel
{
    Q_OBJECT

public:
    explicit PrefPagePixmapLabel(QWidget *parent = 0);
    virtual int heightForWidth(int) const;
    virtual QSize sizeHint() const;
    QPixmap scaledPixmap() const;

public slots:
    void setPixmap(const QPixmap &);
    void resizeEvent(QResizeEvent *);

private:
    QPixmap pixmap;
};

#endif // PREFPAGEBGMNREPORT_H
