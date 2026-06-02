/***************************************************************************
                          bgmnpresethandlergui.cpp  -  description
                             -------------------
    begin                : Tue Jul 22 20:30:00 CEST 2025
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

#include "bgmnpresethandlergui.h"
#include "bgmnprojectwidget.h"
#include "presetcontentsavedialog.h"
#include "../libXrdIO/bgmnpresethandler.h"

#include <QMessageBox>

BgmnPresetHandlerGui::BgmnPresetHandlerGui(BgmnProjectWidget *_projectWidget, ControlFileEdit *_editor)
    : QObject(_projectWidget)
{
    project = _projectWidget;
    editor = _editor;

    if (project) connect(this, SIGNAL(sigUpdatePresetMenu()), project, SIGNAL(sigUpdatePresetMenu()));
}

void BgmnPresetHandlerGui::savePreset(const QString &file)
{
    if (!project) return;

    QFileInfo fi(file);
    QString controlFileContent(editor ? editor->toPlainText() : QString());

    bool wrtRef       = false;
    bool wrtIntegrals = false;
    bool wrtCurveFit  = false;
    bool wrtExcelExp  = false;
    bool wrtBaseLines = false;
    bool wrtPeakListFilters = false;

    getModuleDataState(wrtRef, wrtIntegrals, wrtCurveFit, wrtExcelExp, wrtBaseLines, wrtPeakListFilters);

    PresetContentSaveDialog *pcdlg = new PresetContentSaveDialog(project);
    pcdlg->setCheckStatus(wrtRef, wrtIntegrals, wrtCurveFit, wrtExcelExp, wrtBaseLines, wrtPeakListFilters);

    if (pcdlg->exec() == QDialog::Rejected) {
        delete pcdlg;
        return;
    }

    pcdlg->getCheckStatus(wrtRef, wrtIntegrals, wrtCurveFit, wrtExcelExp, wrtBaseLines, wrtPeakListFilters);
    delete pcdlg;

    BgmnPresetHandler phandler;
    QDomDocument doc = phandler.createPresetXmlDocument(file);
    getPresetDocumentXML(doc, wrtIntegrals, wrtCurveFit, wrtExcelExp, wrtBaseLines, wrtPeakListFilters);

    doc = phandler.createPreset(editor ? project->controlFile : QString(),
                                    file,
                                    wrtRef ? controlFileContent : QString(),
                                    doc,
                                    true);

    if (!doc.toString().isEmpty()) {
        emit sigUpdatePresetMenu();
        qDebug() << QString("BgmnPresetHandlerGui::createPreset(): Preset %1 created in %2").arg(fi.fileName(), file);
        project->refOutput->appendPlainText(QString("Preset %1 created in %2").arg(fi.fileName(), file));
        QMessageBox::information(project, tr("Refinement Preset"), QString(tr("Preset %1 created.")).arg(fi.fileName()));
    } else {
        qDebug() << QString("BgmnPresetHandlerGui::createPreset(): Creating preset %1 in %2 failed").arg(fi.fileName(), file);
        project->refOutput->appendPlainText(QString("Creating preset %1 in %2 failed").arg(fi.fileName(), file));
        QMessageBox::warning(project, tr("Refinement Preset"), QString(tr("Preset %1 could not be created.\n"
                                                                       "Please read the log file for further information."))
                                                                .arg(fi.fileName()));
    }
}

/*
 * This creates an xml file similar to the preset files, but for use with ZIP archives.
 * It doesn't copy any files of the refinement project.
 */
QDomDocument BgmnPresetHandlerGui::createBackupXmlFile(const QString &file)
{
    if (!project) return QDomDocument();

    QString controlFileContent(editor ? editor->toPlainText() : QString());

    bool wrtRef       = false;
    bool wrtIntegrals = false;
    bool wrtCurveFit  = false;
    bool wrtExcelExp  = false;
    bool wrtBaseLines = false;
    bool wrtPeakListFilters = false;

    getModuleDataState(wrtRef, wrtIntegrals, wrtCurveFit, wrtExcelExp, wrtBaseLines, wrtPeakListFilters);

    BgmnPresetHandler phandler;
    QDomDocument doc = phandler.createPresetXmlDocument(file);
    getBackupDocumentXML(doc, wrtIntegrals, wrtCurveFit, wrtExcelExp, wrtPeakListFilters);

    QStringList skipFiles;
    for (int i = 0; i < project->graphControl->count(); ++i) {
        QString f = project->graphControl->at(i)->sourceFileName();
        if (!f.isEmpty()) {
            QFileInfo fi(f);
            skipFiles.append(fi.fileName());
        }
    }

    project->graphControl->getPreset(doc, skipFiles);

    doc = phandler.createPreset(editor ? project->controlFile : QString(),
                                file,
                                wrtRef ? controlFileContent : QString(),
                                doc,
                                false);

    return doc;
}

void BgmnPresetHandlerGui::getModuleDataState(bool &wrtRef, bool &wrtIntegrals, bool &wrtCurveFit, bool &wrtExcelExp, bool &wrtBaseLines, bool &wrtPeakListFilters)
{
    wrtRef       = editor                         ? !editor->toPlainText().isEmpty()              : false;
    wrtIntegrals = project->peakIntegrationWidget ? project->peakIntegrationWidget->hasData()     : false;
    wrtCurveFit  = project->peakFitWidget         ? project->peakFitWidget->hasData()             : false;
    wrtExcelExp  = project->excelExporter         ? project->excelExporter->hasData()             : false;
    wrtBaseLines = project->graphControl          ? project->graphControl->baseLines().size() > 0 : false;
    wrtPeakListFilters = project->peakListWidget  ? (!project->peakListWidget->getFilterParameters().isNull()) : false;
}

void BgmnPresetHandlerGui::getPresetDocumentXML(QDomDocument &doc, bool wrtIntegrals, bool wrtCurveFit, bool wrtExcelExp, bool wrtBaseLines, bool wrtPeakListFilters)
{
    if (wrtIntegrals && project->peakIntegrationWidget) project->peakIntegrationWidget->getPreset(doc);
    if (wrtCurveFit  && project->peakFitWidget)         project->peakFitWidget->getPreset(doc);
    if (wrtExcelExp  && project->excelExporter)         project->excelExporter->getPreset(doc);
    if (wrtBaseLines)                                   project->getBaselinePreset(doc);
    if (wrtPeakListFilters && project->peakListWidget)  project->peakListWidget->getPreset(doc);
}

void BgmnPresetHandlerGui::getBackupDocumentXML(QDomDocument &doc, bool wrtIntegrals, bool wrtCurveFit, bool wrtExcelExp, bool wrtPeakListFilters)
{
    if (wrtIntegrals && project->peakIntegrationWidget) project->peakIntegrationWidget->getPreset(doc);
    if (wrtCurveFit  && project->peakFitWidget)         project->peakFitWidget->getPreset(doc);
    if (wrtExcelExp  && project->excelExporter)         project->excelExporter->getPreset(doc);
    if (wrtPeakListFilters && project->peakListWidget)  project->peakListWidget->getPreset(doc);
}

int BgmnPresetHandlerGui::applyPreset(const QString &file, bool overwrite)
{
    if (!project) return -1;

    if (file.isEmpty()) {
        qDebug() << QString("BgmnPresetHandlerGui::applyPreset(): No preset name specified, exiting.");
        return -1;
    } else {
        qDebug() << QString("BgmnPresetHandlerGui::applyPreset(): Applying preset %1 to project %2").arg(file, project->projectBasename);
    }

    int r = 0;
    QDomDocument presetXml("preset");
    presetXml.setContent(BgmnFileIO::readTextFile(file));

    if (presetXml.elementsByTagName("baseline").size()) { // old tag name
        qDebug() << QString("    applying base line preset");
        project->applyPresetBaseLine(presetXml.elementsByTagName("baseline").at(0).toElement());
        ++r;
    } else if (presetXml.elementsByTagName("baseLine").size()) { // new tag name
        qDebug() << QString("    applying base line preset");
        project->applyPresetBaseLine(presetXml.elementsByTagName("baseLine").at(0).toElement());
        ++r;
    }

    if (presetXml.elementsByTagName("peakIntegrals").size()) {
        if (project->peakIntegrationWidget) {
            qDebug() << QString("    applying peak integral preset");
            project->peakIntegrationWidget->applyPreset(presetXml.elementsByTagName("peakIntegrals").at(0).toElement());
            ++r;
        } else {
            qDebug() << QString("    Cannot apply peak integral preset because "
                                "peakIntegrationWidget is not initialized in project %1").arg(project->projectBasename);
        }
    }

    if (presetXml.elementsByTagName("curveFit").size()) {
        if (project->peakFitWidget) {
            qDebug() << QString("    applying peak fit preset");
            project->peakFitWidget->applyPreset(presetXml.elementsByTagName("curveFit").at(0).toElement());
            ++r;
        } else {
            qDebug() << QString("    Cannot apply peak fit preset because "
                                "peakFitWidget is not initialized in project %1").arg(project->projectBasename);
        }
    }

    if (presetXml.elementsByTagName("excelWorkbook").size()) {
        if (project->excelExporter) {
            qDebug() << QString("    applying Excel exporter preset");
            project->excelExporter->applyPreset(presetXml.elementsByTagName("excelWorkbook").at(0).toElement());
            ++r;
        } else {
            qDebug() << QString("    Cannot apply excel export preset because "
                                "excelExporter is not initialized in project %1").arg(project->projectBasename);
        }
    }

    if (presetXml.elementsByTagName("device").size()) {
        BgmnPresetHandler phandler;
        QStringList missingFiles, wrongChecksum;

        bool doApply = phandler.runFileConsistencyChecks(file, presetXml, missingFiles, wrongChecksum);

        if (missingFiles.size() > 0) {
            QMessageBox::warning(project, QString("Problem applying preset"),
                                 QString("The preset cannot be applied because "
                                         "the following files are missing:\n\n%1").arg(missingFiles.join("\n")));
            qDebug() << QString("    Aborted because the following files are missing: %1").arg(missingFiles.join(", "));
        } else if (wrongChecksum.size() > 0) {
            QString warnText = QString("The following files were modified:\n\n%1\n\n"
                                       "Do you want to continue anyway?").arg(wrongChecksum.join("\n"));

            doApply = QMessageBox::warning(project, QString("Checksum warning"),
                                           warnText, QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes ? true : false;
            if (doApply) qDebug() << QString("    The following files have a wrong checksum,"
                                    " but the user decided to continue anyway: %1").arg(wrongChecksum.join(", "));
            else         qDebug() << QString("    The following files have a wrong checksum,"
                                    " the user decided to abort: %1").arg(wrongChecksum.join(", "));
        }

        if (doApply) {
            int nThreads = project->settings->value("bgmnProject/nThreads", 0).toInt();
            QString cFileContent = phandler.createProject(file, project->controlFile, presetXml, project->sampleID, nThreads);

            bool ok = true;

            if (editor) {
                if (!editor->toPlainText().isEmpty()) {
                    if (!overwrite) ok = false;
                }
            } else {
                ok = false;
            }

            if (ok) {
                qDebug() << QString("    applying refinement project preset");
                editor->setPlainText(project->adjustControlOutputFiles(cFileContent, true));
                project->writeFile(project->controlFile, editor);
                ++r;
            } else {
                qDebug() << QString("BgmnPresetHandlerGui::applyPreset(): Could not create a text editor for %1").arg(project->controlFile);
            }
        }
    }

    if (presetXml.elementsByTagName("peakListFilters").size()) {
        if (project->peakListWidget) {
            qDebug() << QString("    applying peak list filter preset");
            project->peakListWidget->applyPreset(presetXml.elementsByTagName("peakListFilters").at(0).toElement());
            ++r;
        } else {
            qDebug() << QString("    Cannot apply peak fit preset because "
                                "peakListWidget is not initialized in project %1").arg(project->projectBasename);
        }
    }

    return r;
}

int BgmnPresetHandlerGui::restoreBackup(const QString &xmlFileName)
{
    if (!project) return -1;

    if (xmlFileName.isEmpty()) {
        qDebug() << QString("BgmnPresetHandlerGui::restoreProjectXmlFile(): No preset name specified, exiting.");
        return -1;
    } else {
        qDebug() << QString("BgmnPresetHandlerGui::restoreProjectXmlFile(): Applying preset %1 to project %2").arg(xmlFileName, project->projectBasename);
    }

    int r = 0;
    QDomDocument presetXml("preset");
    presetXml.setContent(BgmnFileIO::readTextFile(xmlFileName));

    if (presetXml.elementsByTagName("scans").size()) {
        if (project->graphControl) {
            qDebug() << QString("    applying scan parameters");
            project->graphControl->applyPreset(presetXml.elementsByTagName("scans").at(0).toElement(), project->projectDir);
            ++r;
        } else {
            qDebug() << QString("    Cannot apply additional graphs because "
                                "graphControl is not initialized in project %1").arg(project->projectBasename);
        }
    }

    if (presetXml.elementsByTagName("peakIntegrals").size()) {
        if (project->peakIntegrationWidget) {
            qDebug() << QString("    applying peak integral preset");
            project->peakIntegrationWidget->applyPreset(presetXml.elementsByTagName("peakIntegrals").at(0).toElement());
            ++r;
        } else {
            qDebug() << QString("    Cannot apply peak integral preset because "
                                "peakIntegrationWidget is not initialized in project %1").arg(project->projectBasename);
        }
    }

    if (presetXml.elementsByTagName("curveFit").size()) {
        if (project->peakFitWidget) {
            qDebug() << QString("    applying peak fit preset");
            project->peakFitWidget->applyPreset(presetXml.elementsByTagName("curveFit").at(0).toElement());
            ++r;
        } else {
            qDebug() << QString("    Cannot apply peak fit preset because "
                                "peakFitWidget is not initialized in project %1").arg(project->projectBasename);
        }
    }

    if (presetXml.elementsByTagName("excelWorkbook").size()) {
        if (project->excelExporter) {
            qDebug() << QString("    applying Excel exporter preset");
            project->excelExporter->applyPreset(presetXml.elementsByTagName("excelWorkbook").at(0).toElement());
            ++r;
        } else {
            qDebug() << QString("    Cannot apply excel export preset because "
                                "excelExporter is not initialized in project %1").arg(project->projectBasename);
        }
    }

    if (presetXml.elementsByTagName("peakListFilters").size()) {
        if (project->peakListWidget) {
            qDebug() << QString("    applying peak list filter preset");
            project->peakListWidget->applyPreset(presetXml.elementsByTagName("peakListFilters").at(0).toElement());
            ++r;
        } else {
            qDebug() << QString("    Cannot apply peak fit preset because "
                                "peakListWidget is not initialized in project %1").arg(project->projectBasename);
        }
    }

    return r;
}
