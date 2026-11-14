#include "httpmgr.h"

HttpMgr::HttpMgr() : manager_(this) {
    connect(this, &HttpMgr::sigHttpFinish, this, &HttpMgr::slotHttpFinish);
}

void HttpMgr::PostHttpReq(const QUrl& url, const QJsonObject& json, ReqId reqId,
    Modules module) {
    const auto data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(10000);

    auto* reply = manager_.post(request, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply, reqId, module]() {
        const auto response = QString::fromUtf8(reply->readAll());
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool transportFailure = reply->error() != QNetworkReply::NoError && status == 0;
        const bool httpFailure = status < 200 || status >= 300;
        if (transportFailure || httpFailure) {
            qWarning() << "HTTP request failed:" << status << reply->errorString()
                       << response;
            emit sigHttpFinish(reqId, response, ErrorCodes::ERR_NETWORK, module);
        } else {
            emit sigHttpFinish(reqId, response, ErrorCodes::SUCCESS, module);
        }
        reply->deleteLater();
    });
}

void HttpMgr::slotHttpFinish(ReqId id, const QString& response, ErrorCodes error,
    Modules module) {
    if (module == Modules::REGISTERMOD) {
        emit sigRegisterModuleFinish(id, response, error, module);
    } else if (module == Modules::RESETMOD) {
        emit sig_reset_mod_finish(id, response, error);
    }
    else if(module==Modules::LOGINMOD){
        emit sig_login_mod_finish(id,response,error,module);
    }
}
