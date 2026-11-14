#ifndef CLICKEDLABEL_H
#define CLICKEDLABEL_H

#include <QLabel>
#include <QMouseEvent>
#include <QEvent>
#include <QtGlobal>

#include "global.h"

class ClickedLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickedLabel(QWidget* parent = nullptr);

    void SetState(QString normal = "",
                  QString hover = "",
                  QString press = "",
                  QString select = "",
                  QString select_hover = "",
                  QString select_press = "");

    ClickLbState GetCurState();
    bool SetCurState(ClickLbState state);
    void ResetNormalState();

protected:
    void mousePressEvent(QMouseEvent* event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent* event) override;
#else
    void enterEvent(QEvent* event) override;
#endif
    void leaveEvent(QEvent* event) override;
    void mouseReleaseEvent(QMouseEvent*event) override;

signals:
    void clicked(QString,ClickLbState);

private:
    QString _normal;
    QString _normal_hover;
    QString _normal_press;

    QString _selected;
    QString _selected_hover;
    QString _selected_press;

    ClickLbState _curstate;
};

#endif
