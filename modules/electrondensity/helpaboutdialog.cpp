/***************************************************************************
                          helpaboutdialog.cpp  -  description
                             -------------------
    begin                : Sun Mar 29 12:00:00 CEST 2015
    copyright            : (C) 2015 by Nicola Doebelin
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

#include <QDateTime>
#include <QThread>
#include <QStandardPaths>
#include <QClipboard>
#include <QSysInfo>

#include "helpaboutdialog.h"
#include "ui_helpaboutdialog.h"
#include "../libXrdIO/structs.h"
#include "../libXrdIO/bgmnfileio.h"

HelpAboutDialog::HelpAboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpAboutDialog)
{
    ui->setupUi(this);
}

HelpAboutDialog::~HelpAboutDialog()
{
    delete ui;
}

int HelpAboutDialog::exec()
{
    init();
    return QDialog::exec();
}

void HelpAboutDialog::init()
{
    ui->labelVersion->setText(getAboutText());
    ui->labelVersion->setTextFormat(Qt::RichText);
    ui->labelVersion->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->labelVersion->setOpenExternalLinks(true);
    ui->textEditLicense->setText(BgmnFileIO::readTextFile(":/resources/gpl-2.0.html"));
    ui->textEditSysInfo->setText(getSettingsText());
    ui->textEditAck->setText(getAckText());
}

QString HelpAboutDialog::getAboutText()
{
    QString strAbout;
    strAbout += "<h1>ProfexED</h1>";
    strAbout += QString("Electron Density Map Module<br>");
    strAbout += QString("Version %1<br><br>").arg(version);
    strAbout += QString("<a href=\"https://www.profex-xrd.org\">https://www.profex-xrd.org</a><br>");
    strAbout += QString("(c) 2003-%1 by Nicola Döbelin").arg(QDateTime::currentDateTime().toString("yyyy"));
    return strAbout;
}

QString HelpAboutDialog::getSettingsText()
{
    QString strSetInfo;

    strSetInfo += QString("<b>Log file</b><br>%1<br><br>").arg(logDest);
    strSetInfo += QString("<b>Number of CPU cores</b><br>%1<br><br>").arg(QThread::idealThreadCount());
    strSetInfo += QString("<b>Qt compile version</b><br>%1<br><br><b>Qt runtime version</b><br>%2<br><br>").arg(QT_VERSION_STR).arg(qVersion());
    return strSetInfo;
}

QString HelpAboutDialog::getAckText()
{
    QString strAck;
    strAck += "<b>Qt Toolkit</b><br><a href=\"http://www.qt.io/\">http://www.qt.io/</a><br><br>";
    strAck += "<b>zlib</b><br><a href=\"http://www.zlib.net/\">http://www.zlib.net/</a><br><br>";
    strAck += "<b>QuaZip</b><br><a href=\"http://quazip.sourceforge.net/\">http://quazip.sourceforge.net/</a><br><br>";
    strAck += "<b>Icons created by Safa Ishtiaq</b><br><a href=\"https://www.fiverr.com/safaishtiaq19\">https://www.fiverr.com/safaishtiaq19</a>";
    return strAck;
}

QString HelpAboutDialog::getSysInfoText()
{
    QString sysInfo;

    sysInfo += QString("Operating system: %1\n").arg(QSysInfo::prettyProductName());
    sysInfo += QString("Runtime CPU: %1\n").arg(QSysInfo::currentCpuArchitecture());

    return sysInfo;
}

void HelpAboutDialog::toClipboard()
{
    QString spc("\n----------------------------------------------------------------\n");
    QString txt = spc;
    txt += "Profex version " + version + spc;
    txt += "Date: " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "\n\n\n";

    txt += "Host system:" + spc;
    txt += getSysInfoText() + "\n\n";

    txt += "Configuration:" + spc;
    txt += ui->textEditSysInfo->toPlainText() + "\n";

    txt += "Log file content:" + spc;

    if (logDest.toLower() == "console") {
        txt += "log is printed to console";
    } else {
        QFileInfo fi(logDest);
        if (fi.exists()) txt += BgmnFileIO::readTextFile(logDest);
        else             txt += QString("Log file not found at %1").arg(logDest);
    }

    txt += "\n";

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(txt);
}
