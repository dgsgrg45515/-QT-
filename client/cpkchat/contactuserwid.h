#ifndef CONTACTUSERWID_H
#define CONTACTUSERWID_H

#include <QWidget>

namespace Ui {
class ContactUserWid;
}

class ContactUserWid : public QWidget
{
    Q_OBJECT

public:
    explicit ContactUserWid(QWidget *parent = nullptr);
    ~ContactUserWid();

private:
    Ui::ContactUserWid *ui;
};

#endif // CONTACTUSERWID_H
