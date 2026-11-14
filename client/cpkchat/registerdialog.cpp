#include "registerdialog.h"

#include "httpmgr.h"
#include "ui_registerdialog.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QLineEdit>

RegisterDialog::RegisterDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::RegisterDialog) {
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->page_register);

    ui->pass_Edit->setEchoMode(QLineEdit::Password);
    ui->confir_Edit->setEchoMode(QLineEdit::Password);
    ui->err_tip->setProperty("state", "normal");
    repolish(ui->err_tip);

    connect(HttpMgr::GetInstance().get(), &HttpMgr::sigRegisterModuleFinish,
        this, &RegisterDialog::slotRegisterModuleFinish);
    initHttpHandlers();
    ui->err_tip->clear();

    connect(ui->user_Edit, &QLineEdit::editingFinished, this, [this]() {
        checkUserValid();
    });
    connect(ui->email_Edit, &QLineEdit::editingFinished, this, [this]() {
        checkEmailValid();
    });
    connect(ui->pass_Edit, &QLineEdit::editingFinished, this, [this]() {
        checkPassValid();
    });
    connect(ui->confir_Edit, &QLineEdit::editingFinished, this, [this]() {
        checkConfirmValid();
    });
    connect(ui->vercode_Edit, &QLineEdit::editingFinished, this, [this]() {
        checkVarifyValid();
    });

    connect(ui->success_back_btn, &QPushButton::clicked, this, [this]() {
        emit switchLogin();
    });
    ui->pass_visible->setCursor(Qt::PointingHandCursor);
    ui->confir_visible->setCursor(Qt::PointingHandCursor);

    ui->pass_visible->SetState("unvisible","unvisible_hover","","visible",
                               "visible_hover","");

    ui->confir_visible->SetState("unvisible","unvisible_hover","","visible",
                                  "visible_hover","");
    //连接点击事件

    connect(ui->pass_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->pass_visible->GetCurState();
        ui->pass_Edit->setEchoMode(state == Normal ? QLineEdit::Password : QLineEdit::Normal);
        qDebug() << "Label was clicked!";
    });

    connect(ui->confir_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->confir_visible->GetCurState();
        ui->confir_Edit->setEchoMode(state == Normal ? QLineEdit::Password : QLineEdit::Normal);
        qDebug() << "Label was clicked!";
    });
}

RegisterDialog::~RegisterDialog() {
    delete ui;
}

void RegisterDialog::AddTipErr(TipErr te, QString tips) {
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void RegisterDialog::DelTipErr(TipErr te) {
    _tip_errs.remove(te);
    if (_tip_errs.empty()) {
        ui->err_tip->clear();
        return;
    }
    showTip(_tip_errs.first(), false);
}

bool RegisterDialog::checkUserValid() {
    if (ui->user_Edit->text().trimmed().isEmpty()) {
        AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}

bool RegisterDialog::checkPassValid()
{
    auto pass = ui->pass_Edit->text();

    if(pass.length() < 6 || pass.length()>15){
        //提示长度不准确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }

    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pass).hasMatch();
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符"));
        return false;;
    }

    DelTipErr(TipErr::TIP_PWD_ERR);

    return true;
}

bool RegisterDialog::checkEmailValid()
{
    //验证邮箱的地址正则表达式
    auto email = ui->email_Edit->text();
    // 邮箱地址的正则表达式
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch(); // 执行正则表达式匹配
    if(!match){
        //提示邮箱不正确
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱地址不正确"));
        return false;
    }

    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}


bool RegisterDialog::checkVarifyValid() {
    if (ui->vercode_Edit->text().trimmed().isEmpty()) {
        AddTipErr(TipErr::TIP_VARIFY_ERR, tr("验证码不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_VARIFY_ERR);
    return true;
}

bool RegisterDialog::checkConfirmValid() {
    const auto pass = ui->pass_Edit->text();
    const auto confirm = ui->confir_Edit->text();
    if (confirm != pass) {
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("两次密码不一致"));
        return false;
    }
    DelTipErr(TipErr::TIP_CONFIRM_ERR);
    return true;
}

void RegisterDialog::on_getcode_btn_clicked() {
    const auto email = ui->email_Edit->text().trimmed();
    static const QRegularExpression emailRegex(
        R"(^[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}$)",
        QRegularExpression::CaseInsensitiveOption);
    if (!emailRegex.match(email).hasMatch()) {
        showTip(tr("邮箱地址格式不正确"), false);
        return;
    }

    ui->getcode_btn->setEnabled(false);
    QJsonObject request{{"email", email}};
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/get_varifycode"), request,
        ReqId::ID_GET_VARIFY_CODE, Modules::REGISTERMOD);
}

void RegisterDialog::on_confirm_btn_clicked()
{
    bool valid = checkUserValid();
    if(!valid){
        return;
    }

    valid = checkEmailValid();
    if(!valid){
        return;
    }

    valid = checkPassValid();
    if(!valid){
        return;
    }

    valid = checkVarifyValid();
    if(!valid){
        return;
    }

    //发送http重置用户请求
    QJsonObject json_obj;
    json_obj["user"] = ui->user_Edit->text();
    json_obj["email"] = ui->email_Edit->text();
    json_obj["passwd"] = xorString(ui->pass_Edit->text());
    json_obj["confirm"] = xorString(ui->confir_Edit->text());
    json_obj["varifycode"] = ui->vercode_Edit->text();
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_register"),
                                        json_obj, ReqId::ID_REG_USER,Modules::REGISTERMOD);
}

void RegisterDialog::on_cancel_btn_clicked() {
    emit switchLogin();
}

void RegisterDialog::slotRegisterModuleFinish(ReqId id, const QString& response,
    ErrorCodes error, Modules) {
    if (id == ReqId::ID_GET_VARIFY_CODE) {
        ui->getcode_btn->setEnabled(true);
    } else if (id == ReqId::ID_REG_USER) {
        ui->confirm_btn->setEnabled(true);
    }

    if (error != ErrorCodes::SUCCESS) {
        showTip(tr("无法连接 GateServer"), false);
        return;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(response.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        showTip(tr("服务器响应格式错误"), false);
        return;
    }
    if (!handlers_.contains(id)) {
        showTip(tr("未知的响应类型"), false);
        return;
    }
    handlers_.value(id)(document.object());
}

void RegisterDialog::initHttpHandlers() {
    handlers_.insert(ReqId::ID_GET_VARIFY_CODE, [this](const QJsonObject& response) {
        const auto error = response.value("error").toInt(-1);
        if (error != ServerErrorCodes::SERVER_SUCCESS) {
            showTip(serverErrorMessage(error), false);
            return;
        }
        showTip(tr("验证码已生成，请查看邮箱"), true);
    });

    handlers_.insert(ReqId::ID_REG_USER, [this](const QJsonObject& response) {
        const auto error = response.value("error").toInt(-1);
        if (error != ServerErrorCodes::SERVER_SUCCESS) {
            showTip(serverErrorMessage(error), false);
            return;
        }
        ui->stackedWidget->setCurrentWidget(ui->page_success);
    });
}

QString RegisterDialog::serverErrorMessage(int error) const {
    switch (error) {
    case ServerErrorCodes::SERVER_JSON_ERROR:
        return tr("请求参数错误");
    case ServerErrorCodes::SERVER_VERIFY_EXPIRED:
        return tr("验证码不存在或已过期");
    case ServerErrorCodes::SERVER_VERIFY_CODE_ERROR:
        return tr("验证码错误");
    case ServerErrorCodes::SERVER_USER_EXISTS:
        return tr("用户名已存在");
    case ServerErrorCodes::SERVER_PASSWORD_INVALID:
        return tr("两次密码不一致");
    default:
        return tr("服务器处理失败，错误码：%1").arg(error);
    }
}

void RegisterDialog::showTip(const QString& text, bool ok) {
    ui->err_tip->setProperty("state", ok ? "normal" : "err");
    ui->err_tip->setText(text);
    repolish(ui->err_tip);
}
