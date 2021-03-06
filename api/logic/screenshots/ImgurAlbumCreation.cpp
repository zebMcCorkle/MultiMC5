#include "ImgurAlbumCreation.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QStringList>

#include "net/URLConstants.h"
#include "Env.h"
#include <QDebug>

ImgurAlbumCreation::ImgurAlbumCreation(QList<ScreenshotPtr> screenshots) : NetAction(), m_screenshots(screenshots)
{
	m_url = URLConstants::IMGUR_BASE_URL + "album.json";
	m_status = Status::NotStarted;
}

void ImgurAlbumCreation::executeTask()
{
	m_status = Status::InProgress;
	QNetworkRequest request(m_url);
	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Uncached)");
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	request.setRawHeader("Authorization", "Client-ID 5b97b0713fba4a3");
	request.setRawHeader("Accept", "application/json");

	QStringList ids;
	for (auto shot : m_screenshots)
	{
		ids.append(shot->m_imgurId);
	}

	const QByteArray data = "ids=" + ids.join(',').toUtf8() + "&title=Minecraft%20Screenshots&privacy=hidden";

	QNetworkReply *rep = ENV.qnam().post(request, data);

	m_reply.reset(rep);
	connect(rep, &QNetworkReply::uploadProgress, this, &ImgurAlbumCreation::downloadProgress);
	connect(rep, &QNetworkReply::finished, this, &ImgurAlbumCreation::downloadFinished);
	connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
}
void ImgurAlbumCreation::downloadError(QNetworkReply::NetworkError error)
{
	qDebug() << m_reply->errorString();
	m_status = Status::Failed;
}
void ImgurAlbumCreation::downloadFinished()
{
	if (m_status != Status::Failed)
	{
		QByteArray data = m_reply->readAll();
		m_reply.reset();
		QJsonParseError jsonError;
		QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
		if (jsonError.error != QJsonParseError::NoError)
		{
			qDebug() << jsonError.errorString();
			emit failed();
			return;
		}
		auto object = doc.object();
		if (!object.value("success").toBool())
		{
			qDebug() << doc.toJson();
			emit failed();
			return;
		}
		m_deleteHash = object.value("data").toObject().value("deletehash").toString();
		m_id = object.value("data").toObject().value("id").toString();
		m_status = Status::Finished;
		emit succeeded();
		return;
	}
	else
	{
		qDebug() << m_reply->readAll();
		m_reply.reset();
		emit failed();
		return;
	}
}
void ImgurAlbumCreation::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_progressTotal = bytesTotal;
	m_progress = bytesReceived;
	emit progress(bytesReceived, bytesTotal);
}
