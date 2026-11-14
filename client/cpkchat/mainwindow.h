#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "resetdialog.h"
#include <QMainWindow>
#include "chatdialog.h"
#include "logindialog.h"
#include "registerdialog.h"
#include "tcpmgr.h"


class LoginDialog;
class RegisterDialog;
class QStackedWidget;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void showRegisterPage();
    void showLoginPage();
    void showResetPage();
    void showChatPage();

private:
    void showPage(QWidget* page);

    Ui::MainWindow* ui;
    QStackedWidget* stack_;
    LoginDialog* loginDialog_;
    RegisterDialog* registerDialog_;
    ResetDialog* resetDialog_;
    ChatDialog* chatDialog_;
};

#endif
