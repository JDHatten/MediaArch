#include "MetaDatabaseWorker.h"

MetaDatabaseWorker::MetaDatabaseWorker(std::filesystem::path file_path, QObject* parent) : QObject(parent)
{
    MetaDatabaseWorker::file_path_str = QString::fromStdString(std::filesystem::path(file_path).make_preferred().string());
    MetaDatabaseWorker::file_path = file_path;
    qDebug() << "MetaDatabaseWorker Constructed for" << file_path_str;
    //general_metadata_maps.reserve(1);
}

void MetaDatabaseWorker::addRecord()
{
    general_metadata_maps.push_back(std::make_unique<std::map<QString, QVariant>>());
    general_metadata_maps.shrink_to_fit();
    int db_code = DatabaseUpdateCode::Failed;

    // Get File Id/Hash
    if (GenerateFileId()) {

        if (OpenDatabase()) {

            // TODO: Does File Id/Hash already exist?
            // TODO: Does File Path already exist? Update?
            if (ExistsInDatabase()) {
                qDebug() << "File Exists in DB.";
                db_code = DatabaseUpdateCode::Existed;
                getRecord();
            }
            else {
                qDebug() << "File Does Not Exist in DB.";
                media_type = BuildMetadataMaps();
                qDebug() << "File Is A " << MediaItem::Type::print(media_type);

                if (AddToDatabase()) {
                    db_code = DatabaseUpdateCode::New;
                    getRecord();
                }
                else {
                    // TODO: Inform user of error and how to possibly fix it.
                }
            }
        }
    }

    // Emit Code (Newly Added, Updated, Already Existed, Failed)
    emit databaseUpdated(db_code);
}

void MetaDatabaseWorker::updateRecord()
{
    // TODO:
}

void MetaDatabaseWorker::getRecord()
{
    std::map<QString, QSqlRecord>* table_record_map = new std::map<QString, QSqlRecord>; // deleted after used in databaseRecordsRetrieved signal in MediaViewer
    //table_record_map = std::make_unique<std::map<QString, QSqlRecord>>();

    if (GenerateFileId()) {

        if (OpenDatabase()) {

            QString select;
            QSqlQuery query(database);
            //QSqlQuery* query = new QSqlQuery(database); // just send over the query?

            for (auto& table : DatabaseSchema::getAllTables(true)) {
                
                select = QString("SELECT * FROM (%1) WHERE (%2='%3')")
                    .arg(table)
                    .arg(DatabaseSchema::Table::General::Field::Id)
                    .arg(file_id);

                query.prepare(select);

                if (query.exec()) {
                    int track_num = 0;
                    while (query.next()) { // if this loops then its a new stream/track
                        QSqlRecord record;
                        record = query.record();

                        if (track_num)
                            table_record_map->emplace(table + (QVariant(track_num).toString()), query.record());
                        else
                            //table_record_map->emplace(table, query.record());
                            table_record_map->insert({ table, record });
                        track_num++;

                        //qDebug() << "record.count:" << query.record().count();
                        //qDebug() << ">" << table << "::" << record.field(DatabaseSchema::Table::General::Field::FileSize) << ":" << record.value(DatabaseSchema::Table::General::Field::FileSize);
                    }
                }
                else {
                    qDebug() << "ERROR: Query Failed:" << query.lastError();
                    qDebug() << "ERROR:" << query.lastQuery();
                }
                query.finish();
            }
        }
    }

    if (table_record_map->size())
        emit databaseRecordsRetrieved(file_path, table_record_map);
}

MetaDatabaseWorker::~MetaDatabaseWorker()
{
    database.close();
    if (fmt_ctx)
        delete fmt_ctx;
    if (dec_ctx)
        delete dec_ctx;
    qDebug() << "...MetaDatabaseWorker Deconstructed.";
}

bool MetaDatabaseWorker::GenerateFileId(bool use_265_bit_hash)
{
    if (file_id == "00")
        return false; // File is empty or not a regular file.
    if (file_id != "0")
        return true; // file_id already generated
    
    //struct stat t_stat;
    struct _stati64 t_stat; // Windows Only https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/stat-functions?view=msvc-170
    int status;
    //stat(file_path.string().data(), &t_stat);
    status = _stati64(file_path.string().data(), &t_stat);

    if (status != 0) {
        qDebug() << "ERROR:" << status << "- Problem getting information";
        switch (errno) {
        case ENOENT:
            qDebug() << "File" << file_path.string() << "not found.";
            break;
        case EINVAL:
            qDebug() << "Invalid parameter to _stat.";
            break;
        default:
            /* Should never be reached. */
            qDebug() << "Unexpected error in _stat.";
        }
    }

    //qDebug() << "file_id: " << std::to_string(t_stat.st_size) << "+" << std::to_string(t_stat.st_mtime);

    //std::string file_id_str = std::to_string(t_stat.st_size) + std::to_string(t_stat.st_mtime);
    file_size = t_stat.st_size;
    file_date_created = t_stat.st_ctime;
    file_date_modified = t_stat.st_mtime;
    file_id = QVariant(file_size).toString() + QVariant(file_date_modified).toString(); //t_stat.st_ctime;

    if (file_size == 0)
        return false; // File is empty or not a regular file.

    qDebug() << "file_id: " << file_id << " | " << file_path.string();
    //qDebug() << "file_id_str: " << file_id_str << " | " << file_path.string();
    //QByteArray file_data = QByteArray(file_id_str.c_str());

    // Note: Hex and upper case used to make it more readable.
    /*if (use_265_bit_hash) {
        file_id = QString(QCryptographicHash::hash(file_data, QCryptographicHash::Sha256).toHex()).toUpper();
        qDebug() << "SHA256 File Hash: " << file_id;
    }
    else {
        file_id = QString(QCryptographicHash::hash(file_data, QCryptographicHash::Md5).toHex()).toUpper();
        qDebug() << "Md5 File Hash: " << file_id;
    }*/

    return true;
}

bool MetaDatabaseWorker::OpenDatabase()
{
    QString dbt_name = QString(DatabaseSchema::Database::Name) + QString("Thread-%1-%2")
        .arg(reinterpret_cast<intptr_t>(QThread::currentThreadId())).arg(rand() % INT16_MAX);
    database = QSqlDatabase::cloneDatabase(DatabaseSchema::Database::ConnectionName, dbt_name);
    return database.open();
}

bool MetaDatabaseWorker::ExistsInDatabase()
{
    QString select = QString("SELECT * FROM (%1) WHERE (%2='%3')")
        .arg(DatabaseSchema::Table::General::Title)
        .arg(DatabaseSchema::Table::General::Field::Id)
        .arg(file_id);
    qDebug() << "ExistsInDatabase():" << select;
    QSqlQuery query(select, database);
    //query.prepare(select);
    //query.bindValue(":file_id", file_id);

    if (not query.exec()) {
        qDebug() << "ERROR: Query Failed:" << query.lastError();
    }
    else {
        if (query.next()) {
            // it exists
            query.finish();
            return true;
        }
        else {
            // TODO: Update if...?
            // TODO: check if file path exist, if so check if other metadata metadata matches (options unlikely to have changed: duration, file size, dimensions)
            // TODO: update id to new id and any other metadata that changed
        }
    }
    query.finish();
    return false;
}

MediaItem::Type::Filter MetaDatabaseWorker::BuildMetadataMaps()
{
    // Using MediaInfoDLL get all metadata from file.
    MediaInfoDLL::MediaInfo media_info_file;
    size_t media_file = media_info_file.Open(file_path);
    //qDebug() << "Summarized Raw Data -> " << media_info_file.Inform();
    if (not media_info_file.IsReady()) {
        return MediaItem::Type::Unknown;
    }
    for (int stream_type = 0; stream_type < MediaInfoDLL::stream_t::Stream_Max; stream_type++) {
        MediaInfoDLL::stream_t stream_kind = static_cast<MediaInfoDLL::stream_t>(stream_type);
        int stream_count = QVariant(QString::fromStdWString(media_info_file.Get(stream_kind, 0, 2, MediaInfoDLL::Info_Text).c_str())).toInt();
        qDebug() << "Stream Kind:" << MEDIA_INFO::print(stream_kind) << "| Streams/Tracks:" << stream_count;
        
        for (int stream = 0; stream < stream_count; stream++) {
            int param_count = QVariant(QString::fromStdWString(media_info_file.Get(stream_kind, stream, 0, MediaInfoDLL::Info_Text).c_str())).toInt();
            int param_alt_index = 0;
            int preferred_param_alt = 0;
            std::wstring last_param;
            
            for (int p = 0; p < param_count; p++) {
                //QString param = QString::fromStdWString(media_info_file.Get(stream_kind, 2, p, MediaInfoDLL::Info_Name).c_str()); // Blank?
                QString info = QString::fromStdWString(media_info_file.Get(stream_kind, stream, p, MediaInfoDLL::Info_Info).c_str());
                std::wstring param = media_info_file.Get(stream_kind, stream, p, MediaInfoDLL::Info_Name_Text);
                std::wstring value = media_info_file.Get(stream_kind, stream, p, MediaInfoDLL::Info_Text);
                QString extra = QString::fromStdWString(media_info_file.Get(stream_kind, stream, p, MediaInfoDLL::Info_Measure_Text).c_str()); // Are the measurements needed or can they just be assumed? Usually storing the lowest unit of measurement.
                bool successfully_placed = false;
                if (value.length()) {
                    qDebug().noquote().nospace() << "[" << MEDIA_INFO::print(stream_kind) << "-" << p << "]" << info << " - " << param << ": \"" << value << "\"" << extra;
                    if (last_param == param) {
                        param_alt_index++;
                    }
                    else {
                        param_alt_index = 0;
                        preferred_param_alt = MEDIA_INFO::getPreferredAlt(param, stream_kind);
                    }
                    if (param_alt_index == preferred_param_alt) {
                        KeyValuePair key_value_pair = GetProperKeyAndValue(param, value, stream_kind);
                        if (key_value_pair.key != "") {
                            //qDebug() << key_value_pair.key << " : " << key_value_pair.value;
                            successfully_placed = PlaceKeyValuePairIntoCorrectMap(key_value_pair, stream_kind, stream);
                            //qDebug() << "PlaceKeyValuePairIntoCorrectMap:" << successfully_placed;
                        }
                    }
                    last_param = param;
                }
            }
        }
    }

    // Move music metadata from general_metadata to music_metadata.
    /*for (auto& field : DatabaseSchema::Table::Music::Field::getAllFields()) {
        if (general_metadata->contains(field)) {
            music_metadata->insert(general_metadata->extract(field));
        }
    }*/

    // Get media type and filter based on metadata gathered.
    if (general_metadata_maps.size())
        media_filters = MediaItem::Type::Other;
    if (video_metadata_maps.size())
        media_filters |= MediaItem::Type::Video;
    if (audio_metadata_maps.size())
        media_filters |= MediaItem::Type::Audio;
    if (music_metadata_maps.size())
        media_filters |= MediaItem::Type::Music;
    if (image_metadata_maps.size())
        media_filters |= MediaItem::Type::Image;
    media_type = MediaItem::Type::getTypeFromFilter(media_filters);


    // TODO: if Other or Image, load file into OpenCV to get more metadata and thumbnail?


    //std::filesystem::perms file_permissions = std::filesystem::status(file_path).permissions();
    //int file_permissions = int(std::filesystem::status(file_path).permissions());
    //bool file_read_only = !int(std::filesystem::perms::owner_write) & file_permissions;

    // Get Other File Attributes
    // https://learn.microsoft.com/en-us/windows/win32/fileio/file-attribute-constants
    DWORD attributes = GetFileAttributes(file_path.wstring().c_str()); // Windows Only
    bool file_read_only = attributes & FILE_ATTRIBUTE_READONLY;
    bool file_hidden = attributes & FILE_ATTRIBUTE_HIDDEN;
    bool file_system = attributes & FILE_ATTRIBUTE_SYSTEM; // operating system file
    bool file_device = attributes & FILE_ATTRIBUTE_DEVICE; // reserved 
    bool file_temp = attributes & FILE_ATTRIBUTE_TEMPORARY; // temporary storage file
    bool file_sparse = attributes & FILE_ATTRIBUTE_SPARSE_FILE; // basically an empty file
    bool file_reparse = attributes & FILE_ATTRIBUTE_REPARSE_POINT; // usually a symbolic link (mounted drive?)
    bool file_compressed = attributes & FILE_ATTRIBUTE_COMPRESSED;
    bool file_encrypted = attributes & FILE_ATTRIBUTE_ENCRYPTED;
    bool file_virtual = attributes & FILE_ATTRIBUTE_VIRTUAL; // reserved 
    bool file_extended = attributes & FILE_ATTRIBUTE_EA; // extended attributes

    // Resolve file links to get their targeted paths (Windows)
    std::filesystem::path target_path;
    if (file_reparse) {
        std::error_code ec;
        target_path = std::filesystem::read_symlink(file_path, ec);
    }
    if (file_path.extension() == ".lnk") {
        TCHAR resolved_path[MAX_PATH] = { 0 };
        resolveWindowsLink(NULL, file_path.string().c_str(), resolved_path, MAX_PATH);

        if (*resolved_path != NULL) {
            target_path = resolved_path;
            if (std::filesystem::exists(target_path)) {
                target_path.make_preferred();
                target_path = std::filesystem::canonical(target_path);
            }
        }
    }
    QString target_path_str = QString::fromStdString(target_path.string());

    // Get thumbnail data and/or paths.
    CreateThumbnailImages();

    // Add key value pairs to metadata maps not included in media info. Only strings? other data types will be added in database insert queries.
    AddAdditionalDatabaseFields(general_metadata_maps, DatabaseSchema::Table::General::Field::Id, file_id);
    //AddAdditionalDatabaseFields(general_metadata_maps.at(0).get(), DatabaseSchema::Table::General::Field::Id, file_id);
    AddAdditionalDatabaseFields(video_metadata_maps, DatabaseSchema::Table::Graphic::Field::Id, file_id);
    AddAdditionalDatabaseFields(audio_metadata_maps, DatabaseSchema::Table::Audio::Field::Id, file_id);
    AddAdditionalDatabaseFields(music_metadata_maps, DatabaseSchema::Table::Music::Field::Id, file_id);
    AddAdditionalDatabaseFields(image_metadata_maps, DatabaseSchema::Table::Music::Field::Id, file_id);
    AddAdditionalDatabaseFields(general_metadata_maps, DatabaseSchema::Table::General::Field::MediaType, media_type);
    AddAdditionalDatabaseFields(general_metadata_maps, DatabaseSchema::Table::General::Field::MediaStreams, media_filters);
    AddAdditionalDatabaseFields(general_metadata_maps, DatabaseSchema::Table::General::Field::FileSize, file_size, false);
    AddAdditionalDatabaseFields(general_metadata_maps, DatabaseSchema::Table::General::Field::FileCreated, file_date_created, false);
    AddAdditionalDatabaseFields(general_metadata_maps, DatabaseSchema::Table::General::Field::FileLastMod, file_date_modified, false);
    AddAdditionalDatabaseFields(general_metadata_maps, DatabaseSchema::Table::General::Field::FileReadOnly, file_read_only);
    AddAdditionalDatabaseFields(general_metadata_maps, DatabaseSchema::Table::General::Field::FileHidden, file_hidden);
    AddAdditionalDatabaseFields(general_metadata_maps, DatabaseSchema::Table::General::Field::FileTargetPath, target_path_str);
    AddAdditionalDatabaseFields(general_metadata_maps, DatabaseSchema::Table::General::Field::ThumbnailData, ":thumbnail");
    AddAdditionalDatabaseFields(general_metadata_maps, DatabaseSchema::Table::General::Field::ThumbnailPath, thumbnail_path);

    /*/ Print Out... GENERAL
    for (const auto& m : general_metadata_maps) {
        for (const auto& p : *m) {
            qDebug() << "G{" << p.first << " => " << p.second << "}";
        }
    }//*/
    /*/ VIDEO
    for (const auto& m : video_metadata_maps) {
        for (auto const& p : *m) {
            qDebug() << "V{" << p.first << " => " << p.second << "}";
        }
    }//*/
    /*/ AUDIO
    for (const auto& m : audio_metadata_maps) {
        for (auto const& p : *m) {
            qDebug() << "A{" << p.first << " => " << p.second << "}";
        }
    }//*/
    /*/ MUSIC
    for (const auto& m : music_metadata_maps) {
        for (auto const& p : *m) {
            qDebug() << "M{" << p.first << " => " << p.second << "}";
        }
    }//*/
    /*/ IMAGE
    for (const auto& m : image_metadata_maps) {
        for (auto const& p : *m) {
            qDebug() << "I{" << p.first << " => " << p.second << "}";
        }
    }//*/

    return media_type;
}

MetaDatabaseWorker::KeyValuePair MetaDatabaseWorker::GetProperKeyAndValue(std::wstring key, std::wstring value, MediaInfoDLL::stream_t stream_kind)
{
    // TODO: Sterilize All Values (Tested: ' " \) (So far only ' needs special escaping)
    intptr_t found_index = -2;
    while (found_index != value.npos) {
        found_index = value.find_first_of(L"'", found_index+2);
        if (found_index != value.npos)
            value.insert(found_index, L"'");
            //value.replace(found_index, 1, L"\""); // Use to test a possible problem char. 
    };
    QString new_key = "";
    QVariant new_val = QString::fromStdWString(value); // TODO: skip converting this to QString, if it's just going to end up a number?

    if (MediaInfoDLL::stream_t::Stream_General == stream_kind) {
        if (key == MEDIA_INFO::GENERAL::ALBUM) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::Album;
        }
        else if (key == MEDIA_INFO::GENERAL::ALBUM_PERFORMER) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::AlbumPerformer;
        }
        else if (key == MEDIA_INFO::GENERAL::ALBUM_REPLAY_GAIN) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::AlbumReplayGain;
        }
        else if (key == MEDIA_INFO::GENERAL::ALBUM_REPLAY_GAIN_PEAK) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::AlbumReplayGainPeak;
        }
        else if (key == MEDIA_INFO::GENERAL::AUDIO_LANGUAGE_LIST) { // Table???
            new_key = DatabaseSchema::Table::General::Field::AudioLanguageList;
        }
        else if (key == MEDIA_INFO::GENERAL::CODEC_ID) {
            new_key = DatabaseSchema::Table::General::Field::CodecId;
        }
        else if (key == MEDIA_INFO::GENERAL::COMMENT) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::Comment;
        }
        else if (key == MEDIA_INFO::GENERAL::COMMON_FORMATS) {
            new_key = DatabaseSchema::Table::General::Field::CommonFormats;
        }
        else if (key == MEDIA_INFO::GENERAL::COMPILATION) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::Compilation;
        }
        else if (key == MEDIA_INFO::GENERAL::COPYRIGHT) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::Copyright;
        }
        /*else if (key == MEDIA_INFO::GENERAL::COVER) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field:;
        }*/
        /*else if (key == MEDIA_INFO::GENERAL::COVER_MIME) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field:;
        }*/
        /*else if (key == MEDIA_INFO::GENERAL::COVER_TYPE) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field:;
        }*/
        else if (key == MEDIA_INFO::GENERAL::DURATION) {
            new_key = DatabaseSchema::Table::General::Field::Duration;
        }
        /*else if (key == MEDIA_INFO::GENERAL::ENCODED_BY) { // Music Table
            new_key = DatabaseSchema::Table::General::Field:;
        }*/
        else if (key == MEDIA_INFO::GENERAL::ENCODED_DATE) {
            new_key = DatabaseSchema::Table::General::Field::EncodedDate;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::GENERAL::FILE_CREATION_DATE) {
            new_key = DatabaseSchema::Table::General::Field::FileCreated;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::GENERAL::FILE_LAST_MOD_DATE) {
            new_key = DatabaseSchema::Table::General::Field::FileLastMod;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::GENERAL::FILE_PATH) {
            new_key = DatabaseSchema::Table::General::Field::FilePath;
            new_val = file_path_str;
        }
        /*else if (key == MEDIA_INFO::GENERAL::FILE_SIZE) { // Added later with a number in bytes
            new_key = DatabaseSchema::Table::General::Field::FileSize;
        }*/
        else if (key == MEDIA_INFO::GENERAL::FORMAT) {
            new_key = DatabaseSchema::Table::General::Field::Format;
        }
        else if (key == MEDIA_INFO::GENERAL::FORMAT_PROFILE) { // Video Table
            new_key = DatabaseSchema::Table::General::Field::FormatInfo;
        }
        else if (key == MEDIA_INFO::GENERAL::FRAME_COUNT) {
            new_key = DatabaseSchema::Table::General::Field::FrameCount;
        }
        else if (key == MEDIA_INFO::GENERAL::FRAME_RATE) {
            new_key = DatabaseSchema::Table::General::Field::FrameRate;
            new_val = new_val.toDouble();
        }
        else if (key == MEDIA_INFO::GENERAL::GENRE) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::Genre;
        }
        else if (key == MEDIA_INFO::GENERAL::GROUPING) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::Grouping;
        }
        else if (key == MEDIA_INFO::GENERAL::ISRC) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::ISRC;
        }
        else if (key == MEDIA_INFO::GENERAL::LABEL) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::Label; // <-------- TODO: label to publisher?
        }
        else if (key == MEDIA_INFO::GENERAL::LYRICIST) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::Lyricist;
        }
        //else if (key == MEDIA_INFO::GENERAL::LYRICS) { // Music Table
        //    new_key = DatabaseSchema::Table::Music::Field:;
        //}
        else if (key == MEDIA_INFO::GENERAL::MIME_TYPE) {
            new_key = DatabaseSchema::Table::General::Field::MimeType;
        }
        else if (key == MEDIA_INFO::GENERAL::OVERALL_BIT_RATE) {
            new_key = DatabaseSchema::Table::General::Field::OverallBitRate;
        }
        else if (key == MEDIA_INFO::GENERAL::OVERALL_BIT_RATE_MODE) {
            new_key = DatabaseSchema::Table::General::Field::OverallBitRateMode;
        }
        else if (key == MEDIA_INFO::GENERAL::OWNR) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::Owner;
        }
        //else if (key == MEDIA_INFO::GENERAL::PART) { // Music Table
        //    new_key = DatabaseSchema::Table::Music::Field:;
        //}
        else if (key == MEDIA_INFO::GENERAL::PERFORMER) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::Performer;
        }
        else if (key == MEDIA_INFO::GENERAL::PERFORMER_URL) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::AudioURL; // 3 in General (Music), Preferred Order: 2
        }
        else if (key == MEDIA_INFO::GENERAL::PUBLISHER) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::Publisher;
        }
        else if (key == MEDIA_INFO::GENERAL::RECORDED_DATE) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::RecordedDate;
            new_val = new_val.toInt(); // This should just be the year, else treat it like DateTime
        }
        else if (key == MEDIA_INFO::GENERAL::RELEASE_COUNTRY) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::ReleaseCountry;
        }
        else if (key == MEDIA_INFO::GENERAL::STREAMABLE) {
            new_key = DatabaseSchema::Table::General::Field::Streamable;
        }
        else if (key == MEDIA_INFO::GENERAL::STREAM_SIZE) {
            new_key = DatabaseSchema::Table::General::Field::StreamSize;
        }
        else if (key == MEDIA_INFO::GENERAL::TAGGED_DATE) {
            new_key = DatabaseSchema::Table::General::Field::TaggedDate;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::GENERAL::TITLE) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::Title;
        }
        else if (key == MEDIA_INFO::GENERAL::TRACK_NAME) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::TrackName;
        }
        else if (key == MEDIA_INFO::GENERAL::TRACK_NUMBER) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::TrackNumber;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::GENERAL::WEBPAGE) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::AudioURL; // 3 in General (Music), Preferred Order: 3
        }
        else if (key == MEDIA_INFO::GENERAL::WEBPAGE_SOURCE) { // Music Table
            new_key = DatabaseSchema::Table::Music::Field::AudioURL; // 3 in General (Music), Preferred Order: 4
        }
        /*else if (key == MEDIA_INFO::GENERAL::WRITING_LIBRARY) { // Music or Audio Table?
            new_key = DatabaseSchema::Table::Audio::Field::WritingLibrary; // Not including, this should show up in Video or Audio anyways
        }*/
    }

    if (MediaInfoDLL::stream_t::Stream_Video == stream_kind) {
        /*else if (key == MEDIA_INFO::VIDEO::BITS_PER_FRAME) { // This is just math using other metadata
            new_key = DatabaseSchema::Table::Graphic::Field:;
        }*/
        if (key == MEDIA_INFO::VIDEO::BIT_DEPTH) {
            new_key = DatabaseSchema::Table::Graphic::Field::BitDepth;
        }
        else if (key == MEDIA_INFO::VIDEO::BIT_RATE) {
            new_key = DatabaseSchema::Table::Graphic::Field::BitRate;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::VIDEO::CHROMA_SUBSAMPLING) {
            new_key = DatabaseSchema::Table::Graphic::Field::ChromaSubsampling;
        }
        else if (key == MEDIA_INFO::VIDEO::CODEC_ID) {
            new_key = DatabaseSchema::Table::Graphic::Field::CodecId;
        }
        else if (key == MEDIA_INFO::VIDEO::CODEC_ID_INFO) {
            new_key = DatabaseSchema::Table::Graphic::Field::CodecIdInfo;
        }
        /*else if (key == MEDIA_INFO::VIDEO::CODEX_CONFIG_BOX) {
            new_key = DatabaseSchema::Table::Graphic::Field:;
        }*/
        else if (key == MEDIA_INFO::VIDEO::COLOR_PRIMARIES) {
            new_key = DatabaseSchema::Table::Graphic::Field::ColorPrimaries;
        }
        else if (key == MEDIA_INFO::VIDEO::COLOR_RANGE) {
            new_key = DatabaseSchema::Table::Graphic::Field::ColorRange;
        }
        else if (key == MEDIA_INFO::VIDEO::COLOR_SPACE) {
            new_key = DatabaseSchema::Table::Graphic::Field::ColorSpace;
        }
        else if (key == MEDIA_INFO::VIDEO::DISPLAY_ASPECT_RATIO) {
            new_key = DatabaseSchema::Table::Graphic::Field::AspectRatio;
        }
        else if (key == MEDIA_INFO::VIDEO::DURATION) {
            new_key = DatabaseSchema::Table::Graphic::Field::Duration;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::VIDEO::ENCODED_DATE) {
            new_key = DatabaseSchema::Table::Graphic::Field::EncodedDate;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::VIDEO::ENCODING_SETTINGS) {
            new_key = DatabaseSchema::Table::Graphic::Field::EncodingSettings;
        }
        else if (key == MEDIA_INFO::VIDEO::FORMAT) {
            new_key = DatabaseSchema::Table::Graphic::Field::Format;
        }
        else if (key == MEDIA_INFO::VIDEO::FORMAT_INFO) {
            new_key = DatabaseSchema::Table::Graphic::Field::FormatInfo;
        }
        else if (key == MEDIA_INFO::VIDEO::FORMAT_PROFILE) {
            new_key = DatabaseSchema::Table::Graphic::Field::FormatProfile;
        }
        else if (key == MEDIA_INFO::VIDEO::FORMAT_SETTINGS) {
            new_key = DatabaseSchema::Table::Graphic::Field::FormatSettings;
        }
        else if (key == MEDIA_INFO::VIDEO::FORMAT_URL) {
            new_key = DatabaseSchema::Table::Graphic::Field::FormatURL;
        }
        else if (key == MEDIA_INFO::VIDEO::FRAME_COUNT) {
            new_key = DatabaseSchema::Table::Graphic::Field::FrameCount;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::VIDEO::FRAME_RATE) {
            new_key = DatabaseSchema::Table::Graphic::Field::FrameRate;
            new_val = new_val.toDouble();
        }
        else if (key == MEDIA_INFO::VIDEO::FRAME_RATE_MODE) {
            new_key = DatabaseSchema::Table::Graphic::Field::FrameRateMode;
        }
        else if (key == MEDIA_INFO::VIDEO::HEIGHT) {
            new_key = DatabaseSchema::Table::Graphic::Field::Height;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::VIDEO::ID) {
            new_key = DatabaseSchema::Table::Graphic::Field::TrackId;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::VIDEO::LANGUAGE) {
            new_key = DatabaseSchema::Table::Graphic::Field::Language;
        }
        else if (key == MEDIA_INFO::VIDEO::MATRIX_COEFFICIENTS) {
            new_key = DatabaseSchema::Table::Graphic::Field::MatrixCoefficients;
        }
        else if (key == MEDIA_INFO::VIDEO::ROTATION) {
            new_key = DatabaseSchema::Table::Graphic::Field::Rotation;
            new_val = new_val.toDouble();
        }
        else if (key == MEDIA_INFO::VIDEO::SCAN_TYPE) {
            new_key = DatabaseSchema::Table::Graphic::Field::ScanType;
        }
        else if (key == MEDIA_INFO::VIDEO::STREAM_SIZE) {
            new_key = DatabaseSchema::Table::Graphic::Field::StreamSize;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::VIDEO::TAGGED_DATE) {
            new_key = DatabaseSchema::Table::Graphic::Field::TaggedDate;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::VIDEO::TITLE) {
            new_key = DatabaseSchema::Table::Graphic::Field::Title;
        }
        else if (key == MEDIA_INFO::VIDEO::WIDTH) {
            new_key = DatabaseSchema::Table::Graphic::Field::Width;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::VIDEO::WRITING_LIBRARY) {
            new_key = DatabaseSchema::Table::Graphic::Field::WritingLibrary;
        }
    }

    if (MediaInfoDLL::stream_t::Stream_Audio == stream_kind) {
        /*if (key == MEDIA_INFO::AUDIO::ALTERNATE_GROUP) { // Track Id??
            new_key = DatabaseSchema::Table::Audio::Field:;
        }*/
        if (key == MEDIA_INFO::AUDIO::BIT_RATE) {
            new_key = DatabaseSchema::Table::Audio::Field::BitRate;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::AUDIO::BIT_RATE_MODE) {
            new_key = DatabaseSchema::Table::Audio::Field::BitRateMode;
        }
        else if (key == MEDIA_INFO::AUDIO::CHANNELS) {
            new_key = DatabaseSchema::Table::Audio::Field::Channels;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::AUDIO::CHANNELS_LAYOUT) {
            new_key = DatabaseSchema::Table::Audio::Field::ChannelLayout;
        }
        else if (key == MEDIA_INFO::AUDIO::CHANNEL_POSITIONS) {
            new_key = DatabaseSchema::Table::Audio::Field::ChannelPositions;
        }
        else if (key == MEDIA_INFO::AUDIO::CODEC_ID) {
            new_key = DatabaseSchema::Table::Audio::Field::CodecId;
        }
        else if (key == MEDIA_INFO::AUDIO::COMPRESSION_MODE) {
            new_key = DatabaseSchema::Table::Audio::Field::CompressionMode;
        }
        //else if (key == MEDIA_INFO::AUDIO::DEFAULT) {                          // TODO: Is this default track? "TrackId" field?
        //    new_key = DatabaseSchema::Table::Audio::Field:;
        //}
        else if (key == MEDIA_INFO::AUDIO::DURATION) {
            new_key = DatabaseSchema::Table::Audio::Field::Duration;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::AUDIO::ENCODED_DATE) {
            new_key = DatabaseSchema::Table::Audio::Field::EncodedDate;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::AUDIO::FORMAT) {
            new_key = DatabaseSchema::Table::Audio::Field::Format;
        }
        else if (key == MEDIA_INFO::AUDIO::FORMAT_INFO) {
            new_key = DatabaseSchema::Table::Audio::Field::FormatInfo;
        }
        else if (key == MEDIA_INFO::AUDIO::FORMAT_PROFILE) {
            new_key = DatabaseSchema::Table::Audio::Field::FormatProfile;
        }
        else if (key == MEDIA_INFO::AUDIO::FORMAT_SETTINGS) {
            new_key = DatabaseSchema::Table::Audio::Field::FormatSettings;
        }
        /*else if (key == MEDIA_INFO::AUDIO::FORMAT_VERSION) {
            new_key = DatabaseSchema::Table::Audio::Field:;
        }*/
        else if (key == MEDIA_INFO::AUDIO::FRAME_COUNT) {
            new_key = DatabaseSchema::Table::Audio::Field::FrameCount;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::AUDIO::FRAME_RATE) {
            new_key = DatabaseSchema::Table::Audio::Field::FrameRate;
            //new_val = stringToNumber(new_val.toString());
            new_val = new_val.toDouble();
        }
        else if (key == MEDIA_INFO::AUDIO::ID) {
            new_key = DatabaseSchema::Table::Audio::Field::TrackId;
            new_val = new_val.toDouble();
        }
        else if (key == MEDIA_INFO::AUDIO::LANGUAGE) {
            new_key = DatabaseSchema::Table::Audio::Field::Language;
        }
        else if (key == MEDIA_INFO::AUDIO::MAX_BIT_RATE) {
            new_key = DatabaseSchema::Table::Audio::Field::BitRate;
        }
        /*else if (key == MEDIA_INFO::AUDIO::MD5) {
            new_key = DatabaseSchema::Table::Audio::Field:; // Id?
        }*/
        else if (key == MEDIA_INFO::AUDIO::REPLAY_GAIN) {
            new_key = DatabaseSchema::Table::Audio::Field::ReplayGain;
            new_val = new_val.toDouble();
        }
        else if (key == MEDIA_INFO::AUDIO::REPLAY_GAIN_PEAK) {
            new_key = DatabaseSchema::Table::Audio::Field::ReplayGainPeak;
            new_val = new_val.toDouble();
        }
        else if (key == MEDIA_INFO::AUDIO::SAMPLING_COUNT) {
            new_key = DatabaseSchema::Table::Audio::Field::SamplingCount;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::AUDIO::SAMPLING_RATE) {
            new_key = DatabaseSchema::Table::Audio::Field::SamplingRate;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::AUDIO::STREAM_SIZE) {
            new_key = DatabaseSchema::Table::Audio::Field::StreamSize;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::AUDIO::TAGGED_DATE) {
            new_key = DatabaseSchema::Table::Audio::Field::TaggedDate;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::AUDIO::TITLE) {
            new_key = DatabaseSchema::Table::Audio::Field::Title;
        }
        else if (key == MEDIA_INFO::AUDIO::WRITING_LIBRARY) {
            new_key = DatabaseSchema::Table::Audio::Field::WritingLibrary;
        }
    }

    if (MediaInfoDLL::stream_t::Stream_Image == stream_kind) {
        if (key == MEDIA_INFO::IMAGE::BIT_DEPTH) {
            new_key = DatabaseSchema::Table::Graphic::Field::BitDepth;
        }
        else if (key == MEDIA_INFO::IMAGE::CHROMA_SUBSAMPLING) {
            new_key = DatabaseSchema::Table::Graphic::Field::ChromaSubsampling;
        }
        else if (key == MEDIA_INFO::IMAGE::COLOR_SPACE) {
            new_key = DatabaseSchema::Table::Graphic::Field::ColorSpace;
        }
        else if (key == MEDIA_INFO::IMAGE::COLOR_SPACE_ICC) {
            new_key = DatabaseSchema::Table::Graphic::Field::ColorSpaceICC;
        }
        else if (key == MEDIA_INFO::IMAGE::COMPRESSION_MODE) {
            new_key = DatabaseSchema::Table::Graphic::Field::CompressionMode;
        }
        else if (key == MEDIA_INFO::IMAGE::FORMAT) {
            new_key = DatabaseSchema::Table::Graphic::Field::Format;
        }
        else if (key == MEDIA_INFO::IMAGE::HEIGHT) {
            new_key = DatabaseSchema::Table::Graphic::Field::Height;
        }
        else if (key == MEDIA_INFO::IMAGE::STREAM_SIZE) {
            new_key = DatabaseSchema::Table::Graphic::Field::StreamSize;
        }
        else if (key == MEDIA_INFO::IMAGE::WIDTH) {
            new_key = DatabaseSchema::Table::Graphic::Field::Width;
        }
    }

    // TODO: if (MediaInfoDLL::stream_t::Stream_Text == stream_kind) {

    if (new_key == "") {
        qDebug() << "WARNING: No proper key (database field) found for" << key << " |  Type:" << MEDIA_INFO::print(stream_kind);
    }

    return KeyValuePair(new_key, new_val);
}

bool MetaDatabaseWorker::PlaceKeyValuePairIntoCorrectMap(KeyValuePair key_value_pair, MediaInfoDLL::stream_t stream_kind, int stream)
{
    bool success = true;
    if (MediaInfoDLL::Stream_General == stream_kind) {
        if (DatabaseSchema::Table::Music::Field::doesFieldExist(key_value_pair.key)) {
            // Moving: General -> Music
            if (stream >= music_metadata_maps.size()) {
                music_metadata_maps.push_back(std::make_unique<std::map<QString, QVariant>>());
                return PlaceKeyValuePairIntoCorrectMap(key_value_pair, stream_kind, stream);
            }
            music_metadata_maps.at(stream)->emplace(key_value_pair.key, key_value_pair.value);
        }
        else {
            if (stream >= general_metadata_maps.size()) {
                qWarning() << "WARNING: Multiple" << MEDIA_INFO::print(stream_kind) << "streams found, this should never happen.";
                //qWarning().nospace() << "WARNING: More streams than anticipated for the stream type " << MEDIA_INFO::print(stream_kind) << ", create more metadata maps!";
                return false;
                //general_metadata_maps.push_back(std::make_unique<std::map<QString, QVariant>>());// std::move?
                //return PlaceKeyValuePairIntoCorrectMap(key_value_pair, stream_kind, stream);
            }
            general_metadata_maps.at(stream)->emplace(key_value_pair.key, key_value_pair.value);
        }
    }
    else if (MediaInfoDLL::Stream_Video == stream_kind) {
        if (stream >= video_metadata_maps.size()) {
            video_metadata_maps.push_back(std::make_unique<std::map<QString, QVariant>>());
            return PlaceKeyValuePairIntoCorrectMap(key_value_pair, stream_kind, stream);
        }
        video_metadata_maps.at(stream)->emplace(key_value_pair.key, key_value_pair.value);
    }
    else if (MediaInfoDLL::Stream_Audio == stream_kind) {
        if (stream >= audio_metadata_maps.size()) {
            audio_metadata_maps.push_back(std::make_unique<std::map<QString, QVariant>>());
            return PlaceKeyValuePairIntoCorrectMap(key_value_pair, stream_kind, stream);
        }
        audio_metadata_maps.at(stream)->emplace(key_value_pair.key, key_value_pair.value);
    }
    else if (MediaInfoDLL::Stream_Image == stream_kind) {
        if (stream >= image_metadata_maps.size()) {
            image_metadata_maps.push_back(std::make_unique<std::map<QString, QVariant>>());
            return PlaceKeyValuePairIntoCorrectMap(key_value_pair, stream_kind, stream);
        }
        image_metadata_maps.at(stream)->emplace(key_value_pair.key, key_value_pair.value);
    }
    //else if (MediaInfoDLL::Stream_Text == stream_kind) {}
    else {
        success = false;
    }
    return success;
}

void MetaDatabaseWorker::ChoosePreferredKeys(std::map<QString, QVariant>* metadata_map, QString key, int num, bool delete_other_keys)
{
    if (num) {
        key.removeLast().append(QVariant(num).toString());
    }
    if (metadata_map->contains(key)) {
        if (delete_other_keys) {
            metadata_map->erase(key);
            ChoosePreferredKeys(metadata_map, key, num++, delete_other_keys);
        }
        else if (metadata_map->at(key) == "") {
            metadata_map->erase(key);
            if (num == 0)
                key.append("0"); // Note: Prevent a num from being removed when its not there by adding a 0
            ChoosePreferredKeys(metadata_map, key, num++);
        }
        else {
            if (num) {
                auto updated_pair = metadata_map->extract(key);
                updated_pair.key() = key.removeLast();
                metadata_map->insert(std::move(updated_pair));
                ChoosePreferredKeys(metadata_map, key, num++, delete_other_keys);
            }
            else {
                if (num == 0)
                    key.append("0");
                ChoosePreferredKeys(metadata_map, key, num++, delete_other_keys);
            }
        }
    }
}

HRESULT MetaDatabaseWorker::resolveWindowsLink(HWND hwnd, LPCSTR lpszLinkFile, LPWSTR lpszPath, int iPathBufferSize)
{
    HRESULT hres;
    IShellLink* psl;
    WCHAR szGotPath[MAX_PATH];
    WCHAR szDescription[MAX_PATH];
    WIN32_FIND_DATA wfd;
    *lpszPath = 0; // Assume failure 

    // Get a pointer to the IShellLink interface. It is assumed that CoInitialize has already been called. 
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hres)) {
        IPersistFile* ppf;
        // Get a pointer to the IPersistFile interface. 
        hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
        if (SUCCEEDED(hres)) {
            WCHAR wsz[MAX_PATH];
            // Ensure that the string is Unicode. 
            MultiByteToWideChar(CP_ACP, 0, lpszLinkFile, -1, wsz, MAX_PATH);

            // Add code here to check return value from MultiByteWideChar for success.

            // Load the shortcut. 
            hres = ppf->Load(wsz, STGM_READ);
            if (SUCCEEDED(hres)) {
                // Resolve the link. 
                hres = psl->Resolve(hwnd, 0);
                if (SUCCEEDED(hres)) {
                    // Get the path to the link target. 
                    hres = psl->GetPath(szGotPath, MAX_PATH, (WIN32_FIND_DATA*)&wfd, SLGP_SHORTPATH);
                    if (SUCCEEDED(hres)) {
                        // Get the description of the target. 
                        hres = psl->GetDescription(szDescription, MAX_PATH);
                        if (SUCCEEDED(hres)) {
                            hres = StringCbCopy(lpszPath, iPathBufferSize, szGotPath);
                            if (SUCCEEDED(hres)) {
                                // Handle success
                            }
                            else {
                                // Handle the error
                            }
                        }
                    }
                }
            }
            // Release the pointer to the IPersistFile interface. 
            ppf->Release();
        }
        // Release the pointer to the IShellLink interface. 
        psl->Release();
    }
    return hres;
}

void MetaDatabaseWorker::CreateThumbnailImages()
{
    if (media_filters & (MediaItem::Type::Video | MediaItem::Type::Image | MediaItem::Type::Music)) {
    
        //int duration;
        int frame_count;
        int thumbnail_count;
        int frame_interval;
        int skip_frames_above;

        if (media_filters & MediaItem::Type::Video) {
            // TODO: Backup/Alt: do some math and get maximum frames in video, use it to set frame_interval.
            //       If frames not in metadata, get duration and frame rate. else guess based on file size.  duration / 30 FPS?
            //       Even if FPS is wrong it should get close to the right number of thumbnails spread across the video.
            //duration = general_metadata_maps.at(0)->at(DatabaseSchema::Table::General::Field::Duration).toInt();
            frame_count = general_metadata_maps.at(0)->at(DatabaseSchema::Table::Graphic::Field::FrameCount).toInt();
            thumbnail_count = 8; // TODO: user setting
            frame_interval = frame_count / (thumbnail_count + 1);
            skip_frames_above = frame_count - frame_interval;

            //qDebug() << "duration:" << duration << " - frame_count:" << frame_count << " - frame_interval:" << frame_interval;
        }
        else {
            frame_interval = 1;
            skip_frames_above = 2;
        }

        int return_value = 0;
        AVPacket* packet;
        AVFrame* frame;
        AVFrame* rgb_frame;
        //std::vector<QPixmap> thumbnail_images;
        //QVector<QImage> thumbnail_images;
        QVector<QPixmap> thumbnail_images;

        frame = av_frame_alloc();
        packet = av_packet_alloc();
        if (!frame || !packet) {
            qDebug() << "Could not allocate frame or packet";
        }
        else {
            // 
            if ((return_value = LoadInAVContexts(file_path.string().c_str())) > -1) {
                    while (av_read_frame(fmt_ctx, packet) >= 0) {
                        if (packet->stream_index == video_stream_index) {

                            // Skip last thumbnail, else it will be a frame near the end of video.
                            if (dec_ctx->frame_num < skip_frames_above) {

                                rgb_frame = DecodeFrame(dec_ctx, frame, packet, frame_interval);
                                if (rgb_frame) {
                                    //QImage thumbnail_image = FrameToImage(rgb_frame);
                                    //thumbnail_images.push_back(thumbnail_image);
                                    //thumbnail_pixmap->convertFromImage(thumbnail_image);

                                    QPixmap* thumbnail_pixmap = FrameToPixmap(rgb_frame);
                                    thumbnail_images.push_back(*thumbnail_pixmap);
                                    av_frame_free(&rgb_frame);
                                    delete thumbnail_pixmap;
                                }
                            }
                            else {
                                av_packet_unref(packet);
                                break;
                            }
                        }
                        av_packet_unref(packet);
                    }
            }
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&fmt_ctx);
        }
        //else { // If file doesn't open...

        av_frame_free(&frame);
        av_packet_free(&packet);

        if (return_value < 0 and return_value != AVERROR_EOF) {
            qWarning() << "ERROR: AV Loading Error:" << return_value;
        }
        if (thumbnail_images.size()) {
            //thumbnail_data = QByteArray::fromRawData(reinterpret_cast<const char*>(thumbnail_images.data()), thumbnail_images.size());
            QDataStream out(&thumbnail_data, QIODevice::WriteOnly);
            out << thumbnail_images;
            thumbnail_data = qCompress(thumbnail_data, 9); // TODO: test different levels of compression, for both speed and quality... -1 to 9 no effect? DB is same size
            // TODO: compressing doesn't seem to decrease size in DB, maybe try using a different image format if size becomes a problem 
            // Saved JPEG images should be about 6 kb each, but when saving 8 images (pixmap) to the DB it's adding an extra ~340kb
            thumbnail_images.clear();
        }
        else {
            thumbnail_data = 0;
        }
    }

    if (media_type & MediaItem::Type::Image) {
        thumbnail_path = QString::fromStdString(file_path.string());
    }
    // TODO: Always look for image on disk to use for thumbnail or only when none created/found from file?  user setting?  thumbnail_data.isEmpty() 
    else if (media_type > MediaItem::Type::Other) {
        // Search in the file's current and parent directory for files named: folder, cover, front, etc and extensions jpg, png, etc.
        std::filesystem::path thumbnail_link;
        for (auto const& dir_entry : std::filesystem::recursive_directory_iterator{ file_path.parent_path(), std::filesystem::directory_options::skip_permission_denied }) {
            std::string stem = dir_entry.path().stem().string();
            std::string ext = dir_entry.path().extension().string();
            if (stem == "cover" or stem == "folder" or stem == "front" or stem == "front-cover" or stem == "embedded" or stem == file_path.stem().string()) {
                if (ext == ".jpg" or ext == ".jpeg" or ext == ".png" or ext == ".bpm") {
                    thumbnail_link = dir_entry.path();
                    thumbnail_link.make_preferred();
                    break;
                }
            }
        }
        if (not std::filesystem::exists(thumbnail_link)) {
            for (auto const& dir_entry : std::filesystem::directory_iterator{ file_path.parent_path().parent_path(), std::filesystem::directory_options::skip_permission_denied }) {
                std::string stem = dir_entry.path().stem().string();
                std::string ext = dir_entry.path().extension().string();
                if (stem == "cover" or stem == "folder" or stem == "front" or stem == "front-cover") {
                    if (ext == ".jpg" or ext == ".jpeg" or ext == ".png" or ext == ".bpm") {
                        thumbnail_link = dir_entry.path();
                        thumbnail_link.make_preferred();
                        break;
                    }
                }
            }
        }
        if (std::filesystem::exists(thumbnail_link)) {
            qDebug() << "Found Music Thumbnail:" << thumbnail_link.string();
            thumbnail_path = QString::fromStdString(thumbnail_link.string());
        }
    }
}

int MetaDatabaseWorker::LoadInAVContexts(const char* file_path_char, int force_jpeg_codec_for_embedded_stream, bool embedded_file_saved)
{
    int return_value;
    const AVCodec* dec;

    fmt_ctx = avformat_alloc_context();

    if ((return_value = avformat_open_input(&fmt_ctx, file_path_char, NULL, NULL)) < 0) {
        qDebug() << "ERROR: failed to open input file";
        return return_value;
    }

    // Force jpeg codec if missing context.
    if (force_jpeg_codec_for_embedded_stream > -1) {
        fmt_ctx->streams[force_jpeg_codec_for_embedded_stream]->codecpar->codec_id = AV_CODEC_ID_MJPEG;
    }

    if ((return_value = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        qDebug() << "ERROR: failed to find stream information";
        return return_value;
    }
    
    /*// Alt video stream finder
    for (int i = 0; i < fmt_ctx->nb_streams; i++) {

        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            // Find the decoder for the video stream
            dec = avcodec_find_decoder(fmt_ctx->streams[i]->codecpar->codec_id); //AV_CODEC_ID_PNG  AV_CODEC_ID_MJPEG
            if (dec == NULL) {
                fprintf(stderr, "Unsupported codec!\n");
                return -1; // Codec not found
            }
            return_value = i;
            break;
        }
    }//*/

    return_value = av_find_best_stream(fmt_ctx, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (return_value < 0) {
        qDebug() << "ERROR: failed to find a video stream in the input file";
        return return_value;
    }
    video_stream_index = return_value;

    // If no useful context found for an image, try forcing jpeg codec. (sometimes PNG images are embedded, but are actually Jpegs)
    if (force_jpeg_codec_for_embedded_stream == -1 and fmt_ctx->streams[video_stream_index]->codecpar->format == -1)
        return LoadInAVContexts(file_path_char, video_stream_index);

    /*/ Alt or back up option: Not enough useful context to modify image, saving image to disk to be loaded back in.
    if (embedded_file_saved and fmt_ctx->streams[video_stream_index]->codecpar->format == -1) {
        qDebug() << "Last resort, keep image on disk and load from image file when needed for thumbnail.";
        return -1;
    }
    if (embedded_file_saved) {
        std::filesystem::path embed_file_path = (file_path.parent_path() / "embedded.jpg").make_preferred();
        std::filesystem::remove(embed_file_path);
    }
    else if (fmt_ctx->streams[video_stream_index]->codecpar->format == -1) {
        if (fmt_ctx->streams[video_stream_index]->attached_pic.size) {

            std::filesystem::path embed_file_path = (file_path.parent_path() / "embedded.jpg").make_preferred();
            std::string embed_file_path_str = embed_file_path.string();
            const char* embed_file_path_c = embed_file_path_str.c_str();

            if (not std::filesystem::exists(embed_file_path)) {
                AVPacket packet = fmt_ctx->streams[video_stream_index]->attached_pic;
                FILE* image_file = fopen(embed_file_path_c, "wb");
                int result = fwrite(packet.data, packet.size, 1, image_file);
                fclose(image_file);
            }
            return LoadInAVContexts(embed_file_path_c, -2, true);
        }
        return -1;
    }//*/

    /*/ Packet Save Test
    AVPacket* packet;
    packet = av_packet_alloc();
    av_read_frame(fmt_ctx, packet);
    FILE* image_file = fopen("embedded.jpg", "wb");
    int result = fwrite(packet->data, packet->size, 1, image_file);
    fclose(image_file);//*/
    
    dec_ctx = avcodec_alloc_context3(dec);
    if (!dec_ctx) {
        qDebug() << "ERROR: failed to allocate decoding context/memory";
        return AVERROR(ENOMEM);
    }

    avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[video_stream_index]->codecpar);

    // Initiate the video decoder
    if ((return_value = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
        qDebug() << "ERROR: failed to open video decoder";
        return return_value;
    }

    return 0;
}

AVFrame* MetaDatabaseWorker::DecodeFrame(AVCodecContext* codec_ctx, AVFrame* frame, AVPacket* packet, int mod)
{
    // Send packet to decoder
    int return_value = avcodec_send_packet(codec_ctx, packet);
    if (return_value < 0) {
        qDebug() << "ERROR: error sending packet for decoding";
        switch (errno) {
        case AVERROR(EAGAIN):
            qDebug() << "Input is not accepted in the current state - user must read output with avcodec_receive_frame() (once all output is read, the packet should be resent, and the call will not fail with EAGAIN).";
            break;
        case AVERROR_EOF:
            qDebug() << "The decoder has been flushed, and no new packets can be sent to it (also returned if more than 1 flush packet is sent).";
            break;
        case AVERROR(EINVAL):
            qDebug() << "Codec not opened, it is an encoder, or requires flush.";
            break;
        case AVERROR(ENOMEM):
            qDebug() << "Failed to add packet to internal queue, or similar.";
            break;
        default:
            qDebug() << "Legitimate decoding errors:" << return_value;
        }
        return nullptr;
    }

    //TODO: get user setting "media_item_size"
    int media_item_size = 256;

    // Scale image keeping aspect ratio
    int scaled_width = codec_ctx->width;
    int scaled_height = codec_ctx->height;
    if (scaled_width > scaled_height) {
        scaled_height = scaled_height / (scaled_width / media_item_size);
        scaled_width = media_item_size;
    }
    else if (scaled_height > scaled_width) {
        scaled_width = scaled_width / (scaled_height / media_item_size);
        scaled_height = media_item_size;
    }
    else {
        scaled_width = media_item_size;
        scaled_height = media_item_size;
    }

    // Get frame back from decoder - (put this back if problems arise later)
    return_value = avcodec_receive_frame(codec_ctx, frame);

    // If resource temporaily not available or end of file reached, error out.
    if (return_value == AVERROR(EAGAIN) || return_value == AVERROR_EOF) {
        return nullptr; // Can still go again if more frames available
    }
    else if (return_value < 0) {
        qWarning() << "ERROR: Frame decoding failure, failed to receive frame.";
        return nullptr;
    }

    AVPixelFormat src_format = (AVPixelFormat)frame->format;
    AVPixelFormat dst_format = AV_PIX_FMT_RGB24;

    // Create a scalar context for conversion
    //SwsContext* sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt, scaled_width, scaled_height, dst_format, SWS_BICUBIC, NULL, NULL, NULL);
    SwsContext* sws_ctx = sws_getContext(frame->width, frame->height, src_format, scaled_width, scaled_height, dst_format, SWS_BICUBIC, NULL, NULL, NULL);

    if (sws_ctx == nullptr) {
        qWarning() << "ERROR: Missing image context, scaling failed.";
        return nullptr;
    }

    // Create a new RGB frame for conversion
    AVFrame* rgb_frame = av_frame_alloc();
    rgb_frame->format = dst_format;
    /////rgb_frame->format = AV_PIX_FMT_RGB24;
    //rgb_frame->format = codec_ctx->pix_fmt;
    //rgb_frame->format = AV_PIX_FMT_RGB32;
    //rgb_frame->format = AV_PIX_FMT_YUV420P;
    //rgb_frame->format = AV_PIX_FMT_YUV440P;
    //rgb_frame->format = AV_PIX_FMT_YUV444P;
    //rgb_frame->format = AV_PIX_FMT_BGRA;
    //rgb_frame->format = AV_PIX_FMT_BGR24;
    rgb_frame->width = scaled_width;
    rgb_frame->height = scaled_height;
    
    // Allocate a new buffer for the RGB conversion frame
    av_frame_get_buffer(rgb_frame, 0);

    /*/ Alt: prepare line sizes structure  and char buffer in array... as sws_scale expects
    int rgb_linesizes[8] = { 0 };
    rgb_linesizes[0] = 3 * frame->width;
    unsigned char* rgb_data[8];
    int image_byte_size = 3 * frame->height * frame->width;
    rgb_data[0] = (unsigned char*)malloc(image_byte_size + 64); // alloc extra 64 bytes
    if (!rgb_data[0]) {
        qDebug() << "Error allocating buffer for frame conversion";
        free(rgb_data[0]);
        sws_freeContext(sws_ctx);
        return nullptr;
    }//*/

    // Run while frames are available with no errors
    if (return_value >= 0) {
        
        /*/ Get frame back from decoder
        return_value = avcodec_receive_frame(codec_ctx, frame); // put this back if problems arise later
        
        // If resource temporaily not available or end of file reached, error out.
        if (return_value == AVERROR(EAGAIN) || return_value == AVERROR_EOF) {
            return nullptr; // Can still go again if more frames available
        }
        else if (return_value < 0) {
            qWarning() << "ERROR: Frame decoding failure, failed to receive frame.";
            return nullptr;
        }//*/

        // Get only the (mod)'th frame...
        if (codec_ctx->frame_num % mod == 0) {
            qDebug().nospace() << "Decoding frame #" << codec_ctx->frame_num;

            // Scale/Convert the old frame to the new RGB frame
            sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, rgb_frame->data, rgb_frame->linesize);
            
            /*/ Alt
            sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, rgb_data, rgb_linesizes);
            QImage image(scaled_width, scaled_height, QImage::Format::Format_RGB888); // AV_PIX_FMT_YUVJ444P to AV_PIX_FMT_RGB24
            for (size_t y = 0; y < scaled_height; y++) {
                memcpy(image.scanLine(y), rgb_data[0] + y * rgb_linesizes[0], 3 * scaled_width);
            }
            image.save("test.jpg");//*/

            /*/ Frame Test Save
            FILE* pFile;
            char szFilename[32];
            //sprintf(szFilename, "frame%d.ppm", 1);
            sprintf(szFilename, "frame%d.jpg", 1);
            pFile = fopen(szFilename, "wb");
            if (pFile == NULL)
                return nullptr;
            // Write header
            fprintf(pFile, "P6\n%d %d\n255\n", rgb_frame->width, rgb_frame->height);
            // Write pixel data
            for (int y = 0; y < rgb_frame->height; y++)
                fwrite(rgb_frame->data[0] + y * rgb_frame->linesize[0], 1, rgb_frame->width * 3, pFile);
            fclose(pFile);//*/

            return rgb_frame;
        }
    }
    return nullptr;
}

QPixmap* MetaDatabaseWorker::FrameToPixmap(const AVFrame* frame) const
{
    QImage image(frame->width, frame->height, QImage::Format::Format_RGB888); // Format_RGB888 - AV_PIX_FMT_YUVJ444P to AV_PIX_FMT_RGB24
    size_t pix_multi = 3; // 3-24, 4-32, 5-64
    for (size_t y = 0; y < frame->height; y++) {
        memcpy(image.scanLine(y), frame->data[0] + y * frame->linesize[0], pix_multi * frame->width);
        //memcpy(image.scanLine(y), frame->data[0] + y * pix_multi * frame->width, pix_multi * frame->width);
    }
    //image.save("test.jpg");
    QPixmap* pixmap = new QPixmap();
    pixmap->convertFromImage(image);

    return pixmap;
}
// Alt methods
QImage MetaDatabaseWorker::FrameToImage(const AVFrame* frame) const
{
    // Convert the input AVFrame to the desired format
    SwsContext* img_convert_ctx = sws_getContext(
        frame->width,
        frame->height,
        (AVPixelFormat)frame->format,
        //(AVPixelFormat)dec_ctx->pix_fmt,
        frame->width,
        frame->height,
        AV_PIX_FMT_RGB24,
        //AV_PIX_FMT_YUV420P,
        SWS_BICUBIC, NULL, NULL, NULL);

    if (!img_convert_ctx) {
        qDebug() << "Failed to create sws context";
        return QImage();
    }

    // Prepare line sizes structure as sws_scale expects
    int rgb_linesizes[8] = { 0 };
    rgb_linesizes[0] = 3 * frame->width;

    // Prepare char buffer in array, as sws_scale expects
    unsigned char* rgb_data[8];
    int image_byte_size = 3 * frame->height * frame->width;
    // Alloc extra 64 bytes
    rgb_data[0] = (unsigned char*)malloc(image_byte_size + 64);

    if (!rgb_data[0]) {
        qDebug() << "Error allocating buffer for frame conversion";
        free(rgb_data[0]);
        sws_freeContext(img_convert_ctx);
        return QImage();
    }

    //QImage image(frame->width, frame->height, QImage::Format_RGB888);
    //QVariant img_bpl = image.bytesPerLine();
    //int bpl[8] = { img_bpl.toInt() };
    //unsigned char* data[8] = { image.bits() };

    if (sws_scale(img_convert_ctx, frame->data, frame->linesize, 0, frame->height, rgb_data, rgb_linesizes) != frame->height) {
        //if (sws_scale(img_convert_ctx, frame->data, frame->linesize, 0,  frame->height, data, bpl) != frame->height) {
        qDebug() << "Error changing frame color range";
        free(rgb_data[0]);
        sws_freeContext(img_convert_ctx);
        return QImage();
    }
    //*/
    // then create QImage and copy converted frame data into it

    //QImage image(frame->data[0], dec_ctx->width, dec_ctx->height, frame->linesize[0], QImage::Format_RGB888);
    QImage image(frame->width, frame->height, QImage::Format::Format_RGB888);

    for (int y = 0; y < frame->height; y++) {
        memcpy(image.scanLine(y), rgb_data[0] + y * 3 * frame->width, 3 * frame->width);
        //memcpy(image.scanLine(y), frame->data[0] + y * 3 * frame->width, 3 * frame->width);
        //memcpy(image.scanLine(y), data[0] + y * 3 * frame->width, 3 * frame->width);
    }

    //image.save("test.jpg");

    free(rgb_data[0]);
    sws_freeContext(img_convert_ctx);
    return image;
}

void MetaDatabaseWorker::AddAdditionalDatabaseFields(std::map<QString, QVariant>* metadata_map, QString key, QVariant value, bool overwrite)
{
    if (metadata_map and not metadata_map->empty() and value.isValid())
        if (metadata_map->contains(key)) {
            if (overwrite)
                metadata_map->at(key) = value;
            else
                qDebug() << "[Not Overwritten] Key:" << key << " - Value:" << metadata_map->at(key) << " - New Value:" << value;
        }
        else
            metadata_map->insert({ key, value });
    else
        qDebug() << "Empty metadata_map, not adding:" << key;
}
void MetaDatabaseWorker::AddAdditionalDatabaseFields(std::vector< std::unique_ptr<std::map<QString, QVariant>> >& metadata_maps, QString key, QVariant value, bool overwrite, int stream)
{
    if (stream > -1)
        AddAdditionalDatabaseFields(metadata_maps.at(stream).get(), key, value, overwrite);
    else
        for (auto& metadata_map : metadata_maps)
            AddAdditionalDatabaseFields(metadata_map.get(), key, value, overwrite);
}

QString MetaDatabaseWorker::BuildDatabaseInsertQueryString(std::map<QString, QVariant>* metadata_map, QString table)
{
    QString insert;
    QString values;
    if (metadata_map and metadata_map->size()) {
        insert = QString("INSERT INTO %1 (").arg(table);
        values = "VALUES (";
        for (auto& metadata : *metadata_map) {
            insert.append(QString("\"%1\", ").arg(metadata.first));
            qDebug() << metadata.second;
            if (metadata.second.typeId() == QMetaType::QString) {
                if (metadata.second.toString().length() and metadata.second.toString().at(0) == QString(":"))
                    values.append(QString("%1, ").arg(metadata.second.toString())); // "variable" placeholder, value added later
                else
                    values.append(QString("'%1', ").arg(metadata.second.toString())); // string
            }
            else //if (metadata.second.typeId() == QMetaType::Int or metadata.second.typeId() == QMetaType::Double)
                values.append(QString("%1, ").arg(metadata.second.toString())); // number or some other kind of data
        }
        insert.removeLast().removeLast().append(") ");
        values.removeLast().removeLast().append(")");
        //qDebug() << insert + values;
    }
    return insert + values;
}

bool MetaDatabaseWorker::AddToDatabase()
{
    bool success = true;
    QList<QString> metadata_query_strings;

    for (auto& map : general_metadata_maps) {
        metadata_query_strings.append(BuildDatabaseInsertQueryString(map.get(), DatabaseSchema::Tables::General));
    }
    for (auto& map : video_metadata_maps) {
        metadata_query_strings.append(BuildDatabaseInsertQueryString(map.get(), DatabaseSchema::Tables::Graphic));
    }
    for (auto& map : audio_metadata_maps) {
        metadata_query_strings.append(BuildDatabaseInsertQueryString(map.get(), DatabaseSchema::Tables::Audio));
    }
    for (auto& map : music_metadata_maps) {
        metadata_query_strings.append(BuildDatabaseInsertQueryString(map.get(), DatabaseSchema::Tables::Music));
    }
    for (auto& map : image_metadata_maps) {
        metadata_query_strings.append(BuildDatabaseInsertQueryString(map.get(), DatabaseSchema::Tables::Graphic));
    }
    bool general = true;
    QSqlQuery query(database);
    for (auto& mqs : metadata_query_strings) {
        //qDebug() << mqs;
        query.prepare(mqs);
        if (general) {
            query.bindValue(":thumbnail", thumbnail_data);
            query.bindValue(":file_read_only", 0);
            query.bindValue(":file_hidden", 0);
            general = false;
        }

        if (query.exec()) {
            qDebug() << "New Values added successfully to DB for file:"<< file_path.filename().string() << " | Query:" << query.lastQuery();
            //success = true;
        }
        else {
            qDebug() << "ERROR: Failed To add Values from file:" << file_path.filename().string() << " | " << query.lastError();
            qDebug() << "ERROR: The Failed Query:" << query.lastQuery();
            success = false;
        }
        query.clear();
        query.finish();
    }

    return success;
}

// Not Needed, Delete?
QVariant MetaDatabaseWorker::stringToNumber(QString str)
{
    QString str_val;
    QVariant value;
    bool interger = true;
    for (auto& c : str) {
        if (c.isDigit())
            str_val.append(c);
        else if (interger and c == QString(".")) {
            str_val.append(c);
            interger = false;
        }
    }
    if (interger)
        value = str_val.toInt();
    else
        value = str_val.toDouble();

    //qDebug() << str << "<stringToNumber>" << value;

    return value;
}

long long MetaDatabaseWorker::dateTimeStringToSeconds(QString date_time_string)
{
    QString date_time_str = date_time_string.removeLast().removeLast().removeLast().removeLast();
    QDateTime date_time;
    if (date_time_str.length() > 19)
        date_time = QDateTime::fromString(date_time_str, "yyyy-MM-dd HH:mm:ss.zzz");
    else
        date_time = QDateTime::fromString(date_time_str, "yyyy-MM-dd HH:mm:ss");

    //qDebug() << date_time_str << "<dateTimeStringToSeconds>" << date_time.toSecsSinceEpoch();

    return date_time.toSecsSinceEpoch();
}
