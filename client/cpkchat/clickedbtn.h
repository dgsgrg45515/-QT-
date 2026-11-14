#ifndef CLICKEDBTN_H
#define CLICKEDBTN_H
#include <QPushButton>
#include "global.h"
class ClickedBtn:public QPushButton
{
    Q_OBJECT
public:
    ClickedBtn(QWidget *parent =nullptr);
    ~ClickedBtn();
    void SetState(QString normal,QString hover,QString press);
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
private:
    QString _normal;
    QString _hover;
    QString _press;
};

#endif // CLICKEDBTN_H
