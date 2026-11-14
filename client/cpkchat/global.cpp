#include "global.h"

QString gate_url_prefix;

QPixmap loadUserIcon(QString icon)
{
    if (icon.startsWith(":/res/")) {
        icon.replace(0, 6, ":/Image/");
    }

    QPixmap pixmap(icon);
    if (pixmap.isNull()) {
        pixmap.load(":/Image/head_1.jpg");
    }
    return pixmap;
}

std::function<void(QWidget*)> repolish = [](QWidget* widget) {
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
};

std::function<QString (QString)> xorString=[](QString input){
    QString result=input;
    int length = input.length();
    length = length%255;
    for(int i=0;i<length;i++){
        result[i]=QChar(static_cast<ushort>(input[i].unicode()^static_cast<ushort>(length)));
    }
    return result;
};
