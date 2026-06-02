#ifndef EDITINSTRUMENTPARSINGERRORDIALOG_H
#define EDITINSTRUMENTPARSINGERRORDIALOG_H

#include <QDialog>
#include <QMap>

namespace Ui {
class EditInstrumentParsingErrorDialog;
}

class EditInstrumentParsingErrorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditInstrumentParsingErrorDialog(int mode, QWidget *parent = nullptr);
    ~EditInstrumentParsingErrorDialog();

    void setErrorMap(const QMap<QString, QString> &m);

private:
    Ui::EditInstrumentParsingErrorDialog *ui;
};

#endif // EDITINSTRUMENTPARSINGERRORDIALOG_H
