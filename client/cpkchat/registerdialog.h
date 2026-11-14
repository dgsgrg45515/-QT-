#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include "global.h"

#include <QDialog>
#include <QJsonObject>
#include <QMap>

namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog {
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget* parent = nullptr);
    ~RegisterDialog() override;

signals:
    void switchLogin();

private slots:
    void on_getcode_btn_clicked();
    void on_confirm_btn_clicked();
    void on_cancel_btn_clicked();
    void slotRegisterModuleFinish(ReqId id, const QString& response, ErrorCodes error,
        Modules module);

private:
    void initHttpHandlers();
    void showTip(const QString& text, bool ok);
    QString serverErrorMessage(int error) const;
    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);
    bool checkUserValid();
    bool checkPassValid();
    bool checkEmailValid();
    bool checkVarifyValid();
    bool checkConfirmValid();
    Ui::RegisterDialog* ui;
    QMap<ReqId, std::function<void(const QJsonObject&)>> handlers_;
    QMap<TipErr, QString> _tip_errs;
};

#endif
