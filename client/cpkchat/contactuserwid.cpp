#include "contactuserwid.h"
#include "ui_contactuserwid.h"

ContactUserWid::ContactUserWid(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ContactUserWid)
{
    ui->setupUi(this);
}

ContactUserWid::~ContactUserWid()
{
    delete ui;
}
