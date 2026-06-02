/***************************************************************************
                          prefpagetexteditor.cpp  -  description
                             -------------------
    begin                : Tue May 09 16:00:00 CEST 2017
    copyright            : (C) 2017 by Nicola Doebelin
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

#include <QFontDialog>
#include "prefpagetexteditor.h"
#include "ui_prefpagetexteditor.h"

PrefPageTextEditor::PrefPageTextEditor(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageTextEditor)
{
    ui->setupUi(this);
}

PrefPageTextEditor::~PrefPageTextEditor()
{
    delete ui;
}

void PrefPageTextEditor::initUi()
{
    ui->comboBoxSyntaxHighlighting->addItem(tr("Automatic"));
    ui->comboBoxSyntaxHighlighting->addItem(tr("Light mode"));
    ui->comboBoxSyntaxHighlighting->addItem(tr("Dark mode"));
    ui->comboBoxSyntaxHighlighting->addItem(tr("Off"));

    connect(ui->pushButtonEditorFont, SIGNAL(clicked(bool)), this, SLOT(selectEdFont()));
    initSettings();
}

void PrefPageTextEditor::initSettings()
{
    editorFont.fromString(settings->value("config/editorFont", font().toString()).toString());
    ui->lineEditEditorFont->setFont(editorFont);
    ui->lineEditEditorFont->setText(QString("%1 %2 pt").arg(editorFont.family()).arg(editorFont.pointSize()));

    int hlMode = settings->value("config/editorSyntaxHighlighting", -1).toInt();
    ui->comboBoxSyntaxHighlighting->setCurrentIndex(hlMode + 1);
}

void PrefPageTextEditor::saveSettings()
{
    settings->setValue("config/editorFont", editorFont.toString());
    settings->setValue("config/editorSyntaxHighlighting", ui->comboBoxSyntaxHighlighting->currentIndex() - 1);
}

void PrefPageTextEditor::selectEdFont()
{
    bool ok;
    QFont f = QFontDialog::getFont(&ok, ui->lineEditEditorFont->font(), this);

    if (ok) {
        editorFont = f;
        ui->lineEditEditorFont->setFont(editorFont);
        ui->lineEditEditorFont->setText(QString("%1 %2 pt").arg(editorFont.family()).arg(editorFont.pointSize()));
    }
}
