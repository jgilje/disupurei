#include "playlist.h"

#include <QtNetwork/QNetworkReply>
#include <QStandardPaths>

#include <QJsonArray>
#include <QJsonParseError>

#include <QMetaEnum>

#include <QApplication>
#include <QDesktopWidget>

#include <QDebug>

template <typename T>
static T toCaseInsensitiveEnum(const QString& key, bool* ok) {
    auto enumerator = QMetaEnum::fromType<T>();

    for (int i = 0; i < enumerator.keyCount(); i++) {
        if (key.compare(enumerator.key(i), Qt::CaseInsensitive) == 0) {
            *ok = true;
            return static_cast<T>(enumerator.value(i));
        }
    }

    *ok = false;
    return static_cast<T>(-1);
}

Playlist::Playlist(QObject *parent) : QObject(parent) {
    _cachePath.setPath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    if (! _cachePath.exists()) {
        _cachePath.mkpath(".");
    }

    _entryPath.setPath(_cachePath.filePath("entries"));
    if (! _entryPath.exists()) {
        _entryPath.mkpath(".");
    }

    connect(&_metadataRefreshTimer, &QTimer::timeout, this, &Playlist::refreshMetadata);
    _metadataRefreshTimer.start(10000);
}

const Entry &Playlist::next() {
    ++_playbackIterator;
    if (_playbackIterator >= _entries.constEnd()) {
        _playbackIterator = _entries.constBegin();
    }

    return *_playbackIterator;
}

void Playlist::macAddress(const QString &address) {
    _mac = address;
}

void Playlist::url(const QString &url) {
    _url = url;
}

void Playlist::cleanupStaleEntries() {
    QSet<QString> entries = _entryPath.entryList({}, QDir::Files).toSet();

    for (auto& entry : _entries) {
        entries.remove(entry.fileId);
    }

    for (auto& staleEntry : entries) {
        QFile fileEntry(_entryPath.filePath(staleEntry));
        fileEntry.remove();

        qInfo() << "Removed stale entry" << fileEntry.fileName();
    }
}

void Playlist::downloadEntries() {
    if (_refreshIterator == _refreshEntries.end()) {
        _entries = _refreshEntries;
        _playbackIterator = _entries.constEnd();
        emit playlistAvailable();

        cleanupStaleEntries();
        return;
    }

    QFile entryFile(_entryPath.filePath(_refreshIterator->fileId));
    if (! entryFile.exists()) {
        qDebug() << "Downloading" << _refreshIterator->url;
        auto reply = _nam.get(QNetworkRequest(_refreshIterator->url));
        connect(reply, &QNetworkReply::finished, this, &Playlist::onFetchEntryFinished);
    } else {
        ++_refreshIterator;
        downloadEntries();
    }
}

void Playlist::onFetchEntryFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to download item at url" << _refreshIterator->url << ", removing entry";
    } else {
        QFile entryFile(_entryPath.filePath(_refreshIterator->fileId));
        entryFile.open(QIODevice::WriteOnly);
        entryFile.write(reply->readAll());
        _refreshIterator->loaded = true;

    }

    ++_refreshIterator;
    downloadEntries();
}

void Playlist::parseMetadataEntries(QJsonArray entries) {
    _refreshEntries.clear();
    QRect desktopResolution = QApplication::desktop()->screenGeometry();

    for (QJsonValueRef entry_value : entries) {
        QJsonObject entryObj = entry_value.toObject();

        bool typeOk;
        Playlist::Type type = toCaseInsensitiveEnum<Playlist::Type>(entryObj["type"].toString(), &typeOk);
        if (! typeOk) {
            qWarning() << "Garbage in sequence, skipping entry";
            qWarning() << "\t" << entryObj;
            continue;
        }

        Entry entry;
        entry.fileId = entryObj["fileId"].toString();
        entry.filePath = _entryPath.filePath(entry.fileId);
        entry.type = type;
        switch (type) {
            case Playlist::Type::IMAGE:
            // {\"id\":\"ad934f06-973a-4937-b402-0a057871340a\",\"type\":\"image\",\"fileId\":\"7635d251-e3a1-4818-b59c-86b089514256\",\"durationMillis\":15000}
            entry.durationMillis = entryObj["durationMillis"].toInt();
            entry.url.setUrl(QString("%1/api/getImage/%2/%3/%4").arg(_url).arg(entry.fileId).arg(desktopResolution.width()).arg(desktopResolution.height()));
            break;
            case Playlist::Type::VIDEO:
            // {\"id\":\"af46cf69-b8d2-4f5d-bf31-c57736e4f92b\",\"type\":\"video\",\"fileId\":\"e8043297-5ac3-45c6-a92b-ad5e19468c2f\",\"transcodingComplete\":true}
            entry.url.setUrl(QString("%1/api/getVideo/%2/%3").arg(_url).arg(_mac).arg(entry.fileId));
            break;
        }
        _refreshEntries.append(entry);
    }

    _refreshIterator = _refreshEntries.begin();
    downloadEntries();
}

void Playlist::parseMetadata() {
    QFile input(_cachePath.filePath("metadata"));
    input.open(QIODevice::ReadOnly);

    QJsonObject root = openJsonFile(input);
    if (root.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Failed to parse metadata file, located at" << input.fileName();
        return;
    }

    QJsonObject data = root["data"].toObject();
    QJsonObject sequence = data["sequence"].toObject();
    QString sequenceId = sequence["id"].toString();
    qint64 published = sequence["published"].toVariant().toLongLong();
    if (published != _published || _sequenceId != sequenceId) {
        parseMetadataEntries(sequence["entries"].toArray());
    }

    _sequenceId = sequenceId;
    _published = published;
}

void Playlist::refreshMetadata() {
    QNetworkRequest req(QUrl(QString("%1/api/getSequence/%2.json").arg(_url).arg(_mac)));
    auto reply = _nam.get(req);
    connect(reply, &QNetworkReply::finished, this, &Playlist::onRefreshFinished);
}

void Playlist::checkForCachedMetadata() {
    if (QFile(_cachePath.filePath("metadata")).exists()) {
        parseMetadata();
    }
}

void Playlist::onRefreshFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << Q_FUNC_INFO << "Failed to refresh metadata";
        return;
    }

    QFile output(_cachePath.filePath("metadata"));
    output.open(QIODevice::WriteOnly);
    output.write(reply->readAll());
    output.close();

    parseMetadata();
}

QJsonObject Playlist::openJsonFile(QFile& sourceFile) {
    QJsonParseError parseError;
    QString rawJson = QString::fromUtf8(sourceFile.readAll());

    QJsonDocument json = QJsonDocument::fromJson(rawJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse JSON";
        qWarning() << "\t" << parseError.errorString();
        return QJsonObject();
    }

    QJsonObject rootObject = json.object();
    return rootObject;
}
