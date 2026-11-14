#ifndef CUSTOMIZEEDIT_H
#define CUSTOMIZEEDIT_H
#include <QDebug>
#include <QLineEdit>

class Customizeedit : public QLineEdit
{
    Q_OBJECT
public:
    Customizeedit(QWidget* parent=nullptr);
    void SetMaxLength(int maxLen);
protected:
    void focusOutEvent(QFocusEvent*event){
        QLineEdit::focusOutEvent(event);
        emit sig_focus_out();
    }
private:
    void limiTextLength(QString text){
        if(_max_len<=0){
            return;
        }
        QByteArray byteArray =text.toUtf8();
        if(byteArray.size()>_max_len){
            byteArray=byteArray.left(_max_len);
            this->setText(QString::fromUtf8(byteArray));
        }
    }

    int _max_len;
signals:
    void sig_focus_out();
};
#endif // CUSTOMIZEEDIT_H
