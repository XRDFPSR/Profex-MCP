/***************************************************************************
                          controlfileedit.cpp  -  description
                             -------------------
    begin                : Thu Jul 08 08:00:00 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#include "controlfileedit.h"
#include "bgmnbackendconfig.h"

#include <QRegularExpressionMatch>
#include <QTextBlock>
#include <QDebug>
#include <QThread>
#include "../libXrdIO/structs.h"
#include "../libXrdIO/parser/bgmnstrparser.h"

ControlFileEdit::ControlFileEdit(QWidget *parent, const ControlFileEditFileType ft) :
    CodeEditor(parent), fileType(ft)
{
    settings = SettingsManager::getInstance();
    hasChangedText = false;
    maxNThreads = QThread::idealThreadCount();
    zoomLevel = 0;

    BgmnBackendConfig bgConfig;
    lamFiles = bgConfig.getAllLamFiles();

    // actions for the context menu
    actionSetRefinable = new QAction("Refine parameter", this);
    actionSetRefinableIso = new QAction("Refine isotropically", this);
    actionSetRefinableAniso = new QAction("Refine anisotropically", this);
    actionSetRefinableFix = new QAction("Fix parameter", this);
    actionSetRefinableNumbered = new QAction("Refine parameter", this);
    actionOpenTextFile = new QAction("Open file as text", this);
    actionOpenGraphFile = new QAction("Open file as graph", this);
    actionSetParameterNo = new QAction("Set to disabled", this);
    actionSetParameterYes = new QAction("Set to enabled", this);

    actionAddStrucOutFile = new QAction("Add STRUCOUT file", this);
    actionAddSimpleStrucOutFile = new QAction("Add SimpleSTRUCOUT file", this);
    actionAddResFcfOutFile = new QAction("Add RESOUT and FCFOUT file", this);
    actionAddPdbOutFile = new QAction("Add PDBOUT file", this);

    actionRenumberItems = new QAction("Renumber items", this);

    // avoid conflicts with actions of the editor widget using the same shortcuts
    actionSetRefinable->setShortcutContext(Qt::WidgetShortcut);
    actionSetRefinableIso->setShortcutContext(Qt::WidgetShortcut);
    actionSetRefinableAniso->setShortcutContext(Qt::WidgetShortcut);
    actionSetRefinableFix->setShortcutContext(Qt::WidgetShortcut);
    actionSetRefinableNumbered->setShortcutContext(Qt::WidgetShortcut);
    actionOpenTextFile->setShortcutContext(Qt::WidgetShortcut);
    actionOpenGraphFile->setShortcutContext(Qt::WidgetShortcut);
    actionAddStrucOutFile->setShortcutContext(Qt::WidgetShortcut);
    actionAddSimpleStrucOutFile->setShortcutContext(Qt::WidgetShortcut);
    actionAddResFcfOutFile->setShortcutContext(Qt::WidgetShortcut);
    actionAddPdbOutFile->setShortcutContext(Qt::WidgetShortcut);
    actionSetParameterNo->setShortcutContext(Qt::WidgetShortcut);
    actionSetParameterYes->setShortcutContext(Qt::WidgetShortcut);
    actionRenumberItems->setShortcutContext(Qt::WidgetShortcut);

    // set shortcuts for menu actions
    actionSetRefinable->setShortcut(QKeySequence("F6"));
    // the next one can be F5 or F6, will be determined when the menu is called:
    // actionSetRefinableIso->setShortcut(QKeySequence("F"));
    actionSetRefinableAniso->setShortcut(QKeySequence("F6"));
    actionSetRefinableFix->setShortcut(QKeySequence("F5"));
    actionSetRefinableNumbered->setShortcut(QKeySequence("F6"));
    actionSetParameterNo->setShortcut(QKeySequence("F5"));
    actionSetParameterYes->setShortcut(QKeySequence("F6"));

    // action for direct toggling via shortcut
    actionToggleRefineStateDown = new QAction("Decrease value", this);
    actionToggleRefineStateUp = new QAction("Increase value", this);
    actionAllCoordinatesRefined = new QAction("Refine all fractional coordinates", this);
    actionAllCoorindatesFixed = new QAction("Fix all fractional coordinates", this);
    actionAllTdsRefined = new QAction("Refine all TDS", this);
    actionAllTdsFixed = new QAction("Fix all TDS", this);

    actionToggleCommentLine = new QAction("Toggle comment line", this);

    actionSetSubstitution = new QAction("Add substitution", this);
    actionRevertSubstitution = new QAction("Remove substitution", this);

    actionShowHelp = new QAction("Show context help", this);

    // avoid conflicts with actions of the context menu using the same shortcuts
    actionToggleRefineStateDown->setShortcutContext(Qt::WidgetShortcut);
    actionToggleRefineStateUp->setShortcutContext(Qt::WidgetShortcut);

    // set shortcuts for widget actions
    actionToggleRefineStateDown->setShortcut(QKeySequence("F5"));
    actionToggleRefineStateUp->setShortcut(QKeySequence("F6"));
    actionToggleCommentLine->setShortcuts({QKeySequence("F7"), QKeySequence("Ctrl+/")});
    actionAllCoordinatesRefined->setShortcut(QKeySequence("Ctrl+Shift+F6"));
    actionAllCoorindatesFixed->setShortcut(QKeySequence("Ctrl+Shift+F5"));
    actionAllTdsRefined->setShortcut(QKeySequence("Ctrl+Shift+F8"));
    actionAllTdsFixed->setShortcut(QKeySequence("Ctrl+Shift+F7"));

    addAction(actionToggleRefineStateDown);
    addAction(actionToggleRefineStateUp);
    addAction(actionToggleCommentLine);
    addAction(actionAllCoordinatesRefined);
    addAction(actionAllCoorindatesFixed);
    addAction(actionAllTdsRefined);
    addAction(actionAllTdsFixed);

    // connections
    connect(actionSetRefinable, SIGNAL(triggered()), this, SLOT(setRefinableIso()));
    connect(actionSetRefinableIso, SIGNAL(triggered()), this, SLOT(setRefinableIso()));
    connect(actionSetRefinableAniso, SIGNAL(triggered()), this, SLOT(setRefinableAniso()));
    connect(actionSetRefinableFix, SIGNAL(triggered()), this, SLOT(setRefinableFix()));
    connect(actionSetRefinableNumbered, SIGNAL(triggered()), this, SLOT(setRefinableNumbered()));
    connect(actionOpenTextFile, SIGNAL(triggered()), this, SLOT(openTextFile()));
    connect(actionOpenGraphFile, SIGNAL(triggered()), this, SLOT(openGraphFile()));

    connect(actionAddStrucOutFile, SIGNAL(triggered()), this, SLOT(addStrucOutFile()));
    connect(actionAddSimpleStrucOutFile, SIGNAL(triggered()), this, SLOT(addSimpleStrucOutFile()));
    connect(actionAddResFcfOutFile, SIGNAL(triggered()), this, SLOT(addResFcfOutFile()));
    connect(actionAddPdbOutFile, SIGNAL(triggered()), this, SLOT(addPdbOutFile()));

    connect(actionToggleRefineStateDown, SIGNAL(triggered()), this, SLOT(toggleRefineStateDown()));
    connect(actionToggleRefineStateUp, SIGNAL(triggered()), this, SLOT(toggleRefineStateUp()));
    connect(actionAllCoordinatesRefined, SIGNAL(triggered(bool)), this, SLOT(setAllCoordinatesRefined()));
    connect(actionAllCoorindatesFixed, SIGNAL(triggered(bool)), this, SLOT(setAllCoordinatesFixed()));
    connect(actionAllTdsRefined, SIGNAL(triggered(bool)), this, SLOT(setAllTdsRefined()));
    connect(actionAllTdsFixed, SIGNAL(triggered(bool)), this, SLOT(setAllTdsFixed()));

    connect(actionToggleCommentLine, SIGNAL(triggered(bool)), this, SLOT(toggleCommentLine()));
    connect(actionRenumberItems, SIGNAL(triggered(bool)), this, SLOT(renumberItems()));

    connect(actionSetParameterYes, SIGNAL(triggered(bool)), this, SLOT(toggleParamYes()));
    connect(actionSetParameterNo, SIGNAL(triggered(bool)), this, SLOT(toggleParamNo()));

    connect(actionSetSubstitution, SIGNAL(triggered(bool)), this, SLOT(setSubstitution()));
    connect(actionRevertSubstitution, SIGNAL(triggered(bool)), this, SLOT(revertSubstitution()));

    connect(actionShowHelp, SIGNAL(triggered(bool)), this, SLOT(showHelp()));

    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(broadcastCurrentWord()));

    paramStart = 0;

    paramsFix.append("A");
    paramsFix.append("B");
    paramsFix.append("C");
    paramsFix.append("ALPHA");
    paramsFix.append("BETA");
    paramsFix.append("GAMMA");
    paramsFix.append("B1");
    paramsFix.append("B2");
    paramsFix.append("k1");
    paramsFix.append("k2");
    paramsFix.append("TDS");
    paramsFix.append("x");
    paramsFix.append("y");
    paramsFix.append("z");
    paramsFix.append("EPS1");
    paramsFix.append("EPS2");
    paramsFix.append("EPS3");
    paramsFix.append("EPS4");

    paramsOnlyIso.append("A");
    paramsOnlyIso.append("B");
    paramsOnlyIso.append("C");
    paramsOnlyIso.append("ALPHA");
    paramsOnlyIso.append("BETA");
    paramsOnlyIso.append("GAMMA");
    paramsOnlyIso.append("k1");
    paramsOnlyIso.append("x");
    paramsOnlyIso.append("y");
    paramsOnlyIso.append("z");
    paramsOnlyIso.append("B2");

    paramsIso.append("B1");
    paramsIso.append("k2");
    paramsIso.append("TDS");

    paramsAniso.append("B1");
    paramsAniso.append("k2");
    paramsAniso.append("TDS");

    paramsYesNo.append("PROTOKOLL");
    paramsYesNo.append("SAVE");
    paramsYesNo.append("DDM");
    paramsYesNo.append("ONLYISO");
    paramsYesNo.append("GSUM");

    paramsNumbered.append("EPS1");
    paramsNumbered.append("EPS2");
    paramsNumbered.append("EPS3");
    paramsNumbered.append("EPS4");

    paramsIntegerValue.append("WMIN");
    paramsIntegerValue.append("WMAX");
    paramsIntegerValue.append("RU");
    paramsIntegerValue.append("RP");
    paramsIntegerValue.append("NTHREADS");
    paramsIntegerValue.append("LeBail");
    paramsIntegerValue.append("GEWICHT");

    filesText.append("STRUC");
    filesText.append("LIST");
    filesText.append("OUTPUT");
    filesText.append("STRUCOUT");
    filesText.append("SimpleSTRUCOUT");
    filesText.append("PDBOUT");
    filesText.append("RESOUT");
    filesText.append("FCFOUT");

    filesGraph.append("VAL");
    filesGraph.append("DIAGRAMM");
    filesGraph.append("UNT");
    filesGraph.append("UNTC");

    itemsNumbered.append("PARAM");
    itemsNumbered.append("STRUC");
    itemsNumbered.append("STRUCOUT");
    itemsNumbered.append("SimpleSTRUCOUT");
    itemsNumbered.append("PDBOUT");
    itemsNumbered.append("RESOUT");
    itemsNumbered.append("FCFOUT");
    itemsNumbered.append("VAL");
    itemsNumbered.append("CUT");
    itemsNumbered.append("GOAL");

    // composing some global RegExp patterns for convenience
    paramFilesPattern  = QString("%1|%2").arg(filesText.join("|"), filesGraph.join("|"));
    paramNamesPattern  = QString("%1|%2|%3").arg(paramsFix.join("|"), paramsNumbered.join("|"), paramsIntegerValue.join("|"));
    paramValuesPattern = QString("%1|ANISO4|ANISOLIN|ANISOSQR|ANISO|SPHAR0|SPHAR2|SPHAR4|SPHAR6|SPHAR8|SPHAR10").arg(global::rxDoublePattern);
    paramYesNoPattern  = QString("%1").arg(paramsYesNo.join("|"));

    // RegExp matching a keyword and file name. File names can contain white spaces. The match is termiated by //, %, \n, or $
    // capture group 1: keyword without the number
    // capture group 2: file name
    rxFilesBlock.setPattern(QString("^(%1)(?:\\[\\d+\\])?=(.+?)\\s*(?=\\/\\/|%|\\n|$)").arg(paramFilesPattern));

    // RegExp matching a parameter block such as x=1.234 or PARAM[1]=EPS1=0_-1^1 or anything in between
    // capture group 1: prefix (PARAM, PARAM[n], or QString())
    // capture group 2: keyword
    // capture group 3: value
    // capture group 4: lower limit (or QString())
    // capture group 5: upper limit (or QString())
    rxParamBlock.setPattern(QString("^(PARAM\\[?\\d*\\]?=)?((?:%1)\\[?\\d*\\]?)=(%2)(?:_(%3))?(?:\\^(%3))?").arg(paramNamesPattern, paramValuesPattern, global::rxDoublePattern));

    // RegExp matching a E= line
    // capture group 1: keyword
    rxElementBlock.setPattern(QString("^(E)="));

    // RegExp matching yes/no parameters
    // capture group 1: keyword
    // capture group 2: value
    rxYesNoBlock.setPattern(QString("^(%1)=([yYjJnN])").arg(paramYesNoPattern));

    // RegExp matching the LAMBDA block
    rxLambdaBlock.setPattern("^(LAMBDA)=(\\S+)");

    // RegExp matching the numbered GOALs. The match is terminated by \n or $
    // capture group 1: keyword without the number
    // capture group 2: value
    rxGoalsBlock.setPattern("^(GOAL)\\[\\d+\\]=(.+)(?=\\n|$)");
}

void ControlFileEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();

    if (parseParam()) {
        menu->addSeparator();
        int state = currentRefinementState();

        // add actions if the parameters are refinable
        if (paramsFix.contains(param)) {
            if (state > 0) menu->addAction(actionSetRefinableFix);
        }

        if (paramsOnlyIso.contains(param)) {
            if (state == 0) menu->addAction(actionSetRefinable);
        }

        if (paramsIso.contains(param)) {
            if (state == 0) {
                actionSetRefinableIso->setShortcut(QKeySequence("F6"));
                menu->addAction(actionSetRefinableIso);
            } else if (state == 3) {
                actionSetRefinableIso->setShortcut(QKeySequence("F5"));
                menu->addAction(actionSetRefinableIso);
            }
        }

        if (paramsAniso.contains(param)) {
            if (state < 3) menu->addAction(actionSetRefinableAniso);
        }

        if (paramsNumbered.contains(param)) {
            if (state == 0) menu->addAction(actionSetRefinableNumbered);
        }

        if (filesText.contains(param)) {
            menu->addAction(actionOpenTextFile);
        }

        if (filesGraph.contains(param)) {
            menu->addAction(actionOpenTextFile);
            menu->addAction(actionOpenGraphFile);
        }

        if (param == "STRUC") {
            menu->addSeparator();
            menu->addAction(actionAddStrucOutFile);
            menu->addAction(actionAddSimpleStrucOutFile);
            menu->addAction(actionAddResFcfOutFile);
            menu->addAction(actionAddPdbOutFile);
        }

        if (param == "LAMBDA") {
            menu->addSeparator();

            QMultiMapIterator<QString, QString> it(lamFiles);

            while (it.hasNext()) {
                it.next();
                menu->addAction(it.key(), this, SLOT(setLambda()));
            }
        }

        if (paramsYesNo.contains(param)) {
            menu->addSeparator();
            if (isYes()) {
                menu->addAction(actionSetParameterNo);
            } else {
                menu->addAction(actionSetParameterYes);
            }
        }

        if (paramsIntegerValue.contains(param)) {
            menu->addSeparator();
            if (!isIntegerAtLowerLimit()) menu->addAction(actionToggleRefineStateDown);
            if (!isIntegerAtUpperLimit()) menu->addAction(actionToggleRefineStateUp);
        }

        qDebug() << param;
        if (itemsNumbered.contains(param)) {
            menu->addSeparator();
            menu->addAction(actionRenumberItems);
        }

        if (param == "E") {
            if (isSubstitution()) {
                menu->addAction(actionRevertSubstitution);
            } else {
                menu->addAction(actionSetSubstitution);
            }
        }
    }

    menu->addSeparator();
    menu->addAction(actionToggleCommentLine);

    if (fileType == BGMN_STR) {
        menu->addSeparator();
        menu->addAction(actionAllCoordinatesRefined);
        menu->addAction(actionAllCoorindatesFixed);
        menu->addSeparator();
        menu->addAction(actionAllTdsRefined);
        menu->addAction(actionAllTdsFixed);
    }

    menu->addSeparator();
    menu->addAction(actionShowHelp);

    menu->exec(event->globalPos());
    delete menu;
}

void ControlFileEdit::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() == Qt::ControlModifier) {
        if (e->angleDelta().y() > 0) {
            zoomIn(1);
            ++zoomLevel;
        } else {
            zoomOut(1);
            --zoomLevel;
        }
    } else {
        QPlainTextEdit::wheelEvent(e);
    }
}

void ControlFileEdit::resetZoom()
{
    if (zoomLevel > 0) {
        zoomOut(zoomLevel);
    } else if (zoomLevel < 0) {
        zoomIn(-zoomLevel);
    }

    zoomLevel = 0;
}

void ControlFileEdit::toggleRefineStateDown()
{
    if (!parseParam()) return;

    if (paramsIntegerValue.contains(param)) {
        if (param == "GEWICHT") {
            static QRegularExpression rx("[\\d\\._^-]+|SPHAR(\\d+)");
            QRegularExpressionMatch rm = rx.match(value);

            if (rm.hasMatch()) {
                int n = 0;

                if (rm.hasCaptured(1)) {
                    n = rm.captured(1).toInt();
                }

                n = n < 2 ? 0 : n - 2;

                replaceCurrentParameter(QString("%1=SPHAR%2").arg(param).arg(n));
            }

            return;
        } else {
            int n = value.toInt();

            if (param == "RU") n = n <= 1 ? 1 : n - 1;
            if (param == "RP") n = n <= 2 ? 2 : n - 1;
            if (param == "WMIN") n = n <= 0 ? 0 : n - 1;
            if (param == "WMAX") n = n <= 0 ? 0 : n - 1;
            if (param == "NTHREADS") n = n <= 1 ? 1 : n - 1;
            if (param == "LeBail") n = 0;

            replaceCurrentParameter(QString("%1=%2").arg(param).arg(n));
            return;
        }
    }

    if (paramsYesNo.contains(param)) {
        replaceCurrentParameter(QString("%1=N").arg(param));
        return;
    }

    int i = currentRefinementState();

    if (i > 1) {
        if (paramsIso.contains(param) || paramsOnlyIso.contains(param)) setRefinableIso();
        return;
    }

    if (i == 1) {
        if (paramsFix.contains(param)) setRefinableFix();
        return;
    }
}

void ControlFileEdit::toggleRefineStateUp()
{
    if (!parseParam()) return;

    if (paramsIntegerValue.contains(param)) {
        if (param == "GEWICHT") {
            static QRegularExpression rx("[\\d\\._^-]+|SPHAR(\\d+)");
            QRegularExpressionMatch rm = rx.match(value);

            if (rm.hasMatch()) {
                int n = 0;

                if (rm.hasCaptured(1)) {
                    n = rm.captured(1).toInt();
                }

                n = n > 8 ? 10 : n + 2;

                replaceCurrentParameter(QString("%1=SPHAR%2").arg(param).arg(n));
            }

            return;
        } else {
            int n = value.toInt();

            if (param == "RU") ++n;
            if (param == "RP") n = n >= 4 ? 4 : n + 1;
            if (param == "WMIN") ++n;
            if (param == "WMAX") ++n;
            if (param == "NTHREADS") n = n >= maxNThreads ? maxNThreads : n + 1;
            if (param == "LeBail") n = 1;

            if (param == "RP") {
                replaceCurrentParameter(addRpConditionalParameters(n));
            } else {
                replaceCurrentParameter(QString("%1=%2").arg(param).arg(n));
            }
            return;
        }
    }

    if (paramsYesNo.contains(param)) {
        replaceCurrentParameter(QString("%1=Y").arg(param));
        return;
    }

    int i = currentRefinementState();

    if (i == 0) {
        if (paramsOnlyIso.contains(param))  setRefinableIso();
        if (paramsIso.contains(param))      setRefinableIso();
        if (paramsNumbered.contains(param)) setRefinableNumbered();
        return;
    }

    if (i < 3) {
        if (paramsAniso.contains(param)) setRefinableAniso();
        return;
    }
}

QString ControlFileEdit::addRpConditionalParameters(int n)
{
    QString out = QString("RP=%1").arg(n);

    if (n == 2) return out;

    static QRegularExpression rxK1("(?:\\s|PARAM=)?k1=\\d+");
    static QRegularExpression rxK2("(?:\\s|PARAM=)?k2=(?:ANISO|\\d+)");
    static QRegularExpression rxB1("(?:\\s|PARAM=)?B1=(?:ANISO|\\d+)");

    QRegularExpressionMatch rm = rxK1.match(toPlainText());
    bool hasK1 = rm.hasMatch();

    rm = rxK2.match(toPlainText());
    bool hasK2 = rm.hasMatch();

    rm = rxB1.match(toPlainText());
    bool hasB1 = rm.hasMatch();

    if (n == 3) {
        if (!hasB1) out += QString(" PARAM=B1=0_0^0.01");
        return out;
    }

    if (!hasK1) out += QString(" k1=0");
    if (!hasK2) out += QString(" k2=0");
    if (!hasB1) out += QString(" PARAM=B1=0_0^0.01");
    return out;
}

int ControlFileEdit::currentRefinementState()
{
    if (value.left(5) == "ANISO")  return 3; // captures all ANISOxxx

    // these need special treatment, just return 2
    if (value.left(5) == "SPHAR")  return 2;
    if (param == "NTHREADS")       return 2;
    if (param == "PROTOKOLL")      return 2;
    if (param == "SAVE")           return 2;
    if (param == "DDM")            return 2;

    if (prefix.left(5) == "PARAM") {
        if (upper.isEmpty()) return 2; // isotropic but without limits
        else                 return 1; // all normal isotropic ones
    }

    return 0;
}

bool ControlFileEdit::isIntegerAtLowerLimit()
{
    int v = value.toInt();

    if (param == "WMIN")     return v == 0;
    if (param == "WMAX")     return v == 0;
    if (param == "RU")       return v == 1;
    if (param == "RP")       return v == 2;
    if (param == "NTHREADS") return v == 1;
    if (param == "LeBail")   return v == 0;
    if (param == "GEWICHT")  return value == "SPHAR0";

    return false;
}

bool ControlFileEdit::isIntegerAtUpperLimit()
{
    int v = value.toInt();

    if (param == "WMIN")     return false;
    if (param == "WMAX")     return false;
    if (param == "RU")       return false;
    if (param == "RP")       return v == 4;
    if (param == "NTHREADS") return v == maxNThreads;
    if (param == "LeBail")   return v == 1;
    if (param == "GEWICHT")  return value == "SPHAR10";

    return false;
}

void ControlFileEdit::toggleParamYes()
{
    replaceCurrentParameter(QString("%1=Y").arg(param));
    resetParamVariables();
}

void ControlFileEdit::toggleParamNo()
{
    replaceCurrentParameter(QString("%1=N").arg(param));
    resetParamVariables();
}

bool ControlFileEdit::parseParam()
{
    // clear old content
    resetParamVariables();

    QTextCursor cursor = textCursor();
    selectedWord = blockUnderSelection();

    // parse the word under the cursor or within the selection
    if (cursor.hasSelection()) {
        splitSelectionUnderCursor(selectedWord, param, value);
    } else {
        splitParameterBlock(selectedWord, prefix, param, value, lower, upper);
    }

    // cut off any "[d]=" at the end of param
    static QRegularExpression rxParam("([^\\[\\]=]+)(?:\\[\\d\\])?=?");
    QRegularExpressionMatch rmParam = rxParam.match(param);

    if (!rmParam.hasMatch()) return false;

    param = rmParam.captured(1).trimmed();
    return true;
}

QString ControlFileEdit::lineUnderCursor()
{
    int start = currentLineStart();
    int end   = currentLineEnd();

    return toPlainText().mid(start, end - start);
}

int ControlFileEdit::currentLineStart()
{
    static QRegularExpression rx("^|\\n");
    int start = toPlainText().left(textCursor().selectionStart()).lastIndexOf(rx);
    start = start < 0 ? 0 : start + 1;
    return start;
}

int ControlFileEdit::currentLineEnd()
{
    static QRegularExpression rx("\\n|$");
    int end = toPlainText().indexOf(rx, textCursor().selectionEnd());
    return end;
}

QString ControlFileEdit::blockUnderSelection()
{
    static QRegularExpression delimStart("^|\\s");

    // find the last space before the selection
    int start = toPlainText().left(textCursor().selectionStart()).lastIndexOf(delimStart);
    start = start < 0 ? 0 : start + 1;

    QString txt = toPlainText().mid(start);
    QRegularExpressionMatch rm;

    rm = rxFilesBlock.match(txt);
    if (!rm.hasMatch()) rm = rxParamBlock.match(txt);
    if (!rm.hasMatch()) rm = rxElementBlock.match(txt);
    if (!rm.hasMatch()) rm = rxYesNoBlock.match(txt);
    if (!rm.hasMatch()) rm = rxLambdaBlock.match(txt);
    if (!rm.hasMatch()) rm = rxGoalsBlock.match(txt);
    if (!rm.hasMatch()) return QString();

    paramStart = start + rm.capturedStart();
    return rm.captured(0);
}

QString ControlFileEdit::wordUnderSelection()
{
    if (textCursor().selectionStart() != textCursor().selectionEnd()) {
        // we ignore actual text selections. The word is only determined
        // if the cursor is placed on the word without selecting parts or all of the word.
        return QString();
    }

    static QRegularExpression delim("\\W");

    // find the first word delimiter left and right of the current cursor position
    int start = toPlainText().left(textCursor().selectionStart()).lastIndexOf(delim);
    int end = toPlainText().indexOf(delim, textCursor().selectionEnd());

    // if a "\\W" was found, we have to cut it off
    paramStart = start < 0 ? 0 : start + 1;

    if (end < 0) return QString();

    return toPlainText().mid(paramStart, end - paramStart);
}

bool ControlFileEdit::splitParameterBlock(const QString &w, QString &pref, QString &par, QString &val, QString &low, QString &up)
{
    QRegularExpressionMatch match;

    // check if it is a file name
    match = rxFilesBlock.match(w);
    if (match.hasMatch()) {
        pref = QString();
        par  = match.captured(1);
        val  = match.captured(2);
        low  = QString();
        up   = QString();
        return true;
    }

    // check if it is a parameter
    match = rxParamBlock.match(w);
    if (match.hasMatch()) {
        pref = match.captured(1);
        par  = match.captured(2);
        val  = match.captured(3);
        low  = match.captured(4);
        up   = match.captured(5);
        return true;
    }

    // check if it is a E= line
    match = rxElementBlock.match(w);
    if (match.hasMatch()) {
        pref = QString();
        par  = match.captured(1);
        val  = QString();
        low  = QString();
        up   = QString();
        return true;
    }

    // check if it is a yes/no parameter
    match = rxYesNoBlock.match(w);
    if (match.hasMatch()) {
        pref = QString();
        par  = match.captured(1);
        val  = match.captured(2);
        low  = QString();
        up   = QString();
        return true;
    }

    // check if it is a LAMBDA line
    match = rxLambdaBlock.match(w);
    if (match.hasMatch()) {
        pref = QString();
        par  = match.captured(1);
        val  = match.captured(2);
        low  = QString();
        up   = QString();
        return true;
    }

    // check if it is a numbered GOAL line
    match = rxGoalsBlock.match(w);
    if (match.hasMatch()) {
        pref = QString();
        par  = match.captured(1);
        val  = match.captured(2);
        low  = QString();
        up   = QString();
        return true;
    }

    return false;
}

bool ControlFileEdit::splitSelectionUnderCursor(const QString &w, QString &par, QString &val)
{
    static QRegularExpression rx("([a-zA-Z\\d]+)(?:\\[\\d+\\])?=(.*)\\s*(?=\\/\\/|$)");
    QRegularExpressionMatch match = rx.match(w);

    if (!match.hasMatch()) {
        qDebug() << QStringLiteral("ControlFileEdit::splitSelectionUnderCursor(): Could not parse word %1").arg(w);
        return false;
    }

    par  = match.captured(1);
    val  = match.captured(2);

    return true;
}

void ControlFileEdit::broadcastCurrentWord()
{
    QStringList lst(absFileName);
    QString word = wordUnderSelection();

    if (absFileName.right(3).toLower() == "sav") {
        if (word == "GOAL") word = QString("GOAL[i]");
    }

    lst << word;
    emit wordUnderCursor(lst);
}

int ControlFileEdit::fileNumber(const QString &selection)
{
    static QRegularExpression rx("[A-Za-z]+\\[(\\d+)\\]=?");
    QRegularExpressionMatch match = rx.match(selection);

    if (match.hasMatch()) {
        return match.captured(1).toInt();
    }

    return -1;
}

QString ControlFileEdit::fileBaseName(const QString &selection)
{
    static QRegularExpression rx("[A-Za-z]+\\[\\d+\\]=(.+)\\.(?:str|STR)");
    QRegularExpressionMatch match = rx.match(selection);

    if (match.hasMatch()) {
        return match.captured(1);
    }

    return QString();
}

void ControlFileEdit::setRefinableIso()
{
    double limits = settings->value("cifimport/limits", 0.01).toDouble();

    // Store the value in a double. We may need it as a double value to calculate
    // upper and lower limits relative to the value.
    bool ok;
    double dvalue = value.toDouble(&ok);

    if (!ok) {
        dvalue = 0.0;
    }

    // construct the new parameter string
    QString ostring;

    // reduce NTHREADS one step
    if (param.toUpper() == "NTHREADS") {
        int n = value.toInt();
        if (n > 1) ostring = QString("NTHREADS=%1").arg(n - 1);
        else       ostring = QStringLiteral("NTHREADS=1");
    }

    // toggle Protokoll, Save, DDM
    if (param.toUpper() == "PROTOKOLL") {
        if ((value.toUpper() == "Y") || value.toUpper() == "J") ostring = QStringLiteral("PROTOKOLL=N");
        else return;
    }

    if (param.toUpper() == "SAVE") {
        if ((value.toUpper() == "Y") || value.toUpper() == "J") ostring = QStringLiteral("SAVE=N");
        else return;
    }

    if (param.toUpper() == "DDM") {
        if ((value.toUpper() == "Y") || value.toUpper() == "J") ostring = QStringLiteral("DDM=N");
        else return;
    }

    // unit cell dimensions, upper and lower limit of limits*dvalue
    if (param == "A" || param == "B" || param == "C" ||
        param == "ALPHA" || param == "BETA" || param == "GAMMA") {
        ostring = QString("PARAM=%1=%2_%3^%4").arg(param, value).arg(dvalue - dvalue * limits).arg(dvalue + dvalue * limits);
    }

    // k1
    if (param == "k1") {
        ostring = QString("PARAM=k1=%1_0^1").arg(value);
    }

    // k2
    if (param == "k2") {
        if (upper.isEmpty()) upper = QStringLiteral("0.0001");
        ostring = QString("PARAM=k2=%1_0^%2").arg(dvalue).arg(upper);
    }

    // B1
    if (param == "B1") {
        if (upper.isEmpty()) upper = QStringLiteral("0.01");
        ostring = QString("PARAM=B1=%1_0^%2").arg(dvalue).arg(upper);
    }

    // B2
    if (param == "B2") {
        if (upper.isEmpty()) upper = QStringLiteral("0.02");
        ostring = QString("PARAM=B2=%1_0^%2").arg(dvalue).arg(upper);
    }

    // tds, lower limit = 0, upper limit = 2*tds
    if (param == "TDS") {
        if (upper.isEmpty()) {
            // fallback value
            upper = QStringLiteral("0.02");
            // fallback 2, in case the value is greater than the fallback upper limit
            if (dvalue > 0.02) upper = QString("%1").arg(2.0 * dvalue, 0, 'f', 6);
        }
        ostring = QString("PARAM=TDS=%1_0^%2").arg(dvalue, 0, 'f', 6).arg(upper);
    }

    // fractional coordinates, upper and lower limit of delta
    if (param == "x" || param == "y" || param == "z") {
        double fcoordLimit = settings->value("bgmnProject/coordinatelimits", 0.05).toDouble();
        ostring = QString("PARAM=%1=%2_%3^%4").arg(param, value).arg(dvalue - fcoordLimit, 0, 'f', 4).arg(dvalue + fcoordLimit, 0, 'f', 4);
    }

    replaceCurrentParameter(ostring);
    resetParamVariables();
}

void ControlFileEdit::setRefinableAniso()
{
    // construct the new parameter string
    QString ostring;

    // increase NTHREADS one step
    if (param.toUpper() == "NTHREADS") {
        int n = value.toInt();
        if (n < maxNThreads) ostring = QString("NTHREADS=%1").arg(n + 1);
        else                 ostring = QString("NTHREADS=%1").arg(maxNThreads);
    }

    // toggle Protokoll, Save, DDM
    if (param.toUpper() == "PROTOKOLL") {
        if (value.toUpper() == "N") ostring = QStringLiteral("PROTOKOLL=Y");
        else return;
    }

    if (param.toUpper() == "SAVE") {
        if (value.toUpper() == "N") ostring = QStringLiteral("SAVE=Y");
        else return;
    }

    if (param.toUpper() == "DDM") {
        if (value.toUpper() == "N") ostring = QStringLiteral("DDM=Y");
        else return;
    }

    // k2
    if (param == "k2") {
        if (upper.isEmpty()) {
            ostring = QStringLiteral("k2=ANISO4^0.0001");
        } else {
            ostring = QString("k2=ANISO4^%1").arg(upper);
        }
    }

    // B1
    if (param == "B1") {
        if (upper.isEmpty()) {
            ostring = QStringLiteral("B1=ANISO^0.01");
        } else {
            ostring = QString("B1=ANISO^%1").arg(upper);
        }
    }

    // TDS
    if (param == "TDS") {
        if (upper.isEmpty()) {
            ostring = QStringLiteral("TDS=ANISO^0.02");
        } else {
            ostring = QString("TDS=ANISO^%1").arg(upper);
        }
    }

    // fractional coordinates, not really anisotropic, but refined without limits
    if (param == "x" || param == "y" || param == "z") {
        ostring = QString("PARAM=%1=%2").arg(param, value);
    }

    // unit cell, not really anisotropic, but refined without limits
    if (param == "A" || param == "B" || param == "C"
            || param == "ALPHA" || param == "BETA" || param == "GAMMA") {
        ostring = QString("PARAM=%1=%2").arg(param, value);
    }

    replaceCurrentParameter(ostring);
    resetParamVariables();
}

void ControlFileEdit::setRefinableFix()
{
    // Store the value in a double. If value is something like ANISO, the conversion
    // will fail and we will fall back to 0.0, which is the intended behaviour
    bool ok;
    double dvalue = value.toDouble(&ok);

    if (!ok) {
        dvalue = 0.0;
    }

    // construct the new parameter string
    QString ostring;

    // special replacements

    // GEWICHT hard coded to SPHAR0
    if (param == "GEWICHT") {
        ostring = QStringLiteral("GEWICHT=0.1");
    }

    // unit cell dimensions, upper and lower limit of 0.01*dvalue
    if (param == "A" || param == "B" || param == "C" ||
        param == "ALPHA" || param == "BETA" || param == "GAMMA") {
        ostring = QString("%1=%2").arg(param, value);
    }

    // k1
    if (param == "k1") {
        ostring = QString("k1=%1").arg(value);
    }

    // k2
    if (param == "k2") {
        ostring = QString("k2=%1").arg(dvalue);
    }

    // B1
    if (param == "B1") {
        ostring = QString("B1=%1").arg(dvalue);
    }

    // B2
    if (param == "B2") {
        ostring = QString("B2=%1").arg(dvalue);
    }

    // TDS
    if (param == "TDS") {
        ostring = QString("TDS=%1").arg(dvalue);
    }

    // fractional coordinates
    if (param == "x" || param == "y" || param == "z") {
        ostring = QString("%1=%2").arg(param, value);
    }

    // EPS values
    if (param.left(3) == "EPS") {
        ostring = QString("%1=%2").arg(param, value);
    }

    replaceCurrentParameter(ostring);

    if (paramsNumbered.contains(param)) {
        fixParameterNumbering("PARAM");
    }

    resetParamVariables();
}

void ControlFileEdit::setRefinableNumbered()
{
    // construct the new parameter string
    QString ostring;

    int n = freeParamNumber();

    // EPS values
    if (param.left(3) == "EPS") {
        ostring = QString("PARAM[%1]=%2=%3_-0.01^0.01").arg(n).arg(param, value);
    }

    replaceCurrentParameter(ostring);
    fixParameterNumbering("PARAM");
    resetParamVariables();
}

void ControlFileEdit::replaceCurrentParameter(const QString &s)
{
    // select and clear the text
    QTextCursor cursor = textCursor();
    cursor.setPosition(paramStart);
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, selectedWord.length());
    cursor.removeSelectedText();

    // write back the string
    cursor.insertText(s);
    cursor.setPosition(paramStart);
    setTextCursor(cursor);
}

void ControlFileEdit::openTextFile()
{
    emit openText(value.trimmed());
}

void ControlFileEdit::openGraphFile()
{
    emit openGraph(value.trimmed());
}

void ControlFileEdit::addStrucOutFile()
{
    QString line = blockUnderSelection();
    int n = fileNumber(line);
    QString bn = fileBaseName(line);

    if ((n >= 0) && !bn.isNull()) {
        QString newLine = QString("STRUCOUT[%1]=%2-%3.str").arg(n).arg(bn, editorBaseName);
        insertLine(line, newLine, false);
    }
}

void ControlFileEdit::addSimpleStrucOutFile()
{
    QString line = blockUnderSelection();
    int n = fileNumber(line);
    QString bn = fileBaseName(line);

    if ((n >= 0) && !bn.isNull()) {
        QString newLine = QString("SimpleSTRUCOUT[%1]=%2-simple-%3.str").arg(n).arg(bn, editorBaseName);
        insertLine(line, newLine, false);
    }
}

void ControlFileEdit::addResFcfOutFile()
{
    QString line = blockUnderSelection();
    int n = fileNumber(line);
    QString bn = fileBaseName(line);

    if ((n >= 0) && !bn.isNull()) {
        QString newResLine = QString("RESOUT[%1]=%2-%3.res").arg(n).arg(bn, editorBaseName);
        QString newFcfLine = QString("FCFOUT[%1]=%2-%3.fcf").arg(n).arg(bn, editorBaseName);

        insertLine(line, newResLine, false);
        insertLine(line, newFcfLine, false);
    }
}

void ControlFileEdit::addPdbOutFile()
{
    QString line = blockUnderSelection();
    int n = fileNumber(line);
    QString bn = fileBaseName(line);

    if ((n >= 0) && !bn.isNull()) {
        QString newLine = QString("PDBOUT[%1]=%2-%3.pdb").arg(n).arg(bn, editorBaseName);
        insertLine(line, newLine, false);
    }
}

void ControlFileEdit::insertLine(const QString &before, const QString &newLine, bool allowDuplicates)
{
    QString str = toPlainText();

    if (str.contains(newLine.trimmed())) {
        if (!allowDuplicates) {
            qDebug() << QString("ControlFileEdit::insertLine(): Line is already present.").arg(newLine);
            return;
        }
    }

    QStringList content = str.split(global::rxLineEnding);

    for (int i = 0; i < content.size(); ++i) {
        if (content.at(i).contains(before.trimmed())) {
            content.insert(i+1, newLine.trimmed());
            break;
        }
    }

    setPlainText(content.join("\n"));
}

void ControlFileEdit::replaceCurrentLine(const QString &newLine)
{
    QTextCursor cursor = textCursor();
    int cpos = textCursor().position();
    QString str = toPlainText();

    int lineStart = currentLineStart();
    int lineEnd   = currentLineEnd();

    str.replace(lineStart, lineEnd - lineStart, newLine);

    setPlainText(str);
    cursor.setPosition(cpos);
    setTextCursor(cursor);
}

void ControlFileEdit::resetParamVariables()
{
    selectedWord = QString();
    prefix = QString();
    param = QString();
    value = QString();
    lower = QString();
    upper = QString();
    paramStart = -1;
}

int ControlFileEdit::freeParamNumber()
{
    static QRegularExpression rx("\\bPARAM\\[(\\d+)\\]=");
    QString content = toPlainText();

    // if no PARAM[n]= line is found, we can use number 1
    if (!content.contains(rx)) return 1;

    // loop over all PARAM[n]= lines and store n in a list
    QList<int> lst;
    int i = content.indexOf(rx, 0);
    int max = 0;

    while (i >= 0) {
        QRegularExpressionMatch rm = rx.match(content, i);
        if (rm.hasMatch()) {
            int v = rm.captured(1).toInt();
            max = qMax(v, max);
            lst.append(v);
        }

        i = content.indexOf(rx, i + 1);
    }

    // try to find the first gap in the list
    for (int n = 1; n < max; ++n) {
        if (!lst.contains(n)) return n;
    }

    // if no gap was found, return max + 1
    return max + 1;
}

void ControlFileEdit::fixAllNumbering()
{
    for (int i = 0; i < itemsNumbered.size(); ++i) {
        fixParameterNumbering(itemsNumbered.at(i));
    }
}

void ControlFileEdit::fixParameterNumbering(const QString &pattern)
{
    QTextCursor cursor = textCursor();
    int cpos = textCursor().position();

    QString content = toPlainText();
    QRegularExpression rx(QString("(%\\s*)?(%1\\[\\d+\\]=)").arg(pattern));
    QRegularExpressionMatchIterator ri = rx.globalMatch(content);

    int n = 1;

    while (ri.hasNext()) {
        QRegularExpressionMatch rm = ri.next();

        if (rm.captured(1).isEmpty()) {
            content.replace(rm.capturedStart(2), rm.capturedLength(2), QString("%1[%2]=").arg(pattern).arg(n));
            ++n;
        }
    }

    setPlainText(content);
    cursor.setPosition(cpos);
    setTextCursor(cursor);
}

void ControlFileEdit::renumberItems()
{
    static QRegularExpression rxp("^([A-Z]+)\\[\\d+\\]=");
    QRegularExpressionMatch rmp = rxp.match(lineUnderCursor());

    if (!rmp.hasMatch()) return;
    if (itemsNumbered.contains(rmp.captured(1))) fixParameterNumbering(rmp.captured(1));
}

int ControlFileEdit::replaceAll(QString findString, bool isExpr, QString replaceString, bool caseSensitively, bool wholeWords)
{
    QTextDocument *doc = document();
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Start);
    QTextCursor newCursor = cursor;
    int count = 0;

    QTextDocument::FindFlags options;
    if (caseSensitively) options = options | QTextDocument::FindCaseSensitively;
    if (wholeWords)      options = options | QTextDocument::FindWholeWords;

    if (!findString.isEmpty())
    {
        while (true) {
            if (isExpr) {
                newCursor = doc->find(QRegularExpression(findString), newCursor, options);
            } else {
                newCursor = doc->find(findString, newCursor, options);
            }

            if (!newCursor.isNull()) {
                if (newCursor.hasSelection()) {
                    newCursor.insertText(replaceString);
                    count++;
                }
            } else {
                break;
            }
        }
    }

    cursor.endEditBlock();
    return count;
}

void ControlFileEdit::commentLine()
{
    QString c(commentSign());
    QString s(lineUnderCursor());

    if (s.trimmed().left(c.length()) != c) {
        replaceCurrentLine(QString("%1 %2").arg(c, s));
    } else {
        return; // already starts with a comment sign. Nothing to do.
    }

    static QRegularExpression rxp("^([A-Z]+)\\[\\d+\\]=");
    QRegularExpressionMatch rmp = rxp.match(s); // match the original line without the comment sign

    if (!rmp.hasMatch()) return; // no numbered item
    if (itemsNumbered.contains(rmp.captured(1))) fixParameterNumbering(rmp.captured(1));
}

void ControlFileEdit::uncommentLine()
{
    QString s(lineUnderCursor());
    static QRegularExpression rxl("^(?:[\\/|%]+\\s*)?(.*)");
    QRegularExpressionMatch rml = rxl.match(s);

    if (rml.hasMatch()) {
        replaceCurrentLine(rml.captured(1));
    } else {
        return; // doesn't start with a comment sign. Nothing to do.
    }

    static QRegularExpression rxp("^([A-Z]+)\\[\\d+\\]=");
    QRegularExpressionMatch rmp = rxp.match(rml.captured(1)); // match the original line without the comment sign

    if (!rmp.hasMatch()) return; // no numbered item
    if (itemsNumbered.contains(rmp.captured(1))) fixParameterNumbering(rmp.captured(1));
}

bool ControlFileEdit::isCommented()
{
    QString s(lineUnderCursor());
    static QRegularExpression rx("^\\s*[\\/|%]+\\s*.*");
    QRegularExpressionMatch rm = rx.match(s);
    return rm.hasMatch();
}

void ControlFileEdit::toggleCommentLine()
{
    if (isCommented()) {
        uncommentLine();
    } else {
        commentLine();
    }
}

QString ControlFileEdit::commentSign()
{
    QString s = toPlainText();

    if (s.contains("PHASE="))   return QString("//");
    if (s.contains("VERZERR=")) return QString("%");

    // fallback
    return QString("%");
}

bool ControlFileEdit::isYes()
{
    if (value == "Y") return true;
    if (value == "y") return true;
    if (value == "J") return true;
    if (value == "j") return true;
    return false;
}

bool ControlFileEdit::isSubstitution()
{
    static QRegularExpression rx("^E=\\([A-Z\\+\\-\\d]+\\([^\\)]+\\),.+\\)");
    QRegularExpressionMatch rm = rx.match(lineUnderCursor());
    return rm.hasMatch();
}

void ControlFileEdit::setLambda()
{
    QString lam = static_cast<QAction*>(sender())->text();
    qDebug() << QString("ControlFileEdit::setLambda(): lam file selected: %1").arg(lam);

    static QRegularExpression rx("LAMBDA=\\S+");

    QString s = toPlainText();
    s.replace(rx, QString("LAMBDA=%1").arg(lam));
    setPlainText(s);
}

QMap<int, QStringList> ControlFileEdit::findAll(const QString &pattern)
{
    QRegularExpression rx(pattern);
    QStringList content = toPlainText().split("\n");

    QMap<int, QStringList> matches;

    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch rm = rx.match(content.at(i));
        if (rm.hasMatch()) matches[i+1] = QStringList() << rm.captured(1) << content.at(i);
    }

    return matches;
}

void ControlFileEdit::setCurrentLine(int l)
{
    moveCursor(QTextCursor::End);
    QTextCursor cur(document()->findBlockByLineNumber(l));
    setTextCursor(cur);
}

void ControlFileEdit::setSubstitution()
{
    bool ok;
    QString s = BgmnStrParser::setSubstitution(lineUnderCursor(), ok);
    if (ok) replaceCurrentLine(s);
}

void ControlFileEdit::revertSubstitution()
{
    bool ok;
    QString s = BgmnStrParser::revertSubstutition(lineUnderCursor(), ok);
    if (ok) replaceCurrentLine(s);
}

void ControlFileEdit::setAllCoordinatesRefined()
{
    bool ok;
    double fcoordLimit = settings->value("bgmnProject/coordinatelimits", 0.05).toDouble();
    QString s = BgmnStrParser::allCoordinatesRefined(toPlainText(), fcoordLimit, ok);
    if (ok) setPlainText(s);
}

void ControlFileEdit::setAllCoordinatesFixed()
{
    bool ok;
    QString s = BgmnStrParser::allCoordinatesFixed(toPlainText(), ok);
    if (ok) setPlainText(s);
}

void ControlFileEdit::setAllTdsRefined()
{
    bool ok;
    double tdsLimit = settings->value("bgmnProject/tdslimits", 0.02).toDouble();
    QString s = BgmnStrParser::allTdsRefined(toPlainText(), tdsLimit, ok);
    if (ok) setPlainText(s);
}

void ControlFileEdit::setAllTdsFixed()
{
    bool ok;
    QString s = BgmnStrParser::allTdsFixed(toPlainText(), ok);
    if (ok) setPlainText(s);
}

void ControlFileEdit::showHelp()
{
    emit helpText("controlFileEditor");
}
