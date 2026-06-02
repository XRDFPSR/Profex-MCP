/***************************************************************************
                          controlfileedit.h  -  description
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

#ifndef CONTROLFILEEDIT_H
#define CONTROLFILEEDIT_H

// #include <QPlainTextEdit>
#include <QMenu>
#include <QContextMenuEvent>
#include <QAction>
#include <QStringList>
#include <QRegularExpression>

#include "../libXrdIO/settingsmanager.h"
#include "3rdparty/codeeditor/codeeditor.h"

class ControlFileEdit : public CodeEditor
{
    Q_OBJECT
public:
    enum ControlFileEditFileType {
        BGMN_SAV, BGMN_LST, BGMN_STR,
        OTHER
    };

    explicit ControlFileEdit(QWidget *parent = nullptr, const ControlFileEditFileType ft = ControlFileEditFileType::OTHER);

    // sets/gets the file's absolute filename
    inline void setFileName(const QString &s) {absFileName = s;}
    inline QString getFileName() {return absFileName;}

    // sets the file's base name
    inline void setBaseName(const QString &s) {editorBaseName = s;}

    // Returns the word including the selection. Example:
    // Word = "LIST=file name.lst"
    // Spaces within the selection will be ignored. This can be used to
    // return words containing spaces.
    // If nothing useful could be identified as a word, QString() is returned.
    QString blockUnderSelection();

    // returns the word precisely under the cursor from whatever text is provided
    QString wordUnderSelection();

    // parses the word "w", which is of type "PARAM=PARAMETER=VALUE_LOWER^UPPER", and writes the
    // resulting strings into pref, par, val, low and up. Empty strings will be empty QString().
    bool splitParameterBlock(const QString &w, QString &pref, QString &par, QString &val, QString &low, QString &up);

    // also parses the word w, which is of type "PARAMETER[n]=filename.ext", whereas [n] is optional. This is needed
    // because file names can contain "_", which would be interpreted as the start of the lower limit in
    // ::splitParameterBlock().
    bool splitSelectionUnderCursor(const QString &w, QString &par, QString &val);

    // searches and replaces text in the text document
    int replaceAll(QString findString, bool isExpr, QString replaceString, bool caseSensitively, bool wholeWords);

    // returns all matching patterns
    QMap<int, QStringList> findAll(const QString &);

    // reset text zoom
    void resetZoom();

    inline void setTextChanged(bool b) {hasChangedText = b;}
    inline bool getTextChanged()       {return hasChangedText;}

    void setCurrentLine(int);

    inline ControlFileEditFileType getFileType() const {return fileType;}

private:
    // extracts the number "n" from a line of form STRUC[n]=file.str
    int fileNumber(const QString &selection);

    // extracts the basename "file" from a line of form STRUC[n]=file.str
    QString fileBaseName(const QString &selection);

    // inserts a new line into the editor after line "before"
    void insertLine(const QString &before, const QString &newLine, bool allowDuplicates = false);

    // replaces the current line text with "newLine"
    void replaceCurrentLine(const QString &newLine);

    // creates and shows the mouse context menu
    void contextMenuEvent(QContextMenuEvent *);

    // zoom the text
    void wheelEvent(QWheelEvent *);

    // parses the parameter and splits it in selectedWord, param, value, lower, upper
    // returns true if successful
    bool parseParam();

    // tests whether a parameter is currently refined fix, iso, or aniso.
    // fix = 0, iso = 1, iso without limits = 2, aniso = 3
    int currentRefinementState();

    // returns true if the current value is an allowed limit of the current parameter
    bool isIntegerAtLowerLimit();
    bool isIntegerAtUpperLimit();

    // resets the global variables prefix, value, param, lower, upper, selectedWord, paramStart
    // to empty
    void resetParamVariables();

    // returns the lowest free number n for PARAM[n]= in a control file
    int freeParamNumber();

    // make sure no gaps occur in parameter numbers PARAM[n]=, STRUC[n] etc.
    void fixParameterNumbering(const QString &);
    void fixAllNumbering();

    // returns the line under the cursor
    QString lineUnderCursor();

    // returns true if the line begins with a comment sign, else returns false
    bool isCommented();

    // returns true if the parameter is set to YES
    bool isYes();

    // returns true if the E= line contains a substitution
    bool isSubstitution();

    // return the start and end positions of the current line within the document string
    int currentLineStart();
    int currentLineEnd();

    // returns the comment sign, depending on the type of file currently displayed
    QString commentSign();

    // replaces the currently selected parameter
    void replaceCurrentParameter(const QString &);

    // adds B1 and k2 depending on the selected RP=n
    QString addRpConditionalParameters(int);

    SettingsManager *settings;

    QString absFileName;
    QAction *actionSetRefinable;
    QAction *actionSetRefinableIso;
    QAction *actionSetRefinableAniso;
    QAction *actionSetRefinableFix;
    QAction *actionSetRefinableNumbered;
    QAction *actionOpenTextFile;
    QAction *actionOpenGraphFile;
    QAction *actionAddStrucOutFile;
    QAction *actionAddSimpleStrucOutFile;
    QAction *actionAddResFcfOutFile;
    QAction *actionAddPdbOutFile;

    QAction *actionSetParameterYes;
    QAction *actionSetParameterNo;

    QAction *actionToggleRefineStateUp;
    QAction *actionToggleRefineStateDown;

    QAction *actionAllCoordinatesRefined;
    QAction *actionAllCoorindatesFixed;
    QAction *actionAllTdsRefined;
    QAction *actionAllTdsFixed;

    QAction *actionToggleCommentLine;
    QAction *actionSetSubstitution;
    QAction *actionRevertSubstitution;
    QAction *actionRenumberItems;

    QAction *actionShowHelp;

    QStringList paramsOnlyIso;
    QStringList paramsIso;
    QStringList paramsAniso;
    QStringList paramsFix;
    QStringList paramsNumbered;
    QStringList filesText;
    QStringList filesGraph;
    QStringList paramsYesNo;
    QStringList paramsIntegerValue;
    QStringList itemsNumbered;

    QString editorBaseName;

    QMultiMap<QString, QString> lamFiles;

    ControlFileEditFileType fileType;
    int paramStart;
    int maxNThreads;
    QString prefix;
    QString value;
    QString param;
    QString lower;
    QString upper;
    QString selectedWord;
    QString paramFilesPattern;
    QString paramNamesPattern;
    QString paramValuesPattern;
    QString paramYesNoPattern;
    bool hasChangedText;
    int zoomLevel;

    QRegularExpression rxFilesBlock;
    QRegularExpression rxParamBlock;
    QRegularExpression rxElementBlock;
    QRegularExpression rxYesNoBlock;
    QRegularExpression rxLambdaBlock;
    QRegularExpression rxGoalsBlock;

signals:
    void openText(const QString &);
    void openGraph(const QString &);
    void wordUnderCursor(QStringList);
    void helpText(const QString &);

private slots:
    void setRefinableIso();
    void setRefinableAniso();
    void setRefinableFix();
    void setRefinableNumbered();
    void openTextFile();
    void openGraphFile();
    void toggleRefineStateUp();
    void toggleRefineStateDown();
    void toggleParamYes();
    void toggleParamNo();
    void setSubstitution();
    void revertSubstitution();
    void renumberItems();

    void addStrucOutFile();
    void addSimpleStrucOutFile();
    void addResFcfOutFile();
    void addPdbOutFile();

    void commentLine();
    void uncommentLine();
    void toggleCommentLine();
    void setLambda();

    void broadcastCurrentWord();
    void showHelp();

    // sets all fractional coordinates to refined or fixed
    void setAllCoordinatesRefined();
    void setAllCoordinatesFixed();
    void setAllTdsRefined();
    void setAllTdsFixed();
};

#endif // CONTROLFILEEDIT_H
