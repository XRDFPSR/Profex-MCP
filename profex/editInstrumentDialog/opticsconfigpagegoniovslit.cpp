#include "opticsconfigpagegonio.h"

OpticsConfigPageGonio::OpticsConfigPageGonio(QWidget *parent) :
 AbstractOpticsConfigPage(parent),
  ui(new Ui::OpticsConfigPageGonioForm)

{
    ui->setupUi(this);
}

void OpticsConfigPageGonio::setText(const QString &)
{

}

QString OpticsConfigPageGonio::getText()
{
    return QString();
}

void OpticsConfigPageGonio::parseText(const QString &)
{

}

