#pragma once
#ifndef METADATABASEWORKER_H
#define METADATABASEWORKER_H

#include <QBuffer>
#include <QDateTime>
#include <QMutex>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlDriver>
//#include <QtSql/QPSQLDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>
#include <QThread>
#include <QWaitCondition>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <filesystem>

#include "shobjidl.h"
#include "shlguid.h"
//#include "stdafx.h"
#include "strsafe.h"
#include "windows.h"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
    #include <libavutil/opt.h>
    #include <libswscale/swscale.h>
}

//#include <MediaInfoDLL.h>
//#include "MediaSpaces.h"
//#include "MediaViewer.h"
#include "MediaItem.h"
#include "UserSettings.h"

class MetaDatabaseWorker : public QObject
{
    Q_OBJECT

public:

    struct KeyValuePair { // TODO: use QHash?
        QString key;
        QVariant value;
    };

    MetaDatabaseWorker(MediaItem* media_item, const UserSettings* user_settings, int update_filter = MA::Update::Nothing, std::map<QString, QVariant>* updated_user_meta = nullptr, QObject* parent = nullptr);
    ~MetaDatabaseWorker();

    std::filesystem::path getFilePath();
    QString getFileId();
    void cancel();
    bool isActive() const;
    void duplicateFound();
    bool isLargeFile() const;
    bool isHeavyComputationInProgress() const;
    bool isPaused() const;
    
    /// <summary>
    /// Uses the Windows Shell's IShellLink and IPersistFile interfaces to retrieve the path and description from an existing shortcut. 
    /// </summary>
    /// <param name="hwnd"> - A handle to the parent window. The Shell uses this window to display a dialog box if it needs to prompt the user for more information while resolving the link.</param>
    /// <param name="lpszLinkFile"> - Address of a buffer that contains the path of the link, including the file name.</param>
    /// <param name="lpszPath"> - Address of a buffer that receives the path of the link target, including the file name.</param>
    /// <param name="iPathBufferSize"> - Address of a buffer that receives the description of the Shell link, stored in the Comment field of the link properties.</param>
    /// <returns>The result of calling the member functions of the interfaces.</returns>
    static HRESULT resolveWindowsLink(HWND hwnd, LPCSTR lpszLinkFile, LPWSTR lpszPath, int iPathBufferSize);
    QVariant stringToNumber(QString str);
    long long dateTimeStringToSeconds(QString date_time_str);

private:

    QSqlDatabase database;
    QMutex mutex;
    QWaitCondition wait_condition;
    bool paused = false;
    bool heavy_computation_in_progress = false;
    bool active = false;
    bool new_duplicate_file_found = false;
    MediaItem* media_item = nullptr;
    const UserSettings* user_settings;
    int user_id = 0;
    std::filesystem::path file_path;
    QString file_path_str;
    QString file_name;
    std::string file_extension;
    QString file_id = "0";
    size_t file_size = 0;
    time_t file_date_created = 0;
    time_t file_date_modified = 0;
    int frame_count = 0;
    int thumbnail_count = 1;
    QString file_format = "";
    MA::Media::Type media_type = MA::Media::Type::Unknown;
    int media_filter = MA::Media::Type::Unknown;
    int update_filter = MA::Update::Nothing;
    long long video_thumbnail_max_file_size = 20971520; // 20 mb
    const long long non_video_thumbnail_max_file_size = 268435456; // 250 mb, TODO: user setting?
    QByteArray thumbnail_data = 0;
    QString thumbnail_path;
    AVFormatContext* format_context = nullptr;
    AVCodecContext* decoder_context = nullptr;
    int video_stream_index = -1;

    // Note: There are only multiple records for the same file in General (duplicate files), Video/Audio (multiple streams/tracks).
    std::vector< std::unique_ptr<std::map<QString, QVariant>> > general_metadata_maps;
    std::vector< std::unique_ptr<std::map<QString, QVariant>> > video_metadata_maps;
    std::vector< std::unique_ptr<std::map<QString, QVariant>> > audio_metadata_maps;
    std::vector< std::unique_ptr<std::map<QString, QVariant>> > music_metadata_maps; // TODO: Multiple music records for dupe files? Can a change here be made and still be considered a dupe file?
    std::vector< std::unique_ptr<std::map<QString, QVariant>> > image_metadata_maps;
    std::vector< std::unique_ptr<std::map<QString, QVariant>> > extended_metadata_maps;
    
    std::map<QString, QVariant>* user_meta = nullptr;

    bool WaitingLoop();
    bool GenerateFileId(bool use_265_bit_hash = false);
    bool OpenDatabase();
    bool ExistsInDatabase(QString table = Table::General);
    bool DeleteFromDatabase(QString table);
    void SendMetadataToMediaItem();

    MA::Media::Type BuildMetadataMaps();
    QVariant SterilizeValue(std::wstring value);
    KeyValuePair GetProperKeyAndValue(const std::wstring key, const std::wstring value, const MediaInfoDLL::stream_t stream_kind);
    bool PlaceKeyValuePairIntoCorrectMap(KeyValuePair key_value_pair, MediaInfoDLL::stream_t stream_kind, int stream);
    void ChoosePreferredKeys(std::map<QString, QVariant>* metadata_map, QString key, int num = 0, bool delete_other_keys = false);

    void CreateThumbnailImages();
    int LoadInAVContexts(const char* filename, int force_jpeg_codec_for_embedded_stream = -1, bool embedded_file_saved = false);
    AVFrame* DecodeFrame(AVCodecContext* cxt, AVFrame* frame, AVPacket* pkt, int mod);
    QPixmap* FrameToPixmap(const AVFrame* frame) const;
    QImage FrameToImage(const AVFrame* frame) const;

    void AddAdditionalDatabaseFields(std::map<QString, QVariant>* mi_map, QString key, QVariant value, bool overwrite = true);
    void AddAdditionalDatabaseFields(std::vector< std::unique_ptr<std::map<QString, QVariant>> >& metadata_maps, QString key, QVariant value, bool overwrite = true, int stream = -1);
    bool AddToDatabase();
    bool UpdateDatabase();
    bool ExecuteQueryStrings(QList<QString> query_strings);
    QString BuildDatabaseInsertQueryString(std::map<QString, QVariant>* mi_map, const QString table) const;
    QString BuildDatabaseUpdateQueryString(std::map<QString, QVariant>* mi_map, const QString table) const;
    
public slots:

    void addRecord();
    void setPaused(bool enable);
    bool updateRecord();
    void getRecord();

signals:

    void checkForDuplicate(MetaDatabaseWorker* file_check);
    void heavyComputationStarted();
    void heavyComputationEnded();
    void progressMade(int steps);
    void progressFinished(MA::Database::FinishCode code, QString file_name);
    void databaseRecordsRetrieved(std::filesystem::path file_path, std::map<QString, QSqlRecord>* table_record_map);
    void databaseUpdated(QString file_id);
    void mediaItemUpdated(MediaItem* media_item);

};

#endif // METADATABASEWORKER_H