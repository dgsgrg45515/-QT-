#include "logindialog.h"
#include "ui_logindialog.h"
#include "clickedlabel.h"
#include "httpmgr.h"
#include "tcpmgr.h"
LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    connect(ui->reg_button,&QPushButton::clicked,this,&LoginDialog::swithchRegister);
    //把密码设置成password模式
    ui->pass_edit->setEchoMode(QLineEdit::Password);
    ui->forgetlabel->SetState("normal","hover","","selected","selected_hover","");
    ui->forgetlabel->setCursor(Qt::PointingHandCursor);
    connect(ui->forgetlabel,&ClickedLabel::clicked,this,&LoginDialog::slot_forget_pwd);
    initHttpHandlers();
    //连接登录回包信号
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_login_mod_finish, this,
            &LoginDialog::slot_login_mod_finish);
    connect(this,&LoginDialog::sig_connect_tcp,TcpMgr::GetInstance().get(),&TcpMgr::slot_tcp_connect);
    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_con_success,this, &LoginDialog::slot_tcp_con_finish);
    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_login_failed,this, &LoginDialog::slot_login_failed);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog:: slot_forget_pwd(){
    qDebug()<<"slot forget pwd";
    emit switchReset();
}

void LoginDialog::on_login_btn_clicked(){
    qDebug()<<"login btn clicked";
    if(checkUserValid()==false){
        return ;
    }
    if(checkPwdValid()==false){
        return ;
    }
    auto user=ui->user_edit->text();
    auto pwd=ui->pass_edit->text();
    QJsonObject json_obj;
    json_obj["user"]=user;
    json_obj["passwd"]=xorString(pwd);
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_login"),
                                        json_obj,ReqId::ID_LOGIN_USER,Modules::LOGINMOD);
}

bool LoginDialog::checkUserValid(){

    auto user = ui->user_edit->text();
    if(user.isEmpty()){
        qDebug() << "User empty " ;
        return false;
    }

    return true;
}

bool LoginDialog::checkPwdValid(){
    auto pwd = ui->pass_edit->text();
    if(pwd.length() < 6 || pwd.length() > 15){
        qDebug() << "Pass length invalid";
        return false;
    }

    return true;
}

void LoginDialog::initHttpHandlers()
{
    //注册获取登录回包逻辑
    handlers_.insert(ReqId::ID_LOGIN_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            if(error == ServerErrorCodes::SERVER_PASSWORD_INVALID){
                showTip(tr("用户不存在或密码错误"),false);
            }else if(error == ServerErrorCodes::SERVER_RPC_FAILED){
                showTip(tr("服务器内部服务异常"),false);
            }else{
                showTip(tr("登录失败"),false);
            }
            enableBtn(true);
            return;
        }
        auto user = jsonObj["user"].toString();

        //发送信号通知tcpMgr发送长链接
        ServerInfo si;
        si.Uid = jsonObj["uid"].toInt();
        si.Host = jsonObj["host"].toString();
        si.Port = jsonObj["port"].toString();
        si.Token = jsonObj["token"].toString();

        _uid = si.Uid;
        _token = si.Token;
        qDebug()<< "user is " << user << " uid is " << si.Uid <<" host is "
                 << si.Host << " Port is " << si.Port << " Token is " << si.Token;
        emit sig_connect_tcp(si);
    });
}

void LoginDialog::slot_login_mod_finish(ReqId id,QString res ,ErrorCodes error){
    if(error!=ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"),false);
        return ;
    }
    // 解析 JSON 字符串,res需转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    //json解析错误
    if(jsonDoc.isNull()){
        showTip(tr("json解析错误"),false);
        return;
    }
    
    //json解析错误
    if(!jsonDoc.isObject()){
        showTip(tr("json解析错误"),false);
        return;
    }
    
    
    //调用对应的逻辑,根据id回调。
    handlers_[id](jsonDoc.object());
    
    return;
}


void LoginDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
        ui->err_tip->setProperty("state","normal");
    }else{
        ui->err_tip->setProperty("state","err");
    }
    
    ui->err_tip->setText(str);
    
    repolish(ui->err_tip);
}

bool LoginDialog::enableBtn(bool enable){
    ui->login_btn->setEnabled(enable);
    ui->reg_button->setEnabled(enable);
    return true;
}

void LoginDialog::slot_tcp_con_finish(bool bsuccess){
    if(bsuccess){
        showTip(tr("聊天服务连接成功，正在登录..."),true);
        QJsonObject jsonObj;
        jsonObj["uid"] = _uid;
        jsonObj["token"] = _token;

        QJsonDocument doc(jsonObj);
        QByteArray jsonString = doc.toJson(QJsonDocument::Indented);

        //发送tcp请求给chatserver
        emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_CHAT_LOGIN, jsonString);

    }else{
        showTip(tr("网络异常"),false);
        enableBtn(true);
    }


}

void LoginDialog::slot_login_failed(int err)
{
    if(err == ServerErrorCodes::SERVER_PASSWORD_INVALID){
        showTip(tr("用户不存在或密码错误"),false);
    }else if(err == ServerErrorCodes::SERVER_RPC_FAILED){
        showTip(tr("服务器内部服务异常"),false);
    }else{
        showTip(tr("登录失败"),false);
    }
    enableBtn(true);
}


