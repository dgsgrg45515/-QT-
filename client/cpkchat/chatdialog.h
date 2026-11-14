#ifndef CHATDIALOG_H
#define CHATDIALOG_H
#include <QAction>
#include <QDialog>
#include "global.h"
#include <QRandomGenerator>
#include "chatuserlist.h"
#include <QList>
#include "statewidget.h"
#include "tcpmgr.h"
#include "userdata.h"
#include <QListWidgetItem>
#include "usermgr.h"
namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();
    void addchatuserlist();
protected:
    bool eventFilter(QObject* watched,QEvent*event);
private:
    void handleGlobalMousePress(QMouseEvent*event);
    void ShowSearch(bool bsearch = false);
    Ui::ChatDialog *ui;
    ChatUIMode _mode;
    ChatUIMode _state;
    bool _b_loading;
    void ClearLabelState(StateWidget *lb);
    QList<StateWidget*> _lb_list;
    void AddLBGroup(StateWidget *lb);
    QMap<int,QListWidgetItem*> _chat_items_added;
    int _cur_chat_uid;
    void SetSelectChatItem(int uid=0);
    void SetSelectChatPage(int uid = 0);
    void loadMoreChatUser();
    void loadMoreConUser();
    QWidget* _last_widget;
    void UpdateChatMsg(std::vector<std::shared_ptr<TextChatData>> msgdata);
private slots:
    void slot_loading_chat_user();
    void slot_side_chat();
    void slot_side_contact();
    void slot_text_changed(const QString &str);
    void slot_loading_contact_user();
public slots:
    void slot_apply_friend(std::shared_ptr<AddFriendApply> apply);
    void slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info);
    void slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp);
    void slot_jump_chat_item(std::shared_ptr<SearchInfo> si);
    void slot_friend_info_page(std::shared_ptr<UserInfo> user_info);
    void slot_switch_apply_friend_page();
    void slot_jump_chat_item_from_infopage(std::shared_ptr<UserInfo> user_info);
    void slot_item_clicked(QListWidgetItem* item);
    void slot_append_send_chat_msg(std::shared_ptr<TextChatData> msgdata);
    void slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
signals:
    void switchChatDialog();
};

#endif // CHATDIALOG_H
