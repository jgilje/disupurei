#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QObject>

#include <functional>

#include <QDir>
#include <QUrl>
#include <QTimer>
#include <QVector>
#include <QtNetwork/QNetworkAccessManager>

#include <QJsonObject>

struct Entry;
class Playlist : public QObject
{
    Q_OBJECT
public:
    enum class Type {
        IMAGE, VIDEO
    };
    Q_ENUM(Type)

    explicit Playlist(QObject *parent = 0);
    const Entry& next();

    void macAddress(const QString& address);
    void url(const QString& url);

signals:
    void playlistAvailable();

public slots:
    void refreshMetadata();
    void checkForCachedMetadata();

private:
    QString _sequenceId;
    qint64 _published = -1;
    QTimer _metadataRefreshTimer;
    QDir _cachePath;
    QDir _entryPath;
    QNetworkAccessManager _nam;
    QVector<Entry> _entries;
    QVector<Entry> _refreshEntries;
    QVector<Entry>::Iterator _refreshIterator;
    QVector<Entry>::Iterator _playbackIterator;
    QString _mac;
    QString _url;

    void cleanupStaleEntries();
    void downloadEntries();

    void parseMetadata();
    void parseMetadataEntries(QJsonArray entries);
    QJsonObject openJsonFile(QFile& sourceFile);
private slots:
    void onRefreshFinished();
    void onFetchEntryFinished();
};

struct Entry {
    QString fileId;
    QString filePath;
    Playlist::Type type;
    int durationMillis;
    QUrl url;
    bool loaded = false;
};

#endif // PLAYLIST_H
