#include "editinstrumentparsingerrordialog.h"
#include "ui_editinstrumentparsingerrordialog.h"

EditInstrumentParsingErrorDialog::EditInstrumentParsingErrorDialog(int mode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditInstrumentParsingErrorDialog)
{
    ui->setupUi(this);

    setWindowTitle(tr("Parsing Instrument Configuration"));

    if (mode == 0) {
        QString msg("The values of the following parameters cannot be set in the graphical editor.\n\n");
                msg += "The file will be opened in the text editor.";
        ui->labelMessage->setText(msg);
        ui->pushButtonReject->setHidden(true);
        ui->pushButtonAccept->setText(tr("&OK"));
    } else if (mode == 1) {
        QString msg("The values of the following parameters cannot be set in the graphical editor.\n\n");
                msg += "If you continue, the parameter values will be lost.\n";
                msg += "Click Abort to stay in the text editor.";
        ui->labelMessage->setText(msg);
        ui->pushButtonReject->setText(tr("&Abort"));
        ui->pushButtonAccept->setText(tr("&Continue"));
    }
}

EditInstrumentParsingErrorDialog::~EditInstrumentParsingErrorDialog()
{
    delete ui;
}

void EditInstrumentParsingErrorDialog::setErrorMap(const QMap<QString, QString> &m)
{
    ui->treeWidgetErrors->clear();

    QMapIterator<QString, QString> it(m);

    while (it.hasNext()) {
        it.next();
        ui->treeWidgetErrors->addTopLevelItem(new QTreeWidgetItem(QStringList() << it.key() << it.value()));
    }
}
