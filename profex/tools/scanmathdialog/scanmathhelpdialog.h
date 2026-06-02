#ifndef SCANMATHHELPDIALOG_H
#define SCANMATHHELPDIALOG_H

#include <QDialog>

namespace Ui {
class ScanMathHelpDialog;
}

class ScanMathHelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScanMathHelpDialog(QWidget *parent = nullptr);
    ~ScanMathHelpDialog();

private:
    Ui::ScanMathHelpDialog *ui;
};

#endif // SCANMATHHELPDIALOG_H
