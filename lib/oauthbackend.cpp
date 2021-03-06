#include <QtCore/QDebug>
#include "oauthbackend.h"
#include "oauthbackend_p.h"
#include "isecurestore.h"

namespace QCloud
{

OAuthBackendPrivate::OAuthBackendPrivate(OAuthBackend* parent)
{
    oauth = new QOAuth::Interface(parent);
}

OAuthBackendPrivate::~OAuthBackendPrivate()
{
}

OAuthBackend::OAuthBackend (QObject* parent) : IBackend (parent)
    , d (new OAuthBackendPrivate(this))
{
}

OAuthBackend::~OAuthBackend()
{
    d->oauth->setNetworkAccessManager(NULL);
    delete d;
}

QString OAuthBackend::appKey() const
{
    return d->appKey;
}

QString OAuthBackend::appSecret() const
{
    return d->appSecret;
}

QByteArray OAuthBackend::oauthToken() const
{
    return d->oauthToken;
}

QByteArray OAuthBackend::oauthTokenSecret() const
{
    return d->oauthTokenSecret;
}

bool OAuthBackend::requestToken()
{
    d->oauth->setRequestTimeout (timeout());

    QOAuth::ParamMap map = d->oauth->requestToken (d->requestTokenUrl, QOAuth::POST, QOAuth::HMAC_SHA1);

    if (d->oauth->error() == 200) {
        qDebug() << map;
        d->oauthToken = map.value (QOAuth::tokenParameterName());
        d->oauthTokenSecret = map.value (QOAuth::tokenSecretParameterName());
        return true;
    }

    else {
        return false;
    }
}

QOAuth::ParamMap OAuthBackend::accessToken(bool* ok, QOAuth::ParamMap params)
{
    d->oauth->setRequestTimeout (timeout());

    QOAuth::ParamMap map = d->oauth->accessToken (d->accessTokenUrl, QOAuth::POST, d->oauthToken, d->oauthTokenSecret, QOAuth::HMAC_SHA1, params);

    qDebug() << d->oauth->error();
    if (d->oauth->error() == QOAuth::NoError) {
        d->oauthToken = map.value (QOAuth::tokenParameterName());
        d->oauthTokenSecret = map.value (QOAuth::tokenSecretParameterName());
        if (ok)
            *ok = true;
    }
    else {
        if (ok)
            *ok = false;
    }
    return map;
}


void OAuthBackend::setRequestTokenUrl (const QString& url)
{
    d->requestTokenUrl = url;
}

void OAuthBackend::setAccessTokenUrl (const QString& url)
{
    d->accessTokenUrl = url;
}

void OAuthBackend::setAppKey (const QString& appkey)
{
    d->appKey = appkey;
    d->oauth->setConsumerKey (d->appKey.toAscii());
}

void OAuthBackend::setAppSecret (const QString& appsecret)
{
    d->appSecret = appsecret;
    d->oauth->setConsumerSecret (d->appSecret.toAscii());
}


uint OAuthBackend::timeout() const
{
    return 0;
}

void OAuthBackend::setNetworkAccessManager (QNetworkAccessManager* manager)
{
    IBackend::setNetworkAccessManager (manager);
    d->oauth->setNetworkAccessManager (manager);
}

void OAuthBackend::setAuthUrl(const QUrl& url)
{
    d->authUrl = url;
}

QByteArray OAuthBackend::authorizationString(const QUrl& url, QOAuth::HttpMethod method, QOAuth::ParamMap params, QOAuth::ParsingMode mode)
{
    return d->oauth->createParametersString(QString::fromAscii(url.toEncoded()), method, d->oauthToken, d->oauthTokenSecret, QOAuth::HMAC_SHA1, params, mode);
}

QByteArray OAuthBackend::inlineString(QOAuth::ParamMap params, QOAuth::ParsingMode mode)
{
    return d->oauth->inlineParameters(params, mode);
}

void OAuthBackend::loadAccountInfo(const QString& key, QSettings& settings, QCloud::ISecureStore* securestore)
{
    d->oauthToken = settings.value("Token").toByteArray();
    securestore->readItem(key, "TokenSecret", d->oauthTokenSecret);
}

void OAuthBackend::saveAccountInfo(const QString& key, QSettings& settings, QCloud::ISecureStore* securestore)
{
    settings.setValue("Token", d->oauthToken);
    securestore->writeItem(key, "TokenSecret", d->oauthTokenSecret);
}

void OAuthBackend::deleteSecretInfo (const QString& key, QCloud::ISecureStore* securestore)
{
    securestore->deleteItem(key, "TokenSecret");
}

}
