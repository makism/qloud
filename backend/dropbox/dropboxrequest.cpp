#include <QFile>
#include <QEventLoop>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDebug>
#include <QTimer>
#include <qjson/parser.h>
#include "dropboxrequest.h"
#include "dropbox.h"

#include <QDebug>

#define BUF_SIZE 512

QCloud::Request::Error DropboxRequest::error()
{
    return m_error;
}

void DropboxRequest::sendRequest(const QUrl& url,const QOAuth::HttpMethod& method,const QOAuth::ParamMap* paramMap = NULL,QIODevice* device = NULL)
{
    m_error = NoError;
    m_reply = 0;
    QNetworkRequest request(url);
    if (paramMap){
        request.setRawHeader("Authorization",dropbox->authorizationHeader(url,method,(*paramMap)));
        QUrl params;
        for (QOAuth::ParamMap::iterator it=
            QOAuth::ParamMap::iterator(paramMap->begin());it!=paramMap->end();it++)
            params.addQueryItem(it.key(),it.value());
        m_reply = dropbox->networkAccessManager()->post(request,params.encodedQuery());
    }
    else{
        request.setRawHeader("Authorization",dropbox->authorizationHeader(url,method));
        if (method==QOAuth::GET)
            m_reply = dropbox->networkAccessManager()->get(request);
        else if (method==QOAuth::PUT)
            m_reply = dropbox->networkAccessManager()->put(request,device);
        else{
            m_error = Request::NetworkError;
            qDebug() << "Not supported method";
            return ;
        }
    }
    connect(m_reply, SIGNAL(readyRead()) , this , SLOT(readyForRead()));
    connect(m_reply, SIGNAL(finished()) ,this , SLOT(replyFinished()));
}

QString DropboxRequest::getRootType()
{
    if (dropbox->m_globalAccess)
        return "root";
    else
        return "sandbox";
}

void DropboxRequest::readyForRead()
{

}

void DropboxRequest::replyFinished()
{

}

DropboxRequest::~DropboxRequest()
{

}

DropboxUploadRequest::DropboxUploadRequest (Dropbox* dropbox, const QString& localFileName, const QString& remoteFilePath) : 
    m_file (localFileName)
{
    if (!m_file.open (QIODevice::ReadOnly)) {
        m_error = FileError;
        QTimer::singleShot (0, this, SIGNAL (finished()));
        return;
    }

    m_buffer.open(QBuffer::ReadWrite);

    QString surl;
    surl = "https://api-content.dropbox.com/1/files_put/%1/%2";
    QUrl url (surl.arg(getRootType()).arg (remoteFilePath));
    sendRequest(url,QOAuth::PUT,NULL,&m_file);
}

void DropboxUploadRequest::readyForRead()
{
    m_buffer.write (m_reply->readAll());
}


DropboxUploadRequest::~DropboxUploadRequest()
{
}

void DropboxUploadRequest::replyFinished()
{
    if (m_reply->error() != QNetworkReply::NoError) {
        qDebug() << "Reponse error " << m_reply->errorString();
        m_error = NetworkError;
    }

    m_buffer.seek(0);
    // Lets print the HTTP PUT response.
    QVariant result = m_parser.parse (m_buffer.data());
    qDebug() << result;
    m_file.close();
    emit finished();
}

DropboxDownloadRequest::DropboxDownloadRequest (Dropbox* dropbox, const QString& remoteFilePath, const QString& localFileName):
     m_file (localFileName)
{
    if (!m_file.open (QIODevice::WriteOnly)) {
        m_error = FileError;
        qDebug() << "Failed opening file for writing!";
        QTimer::singleShot (0, this, SIGNAL (finished()));
        return;
    }
    QString urlString;
    urlString = "https://api-content.dropbox.com/1/files/%1/%2";
    QUrl url (urlString.arg(getRootType()).arg (remoteFilePath));
    sendRequest(url,QOAuth::GET);
}

DropboxDownloadRequest::~DropboxDownloadRequest()
{
}

void DropboxDownloadRequest::readyForRead()
{
    m_file.write (m_reply->readAll());
}

void DropboxDownloadRequest::replyFinished()
{
    if (m_reply->error() != QNetworkReply::NoError) {
        qDebug() << "Reponse error " << m_reply->errorString();
        m_error = NetworkError;
    }
    m_file.close();
    emit finished();
}

DropboxCopyRequest::DropboxCopyRequest(Dropbox* dropbox, const QString& fromPath, const QString& toPath)
{
    QOAuth::ParamMap paramMap;
    paramMap.clear();
    paramMap.insert("from_path",fromPath.toLocal8Bit());
    paramMap.insert("to_path",toPath.toLocal8Bit());
    paramMap.insert("root",getRootType().toLocal8Bit());
    QUrl url("https://api.dropbox.com/1/fileops/copy");
    m_buffer.open(QIODevice::ReadWrite);
    sendRequest(url,QOAuth::POST,&paramMap);
}

void DropboxCopyRequest::readyForRead()
{
    m_buffer.write(m_reply->readAll());
}

void DropboxCopyRequest::replyFinished()
{
    if (m_reply->error() != QNetworkReply::NoError){
        m_error = NetworkError;
        qDebug() << "Reponse error " << m_reply->errorString();
        return ;
    }
    QVariant result = m_parser.parse(m_buffer.data());
    qDebug() << result;
    emit finished();
}

DropboxCopyRequest::~DropboxCopyRequest()
{
    m_buffer.close();
}

