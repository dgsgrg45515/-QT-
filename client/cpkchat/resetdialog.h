#ifndef RESETDIALOG_H
#define RESETDIALOG_H
#include "global.h"
#include <QDialog>

namespace Ui {
class ResetDialog;
}

class ResetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResetDialog(QWidget *parent = nullptr);
    ~ResetDialog();

private slots:
    void on_return_btn_clicked();
    void on_varify_btn_clicked();
    void slot_reset_mod_finish(ReqId Id,QString res,ErrorCodes err);
    void on_sure_btn_clicked();

private:
    Ui::ResetDialog *ui;
    bool checkUserValid();
    bool checkPassValid();
    bool checkEmailValid();
    bool checkVarifyValid();
    void AddTipErr(TipErr re,QString tips);
    void DelTipErr(TipErr te);
    void showTip(QString str,bool b_ok);
    QMap<TipErr,QString> _tip_errs;
    QMap<ReqId,std::function<void (const QJsonObject&)>> _handlers;
    void iniHandlers();
signals:
    void switchLogin();
};

#endif // RESETDIALOG_H
