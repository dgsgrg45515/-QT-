#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H
#include "global.h"
#include <QDialog>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

private:
    Ui::LoginDialog *ui;
    bool checkUserValid();
    bool checkPwdValid();
    void initHttpHandlers();
    QMap<ReqId, std::function<void(const QJsonObject&)>> handlers_;
    QMap<TipErr, QString> _tip_errs;
    void AddTipErr(TipErr te,QString tips);
    void DelTipErr(TipErr te, QString tips);
    bool enableBtn(bool enable);
    void showTip(QString str,bool b_ok);
    int _uid;
    QString _token;
public slots:
    void slot_tcp_con_finish(bool bsuccess);
    void slot_forget_pwd();
    void on_login_btn_clicked();
    void slot_login_mod_finish(ReqId id, QString res, ErrorCodes err);
    void slot_login_failed(int err);
signals:
    void swithchRegister();
    void switchReset();
    void sig_connect_tcp(ServerInfo si);
};

#endif // LOGINDIALOG_H
