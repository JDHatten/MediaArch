#pragma once
#ifndef METADATABASEWORKER_H
#define METADATABASEWORKER_H

#include <QBuffer>
#include <QDateTime>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlDriver>
//#include <QtSql/QPSQLDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>
#include <QThread>

//#include <qfile>
//#include <qcryptographichash.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <filesystem>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
    #include <libavutil/opt.h>
    #include <libswscale/swscale.h>
}

#include <MediaInfoDLL.h>
//#include "MediaSpaces.h"
#include "MediaViewer.h"

class MetaDatabaseWorker : public QObject
{
    Q_OBJECT

public:

    static enum DatabaseUpdateCode {
        Failed, New, Updated, Existed
    };
    struct KeyValuePair {
        QString key;
        QVariant value;
    };

    MetaDatabaseWorker(std::filesystem::path file_path, QObject* parent = nullptr);
    ~MetaDatabaseWorker();
    QVariant stringToNumber(QString str);
    long long dateTimeStringToSeconds(QString date_time_str);

private:

    QSqlDatabase database;
    std::filesystem::path file_path;
    QString file_path_str;
    QString file_id = "0";
    size_t file_size = 0;
    time_t file_date_created = 0;
    time_t file_date_modified = 0;
    int media_type = MediaItem::Type::Other;

    QByteArray thumbnail_data;
    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* dec_ctx = nullptr;
    int video_stream_index = -1;

    std::vector< std::unique_ptr<std::map<QString, QVariant>> > general_metadata_maps; // Note: There should never be multiple General streams/tracks.
    std::vector< std::unique_ptr<std::map<QString, QVariant>> > video_metadata_maps;
    std::vector< std::unique_ptr<std::map<QString, QVariant>> > audio_metadata_maps;
    std::vector< std::unique_ptr<std::map<QString, QVariant>> > music_metadata_maps;
    std::vector< std::unique_ptr<std::map<QString, QVariant>> > image_metadata_maps;

    bool GenerateFileId(bool use_265_bit_hash = false);
    bool OpenDatabase();
    bool ExistsInDatabase();

    int BuildMetadataMaps();
    KeyValuePair GetProperKeyAndValue(const std::wstring key, const std::wstring value, const MediaInfoDLL::stream_t stream_kind);
    bool PlaceKeyValuePairIntoCorrectMap(KeyValuePair key_value_pair, MediaInfoDLL::stream_t stream_kind, int stream);
    void ChoosePreferredKeys(std::map<QString, QVariant>* metadata_map, QString key, int num = 0, bool delete_other_keys = false);

    void CreateThumbnailImages();
    int LoadInAVContexts(const char* filename);
    AVFrame* DecodeFrame(AVCodecContext* cxt, AVFrame* frame, AVPacket* pkt, int mod);
    QPixmap* FrameToPixmap(const AVFrame* frame) const;
    QImage FrameToImage(const AVFrame* frame) const;

    void AddAdditionalDatabaseFields(std::map<QString, QVariant>* mi_map, QString key, QVariant value, bool overwrite = true);
    void AddAdditionalDatabaseFields(std::vector< std::unique_ptr<std::map<QString, QVariant>> >& metadata_maps, QString key, QVariant value, bool overwrite = true, int stream = -1);
    bool AddToDatabase();
    QString BuildDatabaseInsertQueryString(std::map<QString, QVariant>* mi_map, const QString table);

public slots:

    void addRecord();
    void updateRecord();
    void getRecord();

signals:

    void databaseUpdated(int code);
    void databaseRecordsRetrieved(std::filesystem::path file_path, std::map<QString, QSqlRecord>* table_record_map);
    void databaseRecordRetrieved(QSqlRecord record);

};

#endif // METADATABASEWORKER_H