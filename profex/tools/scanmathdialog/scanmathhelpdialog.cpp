#include "scanmathhelpdialog.h"
#include "ui_scanmathhelpdialog.h"

ScanMathHelpDialog::ScanMathHelpDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ScanMathHelpDialog)
{
    ui->setupUi(this);
}

ScanMathHelpDialog::~ScanMathHelpDialog()
{
    delete ui;
}
