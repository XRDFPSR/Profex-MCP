/***************************************************************************
                          learnprofiledialog.cpp  -  description
                             -------------------
    begin                : Fri Jan 08 09:25:00 CET 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#include "learnprofiledialog.h"
#include "ui_learnprofiledialog.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QPlainTextEdit>
#include <QFont>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QDir>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRect>
#include <QLabel>
#include <QDebug>
#include "../libXrdIO/structs.h"

LearnProfileDialog::LearnProfileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LearnProfileDialog)
{
    ui->setupUi(this);

    settings = SettingsManager::getInstance();

    QFont edFont;
    edFont.fromString(settings->value("config/editorFont", font().toString()).toString());
    ui->textEditPar->setFont(edFont);
    ui->textEditSav->setFont(edFont);
    ui->textEditOutput->setFont(edFont);
    ui->labelParFile->setText("");
    ui->labelSavFile->setText("");
    ui->pushButtonRun->setEnabled(false);
    ui->pushButtonSaveAsPar->setEnabled(false);
    ui->pushButtonSaveAsSav->setEnabled(false);

    int hlmode = settings->getSyntaxHighlightingMode();

    BgmnHighlighter *hlPar = new BgmnHighlighter(hlmode, this);
    hlPar->setDocument(ui->textEditPar->document());
    highlighters.insert(ui->textEditPar, hlPar);

    BgmnHighlighter *hlSav = new BgmnHighlighter(hlmode, this);
    hlSav->setDocument(ui->textEditSav->document());
    highlighters.insert(ui->textEditSav, hlSav);

    parWorkingDir = settings->value("learnedProfileDialog/workingdirPar", QDir::homePath()).toString();
    savWorkingDir = settings->value("learnedProfileDialog/workingdirSav", QDir::homePath()).toString();

    initProcesses();

    restoreGeometry(settings->value("learnedProfileDialog/geometry", QByteArray()).toByteArray());
    ui->splitter->restoreState(settings->value("learnedProfileDialog/splitter", QByteArray()).toByteArray());
}

LearnProfileDialog::~LearnProfileDialog()
{
    settings->setValue("learnedProfileDialog/workingdirPar", parWorkingDir);
    settings->setValue("learnedProfileDialog/workingdirSav", savWorkingDir);

    settings->setValue("learnedProfileDialog/geometry", saveGeometry());
    settings->setValue("learnedProfileDialog/splitter", ui->splitter->saveState());

    delete ui;
}

void LearnProfileDialog::initProcesses()
{
    verzerr = new ProcessHandler(this);
    makegeq = new ProcessHandler(this);

    connect(verzerr, SIGNAL(pollOutput()), this, SLOT(pollOutput()));
    connect(verzerr, SIGNAL(allComplete()), this, SLOT(verzerrComplete()));
    connect(verzerr, SIGNAL(aborted()), this, SLOT(processAborted()));

    connect(makegeq, SIGNAL(pollOutput()), this, SLOT(pollOutput()));
    connect(makegeq, SIGNAL(allComplete()), this, SLOT(makegeqComplete()));
    connect(makegeq, SIGNAL(aborted()), this, SLOT(processAborted()));
}


/*
 * Open a PAR file from disk and show in text editor
 */
void LearnProfileDialog::openPar()
{
    QString s = QFileDialog::getOpenFileName(this, tr("Open PAR file"), parWorkingDir, tr("Parameter file (*.par *.PAR)"));
    QFileInfo fi(s);

    // file dialog aborted?
    if (!fi.exists()) return;

    parFileName = s;
    parWorkingDir = fi.absolutePath();
    ui->labelParFile->setText(parFileName);

    QString parContent = readTextFileFromDisk(parFileName);
    ui->textEditPar->setPlainText(unifyPar(parContent));
    ui->pushButtonSaveAsPar->setEnabled(true);

    // if both file names have been set, enable the run button
    if (!parFileName.isEmpty() && !savFileName.isEmpty()) ui->pushButtonRun->setEnabled(true);
}

/*
 * Open a SAV file from disk and show in text editor
 */
void LearnProfileDialog::openSav()
{
    QString s = QFileDialog::getOpenFileName(this, tr("Open SAV file"), savWorkingDir, tr("Control file (*.sav *.SAV)"));
    QFileInfo fi(s);

    // file dialog aborted?
    if (!fi.exists()) return;

    savFileName = s;
    savWorkingDir = fi.absolutePath();
    ui->labelSavFile->setText(savFileName);

    ui->textEditSav->setPlainText(readTextFileFromDisk(savFileName));
    ui->pushButtonSaveAsSav->setEnabled(true);

    // if both file names have been set, enable the run button
    if (!parFileName.isEmpty() && !savFileName.isEmpty()) ui->pushButtonRun->setEnabled(true);
}

/*
 * save text of ui->textEditPar to a new file
 */
void LearnProfileDialog::saveAsPar()
{
    QString s = QFileDialog::getSaveFileName(this, tr("Save PAR file"), parWorkingDir, tr("Control file (*.par *.PAR)"));

    // file dialog aborted?
    if (s.isEmpty()) return;

    if (!saveTextFile(s, ui->textEditPar->toPlainText())) {
        ui->textEditOutput->appendPlainText(QString("Warning: Could not write file %1\n").arg(s));
    } else {
        // saving was successful, so set all global variables to the new file name
        QFileInfo fi(s);
        parFileName = fi.absoluteFilePath();
        parWorkingDir = fi.absolutePath();
        ui->labelParFile->setText(parFileName);
    }
}

/*
 * save text of ui->textEditSav to a new file
 */
void LearnProfileDialog::saveAsSav()
{
    QString s = QFileDialog::getSaveFileName(this, tr("Save SAV file"), savWorkingDir, tr("Control file (*.sav *.SAV)"));

    // file dialog aborted?
    if (s.isEmpty()) return;

    savFileName = s;

    if (!saveTextFile(s, ui->textEditSav->toPlainText())) {
        ui->textEditOutput->appendPlainText(QString("Warning: Could not write file %1\n").arg(s));
    } else {
        // saving was successful, so set all global variables to the new file name
        QFileInfo fi(s);
        savFileName = fi.absoluteFilePath();
        savWorkingDir = fi.absolutePath();
        ui->labelSavFile->setText(savFileName);
    }
}

/*
 * unifies multiple peak lines in the par file
 */
QString LearnProfileDialog::unifyPar(const QString &s)
{
    QStringList content = s.split(global::rxLineEnding);
    QMap<double, QStringList> dataContent;
    QStringList newContent;

    ui->textEditOutput->appendPlainText("Scanning PAR file\n*****************\n");

    for (int i = 0; i < content.size(); ++i) {
        ui->textEditOutput->appendPlainText(QString("Scanning line: %1").arg(content.at(i)));

        // skip lines not containing peaks
        if (!content.at(i).contains("GSUM=") && !content.at(i).contains("PHASE=")) continue;

        // extract the position from the line
        QStringList fields = content.at(i).split(QRegularExpression("\\s+"));
        double pos = fields.at(2).toDouble();

        // if the map already contains data at the position, append the line to the stringlist,
        // else create a new entry
        //
        // in other words, if two peaks occur at the same position (here at 7.4245083):
        //
        //   4   1.991656E-02  7.4245083 0.00056098 0.00000031470 ... 7 3 2
        //   4   3.284202E-03  7.4245083 0.00056098 0.00000031470 ... 7 2 3
        //
        // they will be stored in a string list like this:
        //
        //   QMap(double(7.4245083), QStringList("4   1.991656E-02  7.4245083 0.00056098 0.00000031470 ... 7 3 2";
        //                                       "4   3.284202E-03  7.4245083 0.00056098 0.00000031470 ... 7 2 3"))
        //
        // merging will be done in a second iteration

        if (dataContent.contains(pos)) dataContent[pos].append(content.at(i));
        else dataContent[pos] = QStringList(content.at(i));
    }

    ui->textEditOutput->appendPlainText("\n\nUnifying\n********\n");

    // now we loop over the extracted peak lines and merge the ones that were stored in the string lists
    QMap<double, QStringList>::const_iterator i = dataContent.constBegin();
    while (i != dataContent.constEnd()) {
        if (i.value().size() > 1) {
            ui->textEditOutput->appendPlainText(QString("%1 peaks found at %2").arg(i.value().size()).arg(i.key(), 0, 'f', 7));
        }

        if (i.value().size() <= 1) {
            // if only one peak was found at the current position, do not unify
            newContent.append(i.value());
        } else {
            // several peaks at the current position, do unify
            double sumIntens = 0.0;
            double meanB1 = 0.0;
            double meanB2 = 0.0;
            QList<double> intens;
            QList<double> b1l;
            QList<double> b2l;
            int ix = 0;

            // calculated weighed b1 and b2, and summarize the intensities
            for (int n = 0; n < i.value().size(); ++n) {
                // i.value() holds a stringlist with several peak lines
                // i.value().at(n) holds peak line n as a string
                // i.value().at(n).split(" ") splits peak line n into a string list, to access the values
                QStringList cline = i.value().at(n).split(QRegularExpression("\\s+"));

                // range check
                if (cline.size() < 6) continue;

                // read the first integer, intensity, b1 and b2 and store in lists
                ix = cline.at(0).toInt();
                sumIntens += cline.at(1).toDouble();
                intens.append(cline.at(1).toDouble());
                b1l.append(cline.at(3).toDouble());
                b2l.append(cline.at(4).toDouble());
            }

            // unify the values
            for (int n = 0; n < intens.size(); ++n) {
                meanB1 += b1l.at(n) * intens.at(n) / sumIntens;
                meanB2 += b2l.at(n) * intens.at(n) / sumIntens;
            }

            QString nline = QString("%1 %2 %3 %4 %5").arg(ix).arg(sumIntens, 14, 'E', 6).arg(i.key(), 10, 'f', 7).arg(meanB1, 0, 'f', 8).arg(meanB2, 0, 'f', 10);

            for (int n = 0; n < i.value().size(); ++n) {
                QStringList section(i.value().at(n).split(QRegularExpression("\\s+")).mid(5, -1));
                nline.append(" " + section.join(" "));
            }

            newContent.append(nline);
        }

        ++i;
    }

    QString output(content.first() + "\n");
    output.append(newContent.join("\n"));
    output.append("\n" + content.last());

    output.replace(QRegularExpression("PEAKZAHL=\\d+"), QString("PEAKZAHL=%1").arg(newContent.size()));
    return output;
}

/*
 * reads text file "fn" from disk and returns the content as a string
 */
QString LearnProfileDialog::readTextFileFromDisk(const QString &fn)
{
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
             return QString();
    }

    QTextStream in(&f);
    return in.readAll();
}

/*
 * writes string "str" to text file "fn". existing files will be overwritten without warning.
 * check prior to calling this function if this is a problem.
 */
bool LearnProfileDialog::saveTextFile(const QString &fn, const QString &str)
{
    QFile f(fn);

    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << QString("LearnProfileDialog::saveTextFile(): Could not open file for writing: %1").arg(fn);
        return false;
    }

    QTextStream out(&f);
    out << str;
    f.close();
    return true;
}

void LearnProfileDialog::run()
{
    QFileInfo fiMakeGeq(settings->value("bgmnProject/makegeqExec", "").toString());
    QFileInfo fiVerzerr(settings->value("bgmnProject/verzerrExec", "").toString());

    if (!fiMakeGeq.exists() || !fiVerzerr.exists()) {
        QMessageBox::information(this, tr("Process failed"), tr("Running VERZERR failed. Please check your BGMN configuration."));
        return;
    }

    if (savFileName.isEmpty()) {
        QMessageBox::information(this, tr("Process failed"), tr("Control file not found. Please create and save a control file first."));
        return;
    }

    ui->textEditOutput->clear();
    saveTextFile(parFileName, ui->textEditPar->toPlainText());
    saveTextFile(savFileName, ui->textEditSav->toPlainText());

    ui->textEditOutput->appendPlainText(QString("\nRunning Verzerr\n***************\n\n%1 %2\n").arg(fiVerzerr.absoluteFilePath()).arg(savFileName));
    verzerr->init(savFileName, fiVerzerr.absoluteFilePath(), settings->value("config/useWine", false).toBool(), settings->value("config/wineExec", "").toString());
    verzerr->run();
}

void LearnProfileDialog::pollOutput()
{
    if (verzerr->isRunning()) ui->textEditOutput->appendPlainText(verzerr->readOutput().trimmed());
    if (makegeq->isRunning()) ui->textEditOutput->appendPlainText(makegeq->readOutput().trimmed());
}

void LearnProfileDialog::processAborted()
{
    verzerr->abort();
    makegeq->abort();
}

void LearnProfileDialog::verzerrComplete()
{
    QFileInfo fiMakeGeq(settings->value("bgmnProject/makegeqExec", "").toString());
    ui->textEditOutput->appendPlainText(QString("\nRunning MakeGEQ\n***************\n\n%1 %2\n").arg(fiMakeGeq.absoluteFilePath()).arg(savFileName));
    makegeq->init(savFileName, fiMakeGeq.absoluteFilePath(), settings->value("config/useWine", false).toBool(), settings->value("config/wineExec", "").toString());
    makegeq->run();
}

void LearnProfileDialog::makegeqComplete()
{
    ui->textEditOutput->appendPlainText("\n\nComplete");
}
