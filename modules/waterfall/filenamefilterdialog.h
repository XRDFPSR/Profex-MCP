#ifndef FILENAMEFILTERDIALOG_H
#define FILENAMEFILTERDIALOG_H

#include <QDialog>

namespace Ui {
class FileNameFilterDialog;
}

class FileNameFilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileNameFilterDialog(QWidget *parent = nullptr);
    ~FileNameFilterDialog();

    void setRegExp(const QString &);
    void setPrefix(const QString &);
    void setSuffix(const QString &);
    void setFileNames(const QStringList &);

    QString getRegExp() const;
    QString getPrefix() const;
    QString getSuffix() const;

private:
    Ui::FileNameFilterDialog *ui;

private slots:
    void regExpChanged(QString);
};

#endif // FILENAMEFILTERDIALOG_H
