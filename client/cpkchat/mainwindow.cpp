#include "mainwindow.h"
#include "logindialog.h"
#include "registerdialog.h"
#include "ui_mainwindow.h"
#include "resetdialog.h"
#include <QIcon>
#include <QSize>
#include <QStackedWidget>
#include "tcpmgr.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      stack_(new QStackedWidget(this)),
      loginDialog_(new LoginDialog(stack_)),
      registerDialog_(new RegisterDialog(stack_)),
      resetDialog_(new ResetDialog(stack_)),
      chatDialog_(new ChatDialog(stack_))
    {
    ui->setupUi(this);
    setWindowIcon(QIcon(":/Image/chat.png"));

    loginDialog_->setWindowFlags(Qt::Widget);
    registerDialog_->setWindowFlags(Qt::Widget);
    resetDialog_->setWindowFlags(Qt::Widget);
    chatDialog_->setWindowFlags(Qt::Widget);
    stack_->addWidget(loginDialog_);
    stack_->addWidget(registerDialog_);
    stack_->addWidget(resetDialog_);
    stack_->addWidget(chatDialog_);
    setCentralWidget(stack_);
    showPage(loginDialog_);

    connect(loginDialog_, &LoginDialog::swithchRegister,
        this, &MainWindow::showRegisterPage);
    connect(registerDialog_, &RegisterDialog::switchLogin,
        this, &MainWindow::showLoginPage);
    connect(loginDialog_, &LoginDialog::switchReset,
        this, &MainWindow::showResetPage);
    connect(resetDialog_, &ResetDialog::switchLogin,
        this, &MainWindow::showLoginPage);
    connect(chatDialog_,&ChatDialog::switchChatDialog,
            this,&MainWindow::showChatPage);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_switch_chatdlg,
            this, &MainWindow::showChatPage);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::showPage(QWidget* page) {
    stack_->setCurrentWidget(page);

    const QSize pageSize = page->sizeHint().expandedTo(page->minimumSize());
    setMinimumSize(pageSize);
    setMaximumSize(page->maximumSize());
    resize(pageSize);
}

void MainWindow::showRegisterPage() {
    showPage(registerDialog_);
}

void MainWindow::showLoginPage() {
    showPage(loginDialog_);
}

void MainWindow::showResetPage() {
    showPage(resetDialog_);
}

void MainWindow::showChatPage(){
    stack_->setCurrentWidget(chatDialog_);
    setMinimumSize(QSize(905, 717));
    setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    resize(QSize(1000, 720));
}
