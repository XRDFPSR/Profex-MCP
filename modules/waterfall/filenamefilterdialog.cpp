#include "filenamefilterdialog.h"
#include "ui_filenamefilterdialog.h"

FileNameFilterDialog::FileNameFilterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileNameFilterDialog)
{
    ui->setupUi(this);
}

FileNameFilterDialog::~FileNameFilterDialog()
{
    delete ui;
}

void FileNameFilterDialog::setRegExp(const QString &s)
{
    ui->lineEditRegExp->setText(s);
}

void FileNameFilterDialog::setPrefix(const QString &s)
{
    ui->lineEditPrefix->setText(s);
}

void FileNameFilterDialog::setSuffix(const QString &s)
{
    ui->lineEditSuffix->setText(s);
}

void FileNameFilterDialog::setFileNames(const QStringList &l)
{
    ui->comboBoxPrevFileName->clear();
    ui->comboBoxPrevFileName->addItems(l);
}

QString FileNameFilterDialog::getRegExp() const
{
    return ui->lineEditRegExp->text();
}

QString FileNameFilterDialog::getPrefix() const
{
    return ui->lineEditPrefix->text();
}

QString FileNameFilterDialog::getSuffix() const
{
    return ui->lineEditSuffix->text();
}

void FileNameFilterDialog::regExpChanged(QString)
{
    QString _fname = ui->comboBoxPrevFileName->currentText();
    QString _rxTxt = ui->lineEditRegExp->text();
    QString _px = ui->lineEditPrefix->text();
    QString _sx = ui->lineEditSuffix->text();

    if (_rxTxt.isEmpty()) {
        ui->lineEditPrevLabel->setText(_px + _fname + _sx);
        return;
    }

    QRegularExpression _rx(_rxTxt);

    if (!_rx.isValid()) {
        ui->lineEditPrevLabel->setText(_px + _sx);
        return;
    }

    QRegularExpressionMatch _rm = _rx.match(_fname);
    QString l;

    if (_rm.hasMatch()) {
        if (_rx.captureCount() <= 0) l = _rm.captured(0);
        else                         l = _rm.captured(1);
    }

    ui->lineEditPrevLabel->setText(_px + l + _sx);
}
