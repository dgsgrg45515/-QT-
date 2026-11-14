#ifndef HTTPMGR_H
#define HTTPMGR_H

#include "global.h"
#include "singleton.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QUrl>

class HttpMgr : public QObject, public Singleton<HttpMgr> {
    Q_OBJECT

public:
    ~HttpMgr() override = default;
    void PostHttpReq(const QUrl& url, const QJsonObject& json, ReqId reqId, Modules module);

private:
    friend class Singleton<HttpMgr>;
    HttpMgr();

    QNetworkAccessManager manager_;

private slots:
    void slotHttpFinish(ReqId id, const QString& response, ErrorCodes error, Modules module);

signals:
    void sig_login_mod_finish(ReqId id,QString response,ErrorCodes error,Modules module);
    void sigHttpFinish(ReqId id, QString response, ErrorCodes error, Modules module);
    void sigRegisterModuleFinish(ReqId id, QString response, ErrorCodes error, Modules module);
    void sig_reset_mod_finish(ReqId id, QString response, ErrorCodes error);
};

#endif
