#include "customizeedit.h"

Customizeedit::Customizeedit(QWidget* parent):QLineEdit(parent),_max_len(0)
{
    connect(this,&QLineEdit::textChanged,this,&Customizeedit::limiTextLength);
}

void Customizeedit::SetMaxLength(int maxLen){
    _max_len=maxLen;
}

