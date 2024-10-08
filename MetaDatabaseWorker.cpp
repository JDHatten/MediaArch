#include "MetaDatabaseWorker.h"

MetaDatabaseWorker::MetaDatabaseWorker(MediaItem* media_item, const UserSettings* user_settings, int update_filter, std::map<QString, QVariant>* updated_user_meta, QObject* parent)
    : QObject(parent), media_item(media_item), user_settings(user_settings), update_filter(update_filter), user_meta(updated_user_meta)
{
    MetaDatabaseWorker::file_path = media_item->getFilePath();
    MetaDatabaseWorker::file_path_str = QString::fromStdString(file_path.make_preferred().string());
    file_name = file_path.filename().string().c_str();
    file_extension = file_path.extension().string();
    std::transform(file_extension.begin(), file_extension.end(), file_extension.begin(), tolower);
    user_id = user_settings->getUserSetting(Field::Settings::UserId).toInt();
    video_thumbnail_max_file_size = user_settings->getUserSetting(Field::Settings::AutoGenerateVideoSizeLimit).toLongLong() * 1024 * 1024;
    thumbnail_count = user_settings->getUserSetting(Field::Settings::VideoThumbnailCount).toInt();
    //qDebug().nospace() << "MetaDatabaseWorker Constructed (" << file_path_str << ")";
}

MetaDatabaseWorker::~MetaDatabaseWorker()
{
    database.close();
    if (format_context)
        delete format_context;
    if (decoder_context)
        delete decoder_context;
    //qDebug().nospace() << "...MetaDatabaseWorker Deconstructed (" << file_path.filename().string() << ")";
}

std::filesystem::path MetaDatabaseWorker::getFilePath()
{
    return file_path;
}

QString MetaDatabaseWorker::getFileId()
{
    return file_id;
}

void MetaDatabaseWorker::cancel()
{
    active = false;
}
bool MetaDatabaseWorker::isActive() const
{
    return active;
}

void MetaDatabaseWorker::duplicateFound()
{
    setPaused(true);
    new_duplicate_file_found = true;
    // Delete any progress made in maps
    general_metadata_maps.clear();
    extended_metadata_maps.clear();
}

bool MetaDatabaseWorker::isLargeFile() const
{
    if (media_type & MA::Media::Type::Video and file_size > 52428800) // 50 mb, video files under this size should be processed with in few seconds at most.
        return true;
    if (media_type & (MA::Media::Type::Music | MA::Media::Type::Audio | MA::Media::Type::Image) and file_size > 10485760) // 10 mb
        return true;
    else
        return false;
}

bool MetaDatabaseWorker::isHeavyComputationInProgress() const
{
    return heavy_computation_in_progress;
}

bool MetaDatabaseWorker::isPaused() const
{
    return paused;
}
void MetaDatabaseWorker::setPaused(bool enable)
{
    mutex.lock();
    paused = enable;
    if (not paused)
        wait_condition.wakeAll();
    mutex.unlock();
}

void MetaDatabaseWorker::addRecord()
{
    active = true;
    general_metadata_maps.push_back(std::make_unique<std::map<QString, QVariant>>());
    extended_metadata_maps.push_back(std::make_unique<std::map<QString, QVariant>>());
    //general_metadata_maps.shrink_to_fit();
    MA::Database::FinishCode db_code = MA::Database::FinishCode::Failed;

    // Get File Id/Hash
    if (GenerateFileId() and active) {

        if (OpenDatabase() and active) {

            // First bit of progress made (all progress: 1 + 8[BuildMetadataMaps] + thumbnail_count(8)[CreateThumbnailImages] + 1[AddToDatabase] + 1[SendMetadataToMediaItem] + 1[progressFinished] = 20)
            emit progressMade(1);

            if (ExistsInDatabase() and active) {

                if ((MA::Update::Metadata | MA::Update::SmallFileThumbnails | MA::Update::AllFileThumbnails) & update_filter or user_meta) {

                    if (MA::Update::Metadata & update_filter) {
                        media_type = BuildMetadataMaps();
                    }
                    else {
                        ExistsInDatabase(Table::Extended); // Makes sure to at least set media_filter, media_type
                    }

                    if (MA::Update::AllFileThumbnails & update_filter) {
                        CreateThumbnailImages();
                    }
                    else if (MA::Update::SmallFileThumbnails & update_filter) {

                        if (media_type & MA::Media::Type::Video and file_size < video_thumbnail_max_file_size)
                            CreateThumbnailImages();

                        else if (media_type != MA::Media::Type::Video and file_size < non_video_thumbnail_max_file_size)
                            CreateThumbnailImages();
                    }

                    if (new_duplicate_file_found) { // Note: True if a newly found dupe file is not in database yet, but the original is (Only on first load of dupe).
                        
                        emit progressMade(thumbnail_count); // Skipping CreateThumbnailImages  (TODO: Is there anyway to know ahead of time what will be skipped and therefore not add unneeded progress steps?)

                        if (AddToDatabase()) {
                            db_code = MA::Database::FinishCode::NewDuplicateAdded;
                            SendMetadataToMediaItem();
                        }
                    }
                    else {
                        if (UpdateDatabase()) {
                            db_code = MA::Database::FinishCode::Updated;
                            SendMetadataToMediaItem();
                        }
                    }
                }
                else {
                    emit progressMade(8); // Skipping BuildMetadataMaps +...
                    db_code = MA::Database::FinishCode::AlreadyExisted;
                    SendMetadataToMediaItem();
                    emit progressMade(thumbnail_count + 1); // Skipping ...CreateThumbnailImages + UpdateDatabase
                }
            }
            else if (active) {

                // Check if this "file_id" is currently being added to the database (duplicate files found in the same directory)
                emit checkForDuplicate(this);
                
                if (new_duplicate_file_found) { // TODO: will this always catch the next dupe file in time?
                    WaitingLoop(); // Wait until the original file is added to the database (databaseUpdated signal)...
                    emit progressMade(-1);
                    addRecord(); // Then re-add file as an existing/duplicate file
                    return;
                }

                media_type = BuildMetadataMaps();

                if (media_type & MA::Media::Type::Video and file_size < video_thumbnail_max_file_size)
                    CreateThumbnailImages();

                else if (media_type != MA::Media::Type::Video and file_size < non_video_thumbnail_max_file_size)
                    CreateThumbnailImages();
                else
                    emit progressMade(thumbnail_count); // Skipping CreateThumbnailImages

                if (AddToDatabase()) {
                    db_code = MA::Database::FinishCode::NewlyAdded;
                    SendMetadataToMediaItem();
                }
            }
        }
        else
            emit progressMade(8 + thumbnail_count + 3); // Skipping All
    }
    else
        emit progressMade(8 + thumbnail_count + 3); // Skipping All

    if (not active)
        db_code = MA::Database::FinishCode::Canceled;

    // Emit Code (Newly Added, Updated, Already Existed, Failed, New Duplicate Added)
    emit progressFinished(db_code, file_name);
}

bool MetaDatabaseWorker::WaitingLoop()
{
    while (1) {
        mutex.lock();
        if (paused) {
            wait_condition.wait(&mutex);
        }
        else {
            mutex.unlock();
            return paused;
        }
        mutex.unlock();
    }
    return paused;
}

bool MetaDatabaseWorker::updateRecord()
{
    // not used
    return false;
}

void MetaDatabaseWorker::getRecord()
{
    std::map<QString, QSqlRecord>* table_record_map = new std::map<QString, QSqlRecord>; // This is deleted after databaseRecordsRetrieved signal emitted in MediaViewer
    //table_record_map = std::make_unique<std::map<QString, QSqlRecord>>();

    if (GenerateFileId()) {

        if (OpenDatabase()) {

            QString select;
            QSqlQuery query(database);
            //QSqlQuery* query = new QSqlQuery(database); // just send over the query?

            for (auto& table : DatabaseSchema::getAllTables(true)) {
                
                select = QString("SELECT * FROM \"%1\" WHERE \"%2\" = \"%3\"")
                    .arg(table).arg(Field::General::Id).arg(file_id);
                if (table == Table::UserMeta) {
                    select.append(QString(" AND \"%1\" = %2")
                        .arg(Field::UserMeta::UserId).arg(user_id));
                }
                query.prepare(select);

                if (query.exec()) {
                    int track_num = 0;
                    while (query.next()) { // if this loops then its a new stream/track/dupe
                        QString table_str = table;
                        QSqlRecord record = query.record();

                        if (track_num)
                            table_str.append(QVariant(track_num).toString());
                        
                        table_record_map->emplace(table_str, record);
                        track_num++;
                        /*/
                        for (int i = 0; i < record.count(); i++) {
                            if (record.fieldName(i) == Field::Extended::ThumbnailData) continue;
                            qDebug().nospace() << ">>" << table_str << " (" << record.count() << ") :: " << record.fieldName(i) << " : " << record.value(i);
                        }//*/
                    }
                }
                else {
                    qWarning() << "ERROR: Query Failed:" << query.lastError();
                    qWarning() << "ERROR:" << query.lastQuery();
                }
                query.finish();
            }
        }
    }

    if (table_record_map->size())
        emit databaseRecordsRetrieved(file_path, table_record_map);
}

void MetaDatabaseWorker::SendMetadataToMediaItem()
{
    if (GenerateFileId()) {

        if (OpenDatabase()) {

            QString select;
            QSqlQuery query(database);
            bool update_only_user_meta = (update_filter == MA::Update::Nothing and user_meta);

            mutex.lock();

            for (auto& table : DatabaseSchema::getAllTables(true)) {

                if (not active) return;

                if (update_only_user_meta and table != Table::UserMeta)
                    continue; // Only update media_item with user meta, skip the rest.

                select = QString("SELECT * FROM \"%1\" WHERE \"%2\" = \"%3\"")
                    .arg(table).arg(Field::General::Id).arg(file_id);
                if (table == Table::UserMeta) {
                    select.append(QString(" AND \"%1\" = %2")
                        .arg(Field::UserMeta::UserId).arg(user_id));
                }
                query.prepare(select);

                if (query.exec()) {
                    int stream_index = 0;
                    while (query.next()) { // if this loops then its a new stream/track/dupe

                        if (not active) return;

                        //QString table_str = table;
                        QSqlRecord record = query.record();

                        MA::Media::Type graphic_type = MA::Media::Type::Unknown;
                        if (table == Table::Graphic)
                            graphic_type = MA::Media::Type(record.value(Field::Graphic::Type).toInt());

                        // TODO: get video/audio default stream, if possible. Or use "user meta" option so the chosen default is shown first / in labels / etc.

                        // Only send the correct General metadata and not the duplicates
                        // TODO: Or send all and only show the "setDefaultStream", then something can be done like "show duplicate files", etc.
                        if (table == Table::General) {
                            //qDebug() << record.value(Field::General::FilePath).toString() << "=/=" << file_path_str;
                            if (record.value(Field::General::FilePath).toString() == file_path_str)
                                stream_index = 0; // media_item->setDefaultStream(table, stream_index);
                            else {
                                // Only first duplicate will get thumbnail data, so use the same data for each duplicate and skip other metadata. TODO: ThumbnailPath?
                                /*QVariant thumbnail_data = record.value(Field::Extended::ThumbnailData);
                                if (thumbnail_data.isValid()) {
                                    media_item->updateThumbnail(thumbnail_data.toByteArray());
                                }*/
                                continue;
                            }
                        }
                        for (QString field : DatabaseSchema::getAllFieldsFor(table)) {

                            if (not active) return;

                            QVariant value = record.value(field);
                            if (value.isValid() and not value.isNull() and value.toString().length()) { // ignores "" but allows 0

                                // Intercept thumbnail data/path
                                if (field == Field::Extended::ThumbnailData) {
                                    media_item->updateThumbnail(value.toByteArray());
                                }
                                else if (field == Field::Extended::ThumbnailPath) {
                                    media_item->updateThumbnail(value.toString()); // Only if not already set via ThumbnailData
                                }
                                else {
                                    media_item->addMetadata(table, field, value, stream_index, graphic_type);
                                    //qDebug() << property_name << ":" << value;
                                }
                            }
                        }
                        stream_index++;
                        /*/
                        for (int i = 0; i < record.count(); i++) {
                            if (record.fieldName(i) == Field::Extended::ThumbnailData) continue;
                            qDebug().nospace() << ">>" << table << " (" << record.count() << ") :: " << record.fieldName(i) << " : " << record.value(i);
                        }//*/
                    }
                }
                else {
                    qWarning() << "ERROR: Query Failed:" << query.lastError();
                    qWarning() << "ERROR:" << query.lastQuery();
                }
                query.finish();
            }
            mutex.unlock();
        }
    }
    emit progressMade(1);
    emit mediaItemUpdated(media_item);
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
    if (database.open())
        return true;
    QString dbt_name = QString(DatabaseSchema::Database::Name) + QString("Thread-%1-%2")
        .arg(reinterpret_cast<intptr_t>(QThread::currentThreadId())).arg(rand() % INT16_MAX);
    database = QSqlDatabase::cloneDatabase(DatabaseSchema::Database::ConnectionName, dbt_name);
    return database.open();
}

bool MetaDatabaseWorker::ExistsInDatabase(QString table) // Note: This also sets (General) frame_count, file_format, (Extended) media_filter, media_type, 
{
    bool exists = false;
    //
    QString select = QString("SELECT * FROM \"%1\" WHERE \"%2\" = \"%3\"")
        .arg(table)
        .arg(Field::General::Id)
        .arg(file_id);//*/

    /*/ Are "joins" not possible?
    QString select = QString("SELECT * FROM \"%1\" WHERE \"%3\" = \"%4\" UNION SELECT * FROM \"%2\" WHERE \"%3\" = \"%4\"")
    //QString select = QString("SELECT * FROM \"%1\" INNER JOIN \"%2\" ON \"%3\" = \"%3\" WHERE \"%3\" = \"%4\"")
    //QString select = QString("SELECT * FROM \"%1\" INNER JOIN \"%2\" ON \"%1.%3\" = \"%2.%3\" WHERE \"%3\" = \"%4\"")
        .arg(table)
        .arg(Table::Extended)
        .arg(Field::General::Id)
        .arg(file_id);//*/
    /*SELECT ProductID, ProductName, CategoryName
        FROM Products
        INNER JOIN Categories ON Products.CategoryID = Categories.CategoryID;
        
        SELECT * FROM \"General\" INNER JOIN \"Extended\" ON \"General.Id\" = \"Extended.Id\" WHERE \"Id\" = \"7283961575709687\"
        SELECT * FROM \"General\" INNER JOIN \"Extended\" ON   General.Id   =   Extended.Id   WHERE \"Id\" = \"7283961575709687\"
        SELECT * FROM \"General\" INNER JOIN \"Extended\" ON \"Id\"         = \"Id\"          WHERE \"Id\" = \"1105801712033444\"


        SELECT City, Country FROM Customers
        WHERE Country='Germany'
        UNION
        SELECT City, Country FROM Suppliers
        WHERE Country='Germany'
        ORDER BY City;

        SELECT * FROM \"General\" WHERE \"Id\" = \"1105801712033444\" UNION SELECT * FROM \"Extended\" WHERE \"Id\" = \"1105801712033444\"

        */

    QSqlQuery query(select, database);

    if (query.exec()) {
        if (table == Table::General) {

            while (query.next()) {
                exists = true;

                if (frame_count == 0
                    and query.record().contains(Field::Graphic::FrameCount)
                    and query.record().value(Field::Graphic::FrameCount).isValid()) // Note: these are from General, not Graphic. TODO: Get from Graphic Table? Does it matter?
                        frame_count = query.record().value(Field::Graphic::FrameCount).toInt();
                if (file_format.isEmpty()
                    and query.record().contains(Field::General::Format)
                    and query.record().value(Field::General::Format).isValid())
                        file_format = query.record().value(Field::Graphic::Format).toString();

                if (query.record().value(Field::General::FilePath).toString() == file_path_str) {
                    new_duplicate_file_found = false;
                    break;
                }
                else {
                //if (query.record().value(Field::General::FilePath).toString() != file_path_str) {
                    // TODO: Update other tables, or skip? user setting?
                    new_duplicate_file_found = true;
                }
            }
        }
        else if (table == Table::Extended) {
            if (query.next()) {
                exists = true;
                media_filter = MA::Media::Type(query.record().value(Field::Extended::MediaStreams).toInt());
                media_type = MA::Media::Type(query.record().value(Field::Extended::MediaType).toInt());
            }
        }
        else { // Note: This is used to check if other tables (param) with this file's ID exist.
            while (query.next()) {
                exists = true;
            }
        }
    }
    else {
        qWarning() << "ERROR: Failed Exists In Database Query:" << query.lastError();
        qWarning() << "ERROR: The Failed Query:" << query.lastQuery();
    }

    // Will make a new (only) general record for any duplicate file via an update. (If update_metadata already set, don't change it)
    if ((MA::Update::Metadata & update_filter) == 0 and new_duplicate_file_found)
        update_filter |= MA::Update::Metadata;

    query.finish();
    return exists;
}

bool MetaDatabaseWorker::DeleteFromDatabase(QString table) // Currently unused
{
    bool deleted = false;
    QString delete_record = QString("DELETE FROM \"%1\" WHERE \"%2\" = \"%3\"")
        .arg(table)
        .arg(Field::General::Id)
        .arg(file_id);
    QSqlQuery query(delete_record, database);

    if (query.exec()) {
        qDebug().noquote().nospace() << "Records Deleted From " << table << ": " << query.numRowsAffected();
        deleted = query.numRowsAffected();
    }
    else {
        qWarning() << "ERROR: Failed To Delete Record From Database:" << query.lastError();
        qWarning() << "ERROR: The Failed Query:" << query.lastQuery();
    }
    return deleted;
}

MA::Media::Type MetaDatabaseWorker::BuildMetadataMaps()
{
    // Using MediaInfoDLL get all metadata from file.
    MediaInfoDLL::MediaInfo media_info_file;
    size_t media_file = media_info_file.Open(file_path);
    //qDebug() << "Summarized Raw Data -> " << media_info_file.Inform();
    if (not media_info_file.IsReady()) {
        emit progressMade(8);
        return MA::Media::Type::Unknown;
    }
    for (int stream_type = 0; stream_type < MediaInfoDLL::stream_t::Stream_Max; stream_type++) {
        MediaInfoDLL::stream_t stream_kind = static_cast<MediaInfoDLL::stream_t>(stream_type);
        int stream_count = QVariant(QString::fromStdWString(media_info_file.Get(stream_kind, 0, 2, MediaInfoDLL::Info_Text).c_str())).toInt();
        //qDebug() << "Stream Kind:" << MEDIA_INFO::print(stream_kind) << "| Streams/Tracks:" << stream_count;
        
        if (new_duplicate_file_found and stream_kind != MediaInfoDLL::stream_t::Stream_General) {
            emit progressMade(1);
            continue;
        }
        for (int stream = 0; stream < stream_count; stream++) {
            int param_count = QVariant(QString::fromStdWString(media_info_file.Get(stream_kind, stream, 0, MediaInfoDLL::Info_Text).c_str())).toInt();
            int param_alt_index = 0;
            int preferred_param_alt = 0;
            std::wstring last_param;

            // Add "additional database field" that distinguishes between video and image streams/types in Graphic's table.
            if (stream_kind == MediaInfoDLL::stream_t::Stream_Video or stream_kind == MediaInfoDLL::stream_t::Stream_Image) {
                MA::Media::Type stream_type = MA::Media::fromMediaInfoStreamKind(stream_kind);
                PlaceKeyValuePairIntoCorrectMap({ Field::Graphic::Type, stream_type }, stream_kind, stream);
            }
            
            for (int p = 0; p < param_count; p++) {
                //QString param = QString::fromStdWString(media_info_file.Get(stream_kind, 2, p, MediaInfoDLL::Info_Name).c_str()); // Blank?
                QString info = QString::fromStdWString(media_info_file.Get(stream_kind, stream, p, MediaInfoDLL::Info_Info).c_str());
                std::wstring param = media_info_file.Get(stream_kind, stream, p, MediaInfoDLL::Info_Name_Text);
                std::wstring value = media_info_file.Get(stream_kind, stream, p, MediaInfoDLL::Info_Text);
                QString extra = QString::fromStdWString(media_info_file.Get(stream_kind, stream, p, MediaInfoDLL::Info_Measure_Text).c_str()); // Are the measurements needed or can they just be assumed? Usually storing the lowest unit of measurement.
                bool successfully_placed = false;
                if (value.length()) {
                    //qDebug().noquote().nospace() << "[" << MEDIA_INFO::print(stream_kind) << "-" << p << "]" << info << " - " << param << ": \"" << value << "\"" << extra;
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
        emit progressMade(1);
    }



    // TODO: WebP, SVG, GIF, (Others?) need work.  
    // WebP = get metadata
    // SVG = text file, read code to get metadata, width / height, format: SVG / XML, Colors?, not much else 
    // GIF = Special Thumbnail: get path and play/stop movie on hover



    // Get media type and filter based on metadata gathered. 
    //if (media_filter == MA::Media::Type::Unknown) { // Skips on new_duplicate_file_found
    if (not new_duplicate_file_found) {
        if (general_metadata_maps.size())
            media_filter = MA::Media::Type::Other;
        if (video_metadata_maps.size())
            media_filter |= MA::Media::Type::Video;
        if (audio_metadata_maps.size())
            media_filter |= MA::Media::Type::Audio;
        if (music_metadata_maps.size())
            media_filter |= MA::Media::Type::Music;
        if (image_metadata_maps.size())
            media_filter |= MA::Media::Type::Image;
        media_type = MA::Media::getTypeFromFilter(media_filter);
    }


    // TODO: if Other or Image, load file into OpenCV to get more metadata and thumbnail?


    //std::filesystem::perms file_permissions = std::filesystem::status(file_path).permissions();
    //int file_permissions = int(std::filesystem::status(file_path).permissions());
    //bool file_read_only = !int(std::filesystem::perms::owner_write) & file_permissions;

    // Get Other File Attributes // TODO: use DirectoryTreeViewer pathChecker static function?
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

    // Add key value pairs to metadata maps not included in media info. Thumbnail data/path added in CreateThumbnailImages()
    AddAdditionalDatabaseFields(general_metadata_maps, Field::General::Id, file_id);
    //AddAdditionalDatabaseFields(general_metadata_maps.at(0).get(), Field::General::Id, file_id);
    AddAdditionalDatabaseFields(general_metadata_maps, Field::General::FileSize, file_size, false);
    AddAdditionalDatabaseFields(general_metadata_maps, Field::General::FileCreated, file_date_created, false);
    AddAdditionalDatabaseFields(general_metadata_maps, Field::General::FileLastMod, file_date_modified, false);
    AddAdditionalDatabaseFields(general_metadata_maps, Field::General::FileAttributes, static_cast<long long>(attributes));
    AddAdditionalDatabaseFields(general_metadata_maps, Field::General::FileTargetPath, target_path_str);
    if (not new_duplicate_file_found) {
        AddAdditionalDatabaseFields(video_metadata_maps, Field::Graphic::Id, file_id);
        AddAdditionalDatabaseFields(audio_metadata_maps, Field::Audio::Id, file_id);
        AddAdditionalDatabaseFields(music_metadata_maps, Field::Music::Id, file_id);
        AddAdditionalDatabaseFields(image_metadata_maps, Field::Music::Id, file_id);
        AddAdditionalDatabaseFields(extended_metadata_maps, Field::Extended::Id, file_id);
    }
    AddAdditionalDatabaseFields(extended_metadata_maps, Field::Extended::MediaType, media_type);
    AddAdditionalDatabaseFields(extended_metadata_maps, Field::Extended::MediaStreams, media_filter);

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
    emit progressMade(1);
    return media_type;
}

QVariant MetaDatabaseWorker::SterilizeValue(std::wstring value)
{
    /*/ TODO: Sterilize All Values (Tested: ' " \) ('Sigle Quotes' no longer need special escaping)
    intptr_t found_index = -2;
    while (found_index != value.npos) {
        found_index = value.find_first_of(L"'", found_index + 2);
        if (found_index != value.npos)
            value.insert(found_index, L"'");
        //value.replace(found_index, 1, L"\""); // Use to test a possible problem char. 
    };//*/
    return QString::fromStdWString(value);
}

MetaDatabaseWorker::KeyValuePair MetaDatabaseWorker::GetProperKeyAndValue(std::wstring key, std::wstring value, MediaInfoDLL::stream_t stream_kind)
{
    QString new_key = "";
    QVariant new_val = SterilizeValue(value);

    if (MediaInfoDLL::stream_t::Stream_General == stream_kind) {
        if (key == MEDIA_INFO::GENERAL::ALBUM) { // Music Table
            new_key = Field::Music::Album;
        }
        else if (key == MEDIA_INFO::GENERAL::ALBUM_PERFORMER) { // Music Table
            new_key = Field::Music::AlbumPerformer;
        }
        else if (key == MEDIA_INFO::GENERAL::ALBUM_REPLAY_GAIN) { // Music Table
            new_key = Field::Music::AlbumReplayGain;
        }
        else if (key == MEDIA_INFO::GENERAL::ALBUM_REPLAY_GAIN_PEAK) { // Music Table
            new_key = Field::Music::AlbumReplayGainPeak;
        }
        else if (key == MEDIA_INFO::GENERAL::AUDIO_LANGUAGE_LIST) { // Table???
            new_key = Field::General::AudioLanguageList;
        }
        else if (key == MEDIA_INFO::GENERAL::CODEC_ID) {
            new_key = Field::General::CodecId;
        }
        else if (key == MEDIA_INFO::GENERAL::COMMENT) { // Music Table
            new_key = Field::Music::Comment;
        }
        else if (key == MEDIA_INFO::GENERAL::COMMON_FORMATS) {
            new_key = Field::General::CommonFormats;
        }
        else if (key == MEDIA_INFO::GENERAL::COMPILATION) { // Music Table
            new_key = Field::Music::Compilation;
        }
        else if (key == MEDIA_INFO::GENERAL::COPYRIGHT) { // Music Table
            new_key = Field::Music::Copyright;
        }
        /*else if (key == MEDIA_INFO::GENERAL::COVER) { // Music Table
            new_key = Field::Music:;
        }*/
        /*else if (key == MEDIA_INFO::GENERAL::COVER_MIME) { // Music Table
            new_key = Field::Music:;
        }*/
        /*else if (key == MEDIA_INFO::GENERAL::COVER_TYPE) { // Music Table
            new_key = Field::Music:;
        }*/
        else if (key == MEDIA_INFO::GENERAL::DURATION) {
            new_key = Field::General::Duration;
            new_val = new_val.toLongLong();
        }
        /*else if (key == MEDIA_INFO::GENERAL::ENCODED_BY) { // Music Table
            new_key = Field::General:;
        }*/
        else if (key == MEDIA_INFO::GENERAL::ENCODED_DATE) {
            new_key = Field::General::EncodedDate;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::GENERAL::FILE_CREATION_DATE) {
            new_key = Field::General::FileCreated;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::GENERAL::FILE_LAST_MOD_DATE) {
            new_key = Field::General::FileLastMod;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::GENERAL::FILE_PATH) {
            new_key = Field::General::FilePath;
            new_val = file_path_str;
            //new_val = SterilizeValue(file_path_str.toStdWString());
        }
        /*else if (key == MEDIA_INFO::GENERAL::FILE_SIZE) { // Added later with a number in bytes
            new_key = Field::General::FileSize;
        }*/
        else if (key == MEDIA_INFO::GENERAL::FORMAT) {
            new_key = Field::General::Format;
            file_format = new_val.toString();
        }
        else if (key == MEDIA_INFO::GENERAL::FORMAT_PROFILE) { // Video Table
            new_key = Field::General::FormatInfo;
        }
        else if (key == MEDIA_INFO::GENERAL::FRAME_COUNT) {
            new_key = Field::General::FrameCount;
            new_val = new_val.toLongLong();
        }
        else if (key == MEDIA_INFO::GENERAL::FRAME_RATE) {
            new_key = Field::General::FrameRate;
            new_val = new_val.toDouble();
        }
        else if (key == MEDIA_INFO::GENERAL::GENRE) { // Music Table
            new_key = Field::Music::Genre;
        }
        else if (key == MEDIA_INFO::GENERAL::GROUPING) { // Music Table
            new_key = Field::Music::Grouping;
        }
        else if (key == MEDIA_INFO::GENERAL::ISRC) { // Music Table
            new_key = Field::Music::ISRC;
        }
        else if (key == MEDIA_INFO::GENERAL::LABEL) { // Music Table
            new_key = Field::Music::Label; // <-------- TODO: label to publisher?
        }
        else if (key == MEDIA_INFO::GENERAL::LYRICIST) { // Music Table
            new_key = Field::Music::Lyricist;
        }
        //else if (key == MEDIA_INFO::GENERAL::LYRICS) { // Music Table
        //    new_key = Field::Music:;
        //}
        else if (key == MEDIA_INFO::GENERAL::MIME_TYPE) {
            new_key = Field::General::MimeType;
        }
        else if (key == MEDIA_INFO::GENERAL::OVERALL_BIT_RATE) {
            new_key = Field::General::OverallBitRate;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::GENERAL::OVERALL_BIT_RATE_MODE) {
            new_key = Field::General::OverallBitRateMode;
        }
        else if (key == MEDIA_INFO::GENERAL::OWNR) { // Music Table
            new_key = Field::Music::Owner;
        }
        //else if (key == MEDIA_INFO::GENERAL::PART) { // Music Table
        //    new_key = Field::Music:;
        //}
        else if (key == MEDIA_INFO::GENERAL::PERFORMER) { // Music Table
            new_key = Field::Music::Performer;
        }
        else if (key == MEDIA_INFO::GENERAL::PERFORMER_URL) { // Music Table
            new_key = Field::Music::AudioURL;
        }
        else if (key == MEDIA_INFO::GENERAL::PUBLISHER) { // Music Table
            new_key = Field::Music::Publisher;
        }
        else if (key == MEDIA_INFO::GENERAL::RECORDED_DATE) { // Music Table
            new_key = Field::Music::RecordedDate;
            new_val = new_val.toInt(); // This should just be the year, else treat it like DateTime
        }
        else if (key == MEDIA_INFO::GENERAL::RELEASE_COUNTRY) { // Music Table
            new_key = Field::Music::ReleaseCountry;
        }
        else if (key == MEDIA_INFO::GENERAL::STREAMABLE) {
            new_key = Field::General::Streamable;
        }
        else if (key == MEDIA_INFO::GENERAL::STREAM_SIZE) {
            new_key = Field::General::StreamSize;
            new_val = new_val.toLongLong();
        }
        else if (key == MEDIA_INFO::GENERAL::TAGGED_DATE) {
            new_key = Field::General::TaggedDate;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::GENERAL::TITLE) { // Music Table
            new_key = Field::Music::Title;
        }
        else if (key == MEDIA_INFO::GENERAL::TRACK_NAME) { // Music Table
            new_key = Field::Music::TrackName;
        }
        else if (key == MEDIA_INFO::GENERAL::TRACK_NUMBER) { // Music Table
            new_key = Field::Music::TrackNumber;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::GENERAL::WEBPAGE) { // Music Table
            new_key = Field::Music::AudioURL; // 3 in General (Music), Preferred Order: 3
        }
        else if (key == MEDIA_INFO::GENERAL::WEBPAGE_SOURCE) { // Music Table
            new_key = Field::Music::AudioURL; // 3 in General (Music), Preferred Order: 4
        }
        /*else if (key == MEDIA_INFO::GENERAL::WRITING_LIBRARY) { // Music or Audio Table?
            new_key = Field::Audio::WritingLibrary; // Not including, this should show up in Video or Audio anyways
        }*/
    }

    if (MediaInfoDLL::stream_t::Stream_Video == stream_kind) {
        /*else if (key == MEDIA_INFO::VIDEO::BITS_PER_FRAME) { // This is just math using other metadata
            new_key = Field::Graphic:;
        }*/
        if (key == MEDIA_INFO::VIDEO::BIT_DEPTH) {
            new_key = Field::Graphic::BitDepth;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::VIDEO::BIT_RATE) {
            new_key = Field::Graphic::BitRate;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::VIDEO::CHROMA_SUBSAMPLING) {
            new_key = Field::Graphic::ChromaSubsampling;
        }
        else if (key == MEDIA_INFO::VIDEO::CODEC_ID) {
            new_key = Field::Graphic::CodecId;
        }
        else if (key == MEDIA_INFO::VIDEO::CODEC_ID_INFO) {
            new_key = Field::Graphic::CodecIdInfo;
        }
        /*else if (key == MEDIA_INFO::VIDEO::CODEX_CONFIG_BOX) {
            new_key = Field::Graphic:;
        }*/
        else if (key == MEDIA_INFO::VIDEO::COLOR_PRIMARIES) {
            new_key = Field::Graphic::ColorPrimaries;
        }
        else if (key == MEDIA_INFO::VIDEO::COLOR_RANGE) {
            new_key = Field::Graphic::ColorRange;
        }
        else if (key == MEDIA_INFO::VIDEO::COLOR_SPACE) {
            new_key = Field::Graphic::ColorSpace;
        }
        else if (key == MEDIA_INFO::VIDEO::DISPLAY_ASPECT_RATIO) {
            new_key = Field::Graphic::AspectRatio;
        }
        else if (key == MEDIA_INFO::VIDEO::DURATION) {
            new_key = Field::Graphic::Duration;
            new_val = new_val.toLongLong();
        }
        else if (key == MEDIA_INFO::VIDEO::ENCODED_DATE) {
            new_key = Field::Graphic::EncodedDate;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::VIDEO::ENCODING_SETTINGS) {
            new_key = Field::Graphic::EncodingSettings;
        }
        else if (key == MEDIA_INFO::VIDEO::FORMAT) {
            new_key = Field::Graphic::Format;
        }
        else if (key == MEDIA_INFO::VIDEO::FORMAT_INFO) {
            new_key = Field::Graphic::FormatInfo;
        }
        else if (key == MEDIA_INFO::VIDEO::FORMAT_PROFILE) {
            new_key = Field::Graphic::FormatProfile;
        }
        else if (key == MEDIA_INFO::VIDEO::FORMAT_SETTINGS) {
            new_key = Field::Graphic::FormatSettings;
        }
        else if (key == MEDIA_INFO::VIDEO::FORMAT_URL) {
            new_key = Field::Graphic::FormatURL;
        }
        else if (key == MEDIA_INFO::VIDEO::FRAME_COUNT) {
            new_key = Field::Graphic::FrameCount;
            new_val = new_val.toLongLong();
        }
        else if (key == MEDIA_INFO::VIDEO::FRAME_RATE) {
            new_key = Field::Graphic::FrameRate;
            new_val = new_val.toDouble();
        }
        else if (key == MEDIA_INFO::VIDEO::FRAME_RATE_MODE) {
            new_key = Field::Graphic::FrameRateMode;
        }
        else if (key == MEDIA_INFO::VIDEO::HEIGHT) {
            new_key = Field::Graphic::Height;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::VIDEO::ID) {
            new_key = Field::Graphic::TrackId;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::VIDEO::LANGUAGE) {
            new_key = Field::Graphic::Language;
        }
        else if (key == MEDIA_INFO::VIDEO::MATRIX_COEFFICIENTS) {
            new_key = Field::Graphic::MatrixCoefficients;
        }
        else if (key == MEDIA_INFO::VIDEO::ROTATION) {
            new_key = Field::Graphic::Rotation;
            new_val = new_val.toDouble();
        }
        else if (key == MEDIA_INFO::VIDEO::SCAN_TYPE) {
            new_key = Field::Graphic::ScanType;
        }
        else if (key == MEDIA_INFO::VIDEO::STREAM_SIZE) {
            new_key = Field::Graphic::StreamSize;
            new_val = new_val.toLongLong();
        }
        else if (key == MEDIA_INFO::VIDEO::TAGGED_DATE) {
            new_key = Field::Graphic::TaggedDate;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::VIDEO::TITLE) {
            new_key = Field::Graphic::Title;
        }
        else if (key == MEDIA_INFO::VIDEO::WIDTH) {
            new_key = Field::Graphic::Width;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::VIDEO::WRITING_LIBRARY) {
            new_key = Field::Graphic::WritingLibrary;
        }
    }

    if (MediaInfoDLL::stream_t::Stream_Audio == stream_kind) {
        /*if (key == MEDIA_INFO::AUDIO::ALTERNATE_GROUP) {
            new_key = Field::Audio:;
        }*/
        if (key == MEDIA_INFO::AUDIO::BIT_RATE) {
            new_key = Field::Audio::BitRate;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::AUDIO::BIT_RATE_MODE) {
            new_key = Field::Audio::BitRateMode;
        }
        else if (key == MEDIA_INFO::AUDIO::CHANNELS) {
            new_key = Field::Audio::Channels;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::AUDIO::CHANNELS_LAYOUT) {
            new_key = Field::Audio::ChannelLayout;
        }
        else if (key == MEDIA_INFO::AUDIO::CHANNEL_POSITIONS) {
            new_key = Field::Audio::ChannelPositions;
        }
        else if (key == MEDIA_INFO::AUDIO::CODEC_ID) {
            new_key = Field::Audio::CodecId;
        }
        else if (key == MEDIA_INFO::AUDIO::COMPRESSION_MODE) {
            new_key = Field::Audio::CompressionMode;
        }
        //else if (key == MEDIA_INFO::AUDIO::DEFAULT) {     // TODO: Is this default track? "TrackId" field?
        //    new_key = Field::Audio:;
        //}
        else if (key == MEDIA_INFO::AUDIO::DURATION) {
            new_key = Field::Audio::Duration;
            new_val = new_val.toLongLong();
        }
        else if (key == MEDIA_INFO::AUDIO::ENCODED_DATE) {
            new_key = Field::Audio::EncodedDate;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::AUDIO::FORMAT) {
            new_key = Field::Audio::Format;
        }
        else if (key == MEDIA_INFO::AUDIO::FORMAT_INFO) {
            new_key = Field::Audio::FormatInfo;
        }
        else if (key == MEDIA_INFO::AUDIO::FORMAT_PROFILE) {
            new_key = Field::Audio::FormatProfile;
        }
        else if (key == MEDIA_INFO::AUDIO::FORMAT_SETTINGS) {
            new_key = Field::Audio::FormatSettings;
        }
        /*else if (key == MEDIA_INFO::AUDIO::FORMAT_VERSION) {
            new_key = Field::Audio:;
        }*/
        else if (key == MEDIA_INFO::AUDIO::FRAME_COUNT) {
            new_key = Field::Audio::FrameCount;
            new_val = new_val.toLongLong();
        }
        else if (key == MEDIA_INFO::AUDIO::FRAME_RATE) {
            new_key = Field::Audio::FrameRate;
            //new_val = stringToNumber(new_val.toString());
            new_val = new_val.toDouble();
        }
        else if (key == MEDIA_INFO::AUDIO::ID) {
            new_key = Field::Audio::TrackId;
            new_val = new_val.toDouble();
        }
        else if (key == MEDIA_INFO::AUDIO::LANGUAGE) {
            new_key = Field::Audio::Language;
        }
        else if (key == MEDIA_INFO::AUDIO::MAX_BIT_RATE) {
            new_key = Field::Audio::BitRate;
            new_val = new_val.toInt();
        }
        /*else if (key == MEDIA_INFO::AUDIO::MD5) {
            new_key = Field::Audio:; // Id?
        }*/
        else if (key == MEDIA_INFO::AUDIO::REPLAY_GAIN) {
            new_key = Field::Audio::ReplayGain;
            new_val = new_val.toDouble();
        }
        else if (key == MEDIA_INFO::AUDIO::REPLAY_GAIN_PEAK) {
            new_key = Field::Audio::ReplayGainPeak;
            new_val = new_val.toDouble();
        }
        else if (key == MEDIA_INFO::AUDIO::SAMPLING_COUNT) {
            new_key = Field::Audio::SamplingCount;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::AUDIO::SAMPLING_RATE) {
            new_key = Field::Audio::SamplingRate;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::AUDIO::STREAM_SIZE) {
            new_key = Field::Audio::StreamSize;
            new_val = new_val.toLongLong();
        }
        else if (key == MEDIA_INFO::AUDIO::TAGGED_DATE) {
            new_key = Field::Audio::TaggedDate;
            new_val = dateTimeStringToSeconds(new_val.toString());
        }
        else if (key == MEDIA_INFO::AUDIO::TITLE) {
            new_key = Field::Audio::Title;
        }
        else if (key == MEDIA_INFO::AUDIO::WRITING_LIBRARY) {
            new_key = Field::Audio::WritingLibrary;
        }
    }

    if (MediaInfoDLL::stream_t::Stream_Image == stream_kind) {
        if (key == MEDIA_INFO::IMAGE::BIT_DEPTH) {
            new_key = Field::Graphic::BitDepth;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::IMAGE::CHROMA_SUBSAMPLING) {
            new_key = Field::Graphic::ChromaSubsampling;
        }
        else if (key == MEDIA_INFO::IMAGE::COLOR_SPACE) {
            new_key = Field::Graphic::ColorSpace;
        }
        else if (key == MEDIA_INFO::IMAGE::COLOR_SPACE_ICC) {
            new_key = Field::Graphic::ColorSpaceICC;
        }
        else if (key == MEDIA_INFO::IMAGE::COMPRESSION_MODE) {
            new_key = Field::Graphic::CompressionMode;
        }
        else if (key == MEDIA_INFO::IMAGE::FORMAT) {
            new_key = Field::Graphic::Format;
        }
        else if (key == MEDIA_INFO::IMAGE::HEIGHT) {
            new_key = Field::Graphic::Height;
            new_val = new_val.toInt();
        }
        else if (key == MEDIA_INFO::IMAGE::STREAM_SIZE) {
            new_key = Field::Graphic::StreamSize;
            new_val = new_val.toLongLong();
        }
        else if (key == MEDIA_INFO::IMAGE::WIDTH) {
            new_key = Field::Graphic::Width;
            new_val = new_val.toInt();
        }
    }

    // TODO: if (MediaInfoDLL::stream_t::Stream_Text == stream_kind) {

    //if (new_key == "")
    //    qDebug() << "WARNING: No proper key (database field) found for" << key << " |  Type:" << MEDIA_INFO::print(stream_kind);

    return KeyValuePair(new_key, new_val);
}

bool MetaDatabaseWorker::PlaceKeyValuePairIntoCorrectMap(KeyValuePair key_value_pair, MediaInfoDLL::stream_t stream_kind, int stream)
{
    bool success = true;
    if (MediaInfoDLL::stream_t::Stream_General == stream_kind) {
        if (Field::Music::doesFieldExist(key_value_pair.key)) {
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
    else if (MediaInfoDLL::stream_t::Stream_Video == stream_kind) {
        if (stream >= video_metadata_maps.size()) {
            video_metadata_maps.push_back(std::make_unique<std::map<QString, QVariant>>());
            return PlaceKeyValuePairIntoCorrectMap(key_value_pair, stream_kind, stream);
        }
        video_metadata_maps.at(stream)->emplace(key_value_pair.key, key_value_pair.value);
    }
    else if (MediaInfoDLL::stream_t::Stream_Audio == stream_kind) {
        if (stream >= audio_metadata_maps.size()) {
            audio_metadata_maps.push_back(std::make_unique<std::map<QString, QVariant>>());
            return PlaceKeyValuePairIntoCorrectMap(key_value_pair, stream_kind, stream);
        }
        audio_metadata_maps.at(stream)->emplace(key_value_pair.key, key_value_pair.value);
    }
    else if (MediaInfoDLL::stream_t::Stream_Image == stream_kind) {
        if (stream >= image_metadata_maps.size()) {
            image_metadata_maps.push_back(std::make_unique<std::map<QString, QVariant>>());
            return PlaceKeyValuePairIntoCorrectMap(key_value_pair, stream_kind, stream);
        }
        image_metadata_maps.at(stream)->emplace(key_value_pair.key, key_value_pair.value);
    }
    else if (MediaInfoDLL::stream_t::Stream_Other == stream_kind) { // Note: Stream_Other == Extended
        if (stream >= extended_metadata_maps.size()) {
            extended_metadata_maps.push_back(std::make_unique<std::map<QString, QVariant>>());
            return PlaceKeyValuePairIntoCorrectMap(key_value_pair, stream_kind, stream);
        }
        extended_metadata_maps.at(stream)->emplace(key_value_pair.key, key_value_pair.value);
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

    // TODO: user setting - user direct path to custom image
    // TODO: Based on above use this to search for more images based on each file (in current directory) without a thumbnail.
    //       Search that "direct path", parent, and sub-dirs of parent if it matches the same name as the parent of "direct path"
    
    if (not active) return;

    bool async_processing = user_settings->getUserSetting(Field::Settings::AsynchronousFileProcessing).toBool();
    if (async_processing and not new_duplicate_file_found and isLargeFile()) {
        setPaused(true);
        emit heavyComputationStarted(); // Un-pause first (to get here)
        WaitingLoop();
        heavy_computation_in_progress = true;
    }

    QTime start_time = QTime::currentTime();
    QTime end_time(0, 0, 0, 0);
    int progressed = 0;

    // TODO: Don't create and save more of the same thumbnail if dupe, but still get any different thumbnail paths. (TODO: use correct path and fall back to original dupe thumbnail path if not found)
    if ((MA::Media::Type::Video | MA::Media::Type::Image | MA::Media::Type::Music) & media_filter and not new_duplicate_file_found) {
    
        int frame_interval;
        int skip_frames_above;
        long max_frames = user_settings->getUserSetting(Field::Settings::MaxFramesToSearch).toLongLong();

        if (media_filter & MA::Media::Type::Video) {
            if (general_metadata_maps.at(0)->contains(Field::Graphic::FrameCount) and general_metadata_maps.at(0)->at(Field::Graphic::FrameCount).isValid()) {
                frame_count = general_metadata_maps.at(0)->at(Field::Graphic::FrameCount).toInt();
            }
            else if (frame_count == 0) {
                qWarning() << "WARNING: Missing frame_count, need alt way to create 'frame_interval'";
            }
            if (frame_count > max_frames)
                frame_count = max_frames;
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
        //QVector<QImage> thumbnail_images;
        QVector<QPixmap> thumbnail_images;

        frame = av_frame_alloc();
        packet = av_packet_alloc();
        if (!frame or !packet) {
            qDebug() << "Could not allocate frame or packet";
        }
        else {
            // 
            if ((return_value = LoadInAVContexts(file_path.string().c_str())) > -1) {
                while (av_read_frame(format_context, packet) >= 0) {

                    if (not active) break;

                    if (packet->stream_index == video_stream_index) {

                        // Skip last thumbnail, else it will be a frame near the very end of video.
                        if (decoder_context->frame_num < skip_frames_above) {

                            rgb_frame = DecodeFrame(decoder_context, frame, packet, frame_interval);
                            if (rgb_frame) {
                                QPixmap* thumbnail_pixmap = FrameToPixmap(rgb_frame);
                                thumbnail_images.push_back(*thumbnail_pixmap);
                                //av_frame_free(&rgb_frame);
                                delete thumbnail_pixmap;
                                progressed++;
                                emit progressMade(1);

                                // No need to traverse any more frames if the next viable frame is already known to be above the limit.
                                if (decoder_context->frame_num + frame_interval > skip_frames_above)
                                    break; // Not a big speedup because "skip_frames_above" is right at, or only a few frames above, the last frame saved.
                            }
                            // Is this unnecessary?
                            av_frame_free(&rgb_frame);
                            delete rgb_frame;
                            av_frame_free(&frame);
                            delete frame;
                            frame = av_frame_alloc();//
                        }
                        else {
                            av_packet_unref(packet);
                            break;
                        }
                    }
                    av_packet_unref(packet);

                    // Is this unnecessary?
                    av_packet_free(&packet);
                    delete packet;
                    packet = av_packet_alloc();//
                }
            }
            avcodec_free_context(&decoder_context);
            avformat_close_input(&format_context);
            avformat_free_context(format_context);
        }

        av_frame_free(&frame);
        av_packet_free(&packet);
        delete frame;
        delete packet;
        delete decoder_context;
        delete format_context;
        decoder_context = nullptr;
        format_context = nullptr;

        if (not active) return;

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

            AddAdditionalDatabaseFields(extended_metadata_maps, Field::Extended::ThumbnailData, ":thumbnail");
        }
        else {
            thumbnail_data = 0;
        }
    }
    
    if (not active) return;

    if (media_type & MA::Media::Type::Image) {
        thumbnail_path = QString::fromStdString(file_path.string());
    }
    // TODO: Always look for image on disk to use for thumbnail or only when none created/found from file?  user setting?  thumbnail_data.isEmpty() 
    else if (media_type > MA::Media::Type::Other) {
        // Search in the file's current and parent directory for files named: folder, cover, front, etc and extensions jpg, png, etc.
        std::filesystem::path thumbnail_link;
        for (auto const& dir_entry : std::filesystem::recursive_directory_iterator{ file_path.parent_path(), std::filesystem::directory_options::skip_permission_denied }) {
            std::string stem = dir_entry.path().stem().string();
            std::string ext = dir_entry.path().extension().string();
            std::string file_stem = file_path.stem().string();
            std::transform(stem.begin(), stem.end(), stem.begin(), tolower);
            std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
            std::transform(file_stem.begin(), file_stem.end(), file_stem.begin(), tolower);
            if (ext == ".jpg" or ext == ".jpeg" or ext == ".png" or ext == ".bpm") {
                if (stem == "cover" or stem == "folder" or stem == "front" or stem == "front-cover" or stem == "embedded" or stem == file_stem) {
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
                std::transform(stem.begin(), stem.end(), stem.begin(), tolower);
                std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
                if (ext == ".jpg" or ext == ".jpeg" or ext == ".png" or ext == ".bpm") {
                    if (stem == "cover" or stem == "folder" or stem == "front" or stem == "front-cover") {
                        thumbnail_link = dir_entry.path();
                        thumbnail_link.make_preferred();
                        break;
                    }
                }
            }
        }
        if (std::filesystem::exists(thumbnail_link)) {
            //qDebug() << "Found Music Thumbnail:" << thumbnail_link.string();
            thumbnail_path = QString::fromStdString(thumbnail_link.string());
            AddAdditionalDatabaseFields(extended_metadata_maps, Field::Extended::ThumbnailPath, thumbnail_path);
        }
    }
    
    if (not active) return;

    AddAdditionalDatabaseFields(general_metadata_maps, Field::General::Id, file_id, false);
    AddAdditionalDatabaseFields(general_metadata_maps, Field::General::FilePath, file_path_str, false);
    AddAdditionalDatabaseFields(extended_metadata_maps, Field::Extended::Id, file_id, false);

    if (not thumbnail_data.isNull())
        qDebug() << "Thumbnails created for" << file_name << "in" << end_time.addMSecs(start_time.msecsTo(QTime::currentTime())).toString("m:ss.zzz") << "milliseconds";

    emit progressMade(thumbnail_count - progressed);

    if (async_processing and not new_duplicate_file_found and isLargeFile()) {
        heavy_computation_in_progress = false;
        emit heavyComputationEnded(); // Un-pause next
    }
}

int MetaDatabaseWorker::LoadInAVContexts(const char* file_path_char, int force_jpeg_codec_for_embedded_stream, bool embedded_file_saved)
{
    int return_value;
    const AVCodec* decoder;

    format_context = avformat_alloc_context();

    if ((return_value = avformat_open_input(&format_context, file_path_char, NULL, NULL)) < 0) {
        qWarning() << "ERROR: failed to open input file";
        return return_value;
    }

    // Force jpeg codec if missing context.
    if (force_jpeg_codec_for_embedded_stream > -1) {
        format_context->streams[force_jpeg_codec_for_embedded_stream]->codecpar->codec_id = AV_CODEC_ID_MJPEG;
    }

    if ((return_value = avformat_find_stream_info(format_context, NULL)) < 0) {
        qWarning() << "ERROR: failed to find stream information";
        return return_value;
    }
    
    /*// Alt video stream finder
    for (int i = 0; i < format_context->nb_streams; i++) {

        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            // Find the decoder for the video stream
            decoder = avcodec_find_decoder(format_context->streams[i]->codecpar->codec_id); //AV_CODEC_ID_PNG  AV_CODEC_ID_MJPEG
            if (decoder == NULL) {
                fprintf(stderr, "Unsupported codec!\n");
                return -1; // Codec not found
            }
            return_value = i;
            break;
        }
    }//*/

    return_value = av_find_best_stream(format_context, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (return_value < 0) {
        qWarning() << "ERROR: failed to find a video stream in the input file";
        return return_value;
    }
    video_stream_index = return_value;

    // If no useful context found for an image, try forcing jpeg codec. (sometimes PNG images are embedded, but are actually Jpegs)
    if (force_jpeg_codec_for_embedded_stream == -1 and format_context->streams[video_stream_index]->codecpar->format == -1) {
        return LoadInAVContexts(file_path_char, video_stream_index);
    }

    /*/ Alt or back up option: Not enough useful context to modify image, saving image to disk to be loaded back in.
    if (embedded_file_saved and format_context->streams[video_stream_index]->codecpar->format == -1) {
        qDebug() << "Last resort, keep image on disk and load from image file when needed for thumbnail.";
        return -1;
    }
    if (embedded_file_saved) {
        std::filesystem::path embed_file_path = (file_path.parent_path() / "embedded.jpg").make_preferred();
        std::filesystem::remove(embed_file_path);
    }
    else if (format_context->streams[video_stream_index]->codecpar->format == -1) {
        if (format_context->streams[video_stream_index]->attached_pic.size) {

            std::filesystem::path embed_file_path = (file_path.parent_path() / "embedded.jpg").make_preferred();
            std::string embed_file_path_str = embed_file_path.string();
            const char* embed_file_path_c = embed_file_path_str.c_str();

            if (not std::filesystem::exists(embed_file_path)) {
                AVPacket packet = format_context->streams[video_stream_index]->attached_pic;
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
    av_read_frame(format_context, packet);
    FILE* image_file = fopen("embedded.jpg", "wb");
    int result = fwrite(packet->data, packet->size, 1, image_file);
    fclose(image_file);//*/
    
    decoder_context = avcodec_alloc_context3(decoder);
    if (!decoder_context) {
        qWarning() << "ERROR: failed to allocate decoding context/memory";
        return AVERROR(ENOMEM);
    }

    avcodec_parameters_to_context(decoder_context, format_context->streams[video_stream_index]->codecpar);

    /** Determine how many threads best suited for the decoding job. (Huge Speedup!)
      * Which multithreading methods to use.
      * Use of FF_THREAD_FRAME will increase decoding delay by one frame per thread,
      * so clients which cannot provide future frames should not use it.
      *
      * - encoding: Set by user, otherwise the default is used.
      * - decoding: Set by user, otherwise the default is used.
      */
    decoder_context->thread_count = 0;
    if (media_type & MA::Media::Image)// or file_format == "WebP" or file_extension == ".webp") // Note: Most images can still work with FF_THREAD_FRAME, but WebP images only work with FF_THREAD_SLICE.
        decoder_context->thread_type = FF_THREAD_SLICE;
    else if (decoder->capabilities & AV_CODEC_CAP_FRAME_THREADS)
        decoder_context->thread_type = FF_THREAD_FRAME; // Decode more than one frame at once
    else if (decoder->capabilities & AV_CODEC_CAP_SLICE_THREADS)
        decoder_context->thread_type = FF_THREAD_SLICE; // Decode more than one part of a single frame at once
    else
        decoder_context->thread_count = 1; // Don't use multithreading

    // Initiate the video decoder
    if ((return_value = avcodec_open2(decoder_context, decoder, NULL)) < 0) {
        qWarning() << "ERROR: failed to open video decoder";
        return return_value;
    }
    
    return 0;
}

AVFrame* MetaDatabaseWorker::DecodeFrame(AVCodecContext* codec_context, AVFrame* frame, AVPacket* packet, int mod)
{
    // Send packet to decoder
    int return_value = avcodec_send_packet(codec_context, packet);
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

    int media_item_size = user_settings->getUserSetting(Field::Settings::MediaItemStorageSize).toInt();;

    // Scale image keeping aspect ratio
    int scaled_width = codec_context->width;
    int scaled_height = codec_context->height;
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

    // Get frame back from decoder
    return_value = avcodec_receive_frame(codec_context, frame);

    // If resource temporaily not available or end of file reached, error out.
    if (return_value == AVERROR(EAGAIN) || return_value == AVERROR_EOF) {
        return nullptr; // Can still go again if more frames available
    }
    else if (return_value < 0) {
        qWarning() << "ERROR: Frame decoding failure, failed to receive frame. Code:" << return_value;
        return nullptr;
    }

    // Stopping here if this frame is not needed... next.
    if (codec_context->frame_num % mod > 0) {
        return nullptr; // Note: Frame has advanced and next frame can be read at this point.
    }

    AVPixelFormat src_format = (AVPixelFormat)frame->format;
    AVPixelFormat dst_format = AV_PIX_FMT_RGB24;

    // Create a scalar context for conversion
    //SwsContext* scale_context = sws_getContext(codec_context->width, codec_context->height, codec_context->pix_fmt, scaled_width, scaled_height, dst_format, SWS_BICUBIC, NULL, NULL, NULL);
    SwsContext* scale_context = sws_getContext(frame->width, frame->height, src_format, scaled_width, scaled_height, dst_format, SWS_BICUBIC, NULL, NULL, NULL);

    if (scale_context == nullptr) {
        qWarning() << "ERROR: Missing image context, scaling failed.";
        sws_freeContext(scale_context);
        return nullptr;
    }

    // Create a new RGB frame for conversion
    AVFrame* rgb_frame = av_frame_alloc();
    rgb_frame->format = dst_format;
    /////rgb_frame->format = AV_PIX_FMT_RGB24;
    //rgb_frame->format = codec_context->pix_fmt;
    //rgb_frame->format = AV_PIX_FMT_RGB32;
    //rgb_frame->format = AV_PIX_FMT_YUV420P;
    //rgb_frame->format = AV_PIX_FMT_YUV440P;
    //rgb_frame->format = AV_PIX_FMT_YUV444P;
    //rgb_frame->format = AV_PIX_FMT_BGRA;
    //rgb_frame->format = AV_PIX_FMT_BGR24;
    rgb_frame->width = scaled_width;
    rgb_frame->height = scaled_height;
    
    // Allocate a new buffer for the RGB conversion frame
    return_value = av_frame_get_buffer(rgb_frame, 0);

    /*/ Alt: prepare line sizes structure  and char buffer in array... as sws_scale expects
    int rgb_linesizes[8] = { 0 };
    rgb_linesizes[0] = 3 * frame->width;
    unsigned char* rgb_data[8];
    int image_byte_size = 3 * frame->height * frame->width;
    rgb_data[0] = (unsigned char*)malloc(image_byte_size + 64); // alloc extra 64 bytes
    if (!rgb_data[0]) {
        qDebug() << "Error allocating buffer for frame conversion";
        free(rgb_data[0]);
        sws_freeContext(scale_context);
        return nullptr;
    }//*/

    // Scale frame, if available, with no errors.
    if (return_value >= 0) {

        qDebug().nospace().noquote() << "Decoding frame #" << codec_context->frame_num << " (" << file_name << ")";

        // Scale/Convert the old frame to the new RGB frame.
        sws_scale(scale_context, frame->data, frame->linesize, 0, frame->height, rgb_frame->data, rgb_frame->linesize);
            
        /*/ Alt
        sws_scale(scale_context, frame->data, frame->linesize, 0, frame->height, rgb_data, rgb_linesizes);
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

        sws_freeContext(scale_context);
        return rgb_frame;
    }
    qWarning() << "ERROR: Failed to allocate buffer for RGB frame. Error Code:" << return_value;
    sws_freeContext(scale_context);
    av_frame_free(&rgb_frame);
    delete rgb_frame;
    return nullptr;
}

QPixmap* MetaDatabaseWorker::FrameToPixmap(const AVFrame* frame) const
{
    QImage image(frame->width, frame->height, QImage::Format::Format_RGB888);
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
        //(AVPixelFormat)decoder_context->pix_fmt,
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

    //QImage image(frame->data[0], decoder_context->width, decoder_context->height, frame->linesize[0], QImage::Format_RGB888);
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
    //if (metadata_map and not metadata_map->empty() and value.isValid())
    if (metadata_map and value.isValid())
        if (metadata_map->contains(key)) {
            if (overwrite)
                metadata_map->at(key) = value;
            //else
                //qDebug() << "[Not Overwritten] Key:" << key << " - Value:" << metadata_map->at(key) << " - New Value:" << value;
        }
        else
            metadata_map->insert({ key, value });
    //else
        //qDebug() << "Null metadata_map or invalid value, not adding:" << key;
}
void MetaDatabaseWorker::AddAdditionalDatabaseFields(std::vector< std::unique_ptr<std::map<QString, QVariant>> >& metadata_maps, QString key, QVariant value, bool overwrite, int stream)
{
    if (stream > -1)
        AddAdditionalDatabaseFields(metadata_maps.at(stream).get(), key, value, overwrite);
    else
        for (auto& metadata_map : metadata_maps)
            AddAdditionalDatabaseFields(metadata_map.get(), key, value, overwrite);
}

bool MetaDatabaseWorker::AddToDatabase()
{
    bool success = true;
    QList<QString> metadata_query_strings;

    for (auto& map : general_metadata_maps) {
        metadata_query_strings.append(BuildDatabaseInsertQueryString(map.get(), Table::General));
    }
    if (not new_duplicate_file_found) {
        for (auto& map : video_metadata_maps) {
            metadata_query_strings.append(BuildDatabaseInsertQueryString(map.get(), Table::Graphic));
        }
        for (auto& map : audio_metadata_maps) {
            metadata_query_strings.append(BuildDatabaseInsertQueryString(map.get(), Table::Audio));
        }
        for (auto& map : music_metadata_maps) {
            metadata_query_strings.append(BuildDatabaseInsertQueryString(map.get(), Table::Music));
        }
        for (auto& map : image_metadata_maps) {
            metadata_query_strings.append(BuildDatabaseInsertQueryString(map.get(), Table::Graphic));
        }
        for (auto& map : extended_metadata_maps) {
            metadata_query_strings.append(BuildDatabaseInsertQueryString(map.get(), Table::Extended));
        }
    }
    return ExecuteQueryStrings(metadata_query_strings);
}

bool MetaDatabaseWorker::UpdateDatabase()
{
    bool success = true;
    QList<QString> metadata_query_strings;

    if ((MA::Update::Metadata) & update_filter) {

        for (int i = 0; i < general_metadata_maps.size(); i++) {
            auto& map = general_metadata_maps.at(i);
            for (auto& field : Field::General::getAllFields()) {
                if (not map->contains(field)) { // and (field != Field::General::ThumbnailData and field != Field::General::ThumbnailPath)) {
                    KeyValuePair key_value_pair(field, QVariant()); // Reset to NULL (except thumbnails)
                    //AddAdditionalDatabaseFields(general_metadata_maps.at(i).get(), field, media_type);
                    if (not PlaceKeyValuePairIntoCorrectMap(key_value_pair, MediaInfoDLL::stream_t::Stream_General, i))
                        qWarning() << "WANRING: Field not added to map:" << field;
                }
            }
            metadata_query_strings.append(BuildDatabaseUpdateQueryString(map.get(), Table::General));
        }
        for (int i = 0; i < video_metadata_maps.size(); i++) {
            auto& map = video_metadata_maps.at(i);
            for (auto& field : Field::Graphic::getAllFields()) {
                if (not map->contains(field)) {
                    KeyValuePair key_value_pair(field, QVariant()); // Reset to NULL
                    if (not PlaceKeyValuePairIntoCorrectMap(key_value_pair, MediaInfoDLL::stream_t::Stream_Video, i))
                        qWarning() << "WANRING: Field not added to map:" << field;
                }
            }
            metadata_query_strings.append(BuildDatabaseUpdateQueryString(map.get(), Table::Graphic));
        }
        for (int i = 0; i < music_metadata_maps.size(); i++) {
            auto& map = music_metadata_maps.at(i);
            for (auto& field : Field::Music::getAllFields()) {
                if (not map->contains(field)) {
                    KeyValuePair key_value_pair(field, QVariant()); // Reset to NULL
                    if (not PlaceKeyValuePairIntoCorrectMap(key_value_pair, MediaInfoDLL::stream_t::Stream_General, i)) 
                        qWarning() << "WANRING: Field not added to map:" << field;
                }
            }
            metadata_query_strings.append(BuildDatabaseUpdateQueryString(map.get(), Table::Music));
        }
        for (int i = 0; i < audio_metadata_maps.size(); i++) {
            auto& map = audio_metadata_maps.at(i);
            for (auto& field : Field::Audio::getAllFields()) {
                if (not map->contains(field)) {
                    KeyValuePair key_value_pair(field, QVariant()); // Reset to NULL
                    if (not PlaceKeyValuePairIntoCorrectMap(key_value_pair, MediaInfoDLL::stream_t::Stream_Audio, i))
                        qWarning() << "WANRING: Field not added to map:" << field;
                }
            }
            metadata_query_strings.append(BuildDatabaseUpdateQueryString(map.get(), Table::Audio));
        }
        for (int i = 0; i < image_metadata_maps.size(); i++) { // TODO: there may be a problem with Graphic table multi streams where a video and image are found (find an example file for testing)
            auto& map = image_metadata_maps.at(i);
            for (auto& field : Field::Graphic::getAllFields()) {
                if (not map->contains(field)) {
                    KeyValuePair key_value_pair(field, QVariant()); // Reset to NULL
                    if (not PlaceKeyValuePairIntoCorrectMap(key_value_pair, MediaInfoDLL::stream_t::Stream_Image, i))
                        qWarning() << "WANRING: Field not added to map:" << field;
                }
            }
            metadata_query_strings.append(BuildDatabaseUpdateQueryString(map.get(), Table::Graphic));
        }
        for (int i = 0; i < extended_metadata_maps.size(); i++) {
            auto& map = extended_metadata_maps.at(i);
            for (auto& field : Field::Extended::getAllFields()) {
                if (not map->contains(field) and (field != Field::Extended::ThumbnailData and field != Field::Extended::ThumbnailPath)) {
                    KeyValuePair key_value_pair(field, QVariant()); // Reset to NULL
                    if (not PlaceKeyValuePairIntoCorrectMap(key_value_pair, MediaInfoDLL::stream_t::Stream_Other, i)) // Note: Stream_Other == Extended
                        qWarning() << "WANRING: Field not added to map:" << field;
                }
            }
            metadata_query_strings.append(BuildDatabaseUpdateQueryString(map.get(), Table::Extended));
        }
    }
    else if ((MA::Update::SmallFileThumbnails | MA::Update::AllFileThumbnails) & update_filter) {
        if (extended_metadata_maps.at(0)->contains(Field::Extended::ThumbnailData) or extended_metadata_maps.at(0)->contains(Field::Extended::ThumbnailPath))
            metadata_query_strings.append(BuildDatabaseUpdateQueryString(extended_metadata_maps.at(0).get(), Table::Extended));
    }
    if (user_meta) {
        if (ExistsInDatabase(Table::UserMeta))
            metadata_query_strings.append(BuildDatabaseUpdateQueryString(user_meta, Table::UserMeta));
        else {
            user_meta->at(Field::UserMeta::Id).setValue(file_id);
            metadata_query_strings.append(BuildDatabaseInsertQueryString(user_meta, Table::UserMeta));
        }
    }
    return ExecuteQueryStrings(metadata_query_strings);
}

bool MetaDatabaseWorker::ExecuteQueryStrings(QList<QString> query_strings)
{
    bool success = true;
    //bool general = true;
    QSqlQuery query(database);
    
    for (auto& qs : query_strings) {
        //qDebug() << qs;
        if (qs.length()) {
            query.prepare(qs);
            query.bindValue(":thumbnail", thumbnail_data);
            /*if (general) {
                query.bindValue(":thumbnail", thumbnail_data);
                general = false;
            }*/
            if (query.exec()) {
                qDebug().nospace() << "Values added/updated successfully to DB. | File: " << file_path.filename().string() << " | Query: " << query.lastQuery();
                //success = true;
            }
            else {
                qWarning() << "ERROR: Failed To add Values from file:" << file_path.filename().string() << " | " << query.lastError();
                qWarning() << "ERROR: The Failed Query:" << query.lastQuery();
                success = false;
                // TODO: Inform user of error and how to possibly fix it.
            }
            query.clear();
            query.finish();
        }
    }
    if (success) {
        emit databaseUpdated(file_id);
        emit progressMade(1);
    }
    else
        emit progressMade(2); // Next step will be skipped on fail
    return success;
}

QString MetaDatabaseWorker::BuildDatabaseInsertQueryString(std::map<QString, QVariant>* metadata_map, QString table) const
{
    QString insert;
    QString values;
    if (metadata_map and metadata_map->size()) {
        insert = QString("INSERT INTO %1 (").arg(table);
        values = "VALUES (";
        for (auto& metadata : *metadata_map) {
            insert.append(QString("\"%1\", ").arg(metadata.first));
            if (metadata.second.isValid())
                if (metadata.second.typeId() == QMetaType::QString)
                    if (metadata.second.toString().length() and metadata.second.toString().at(0) == QString(":"))
                        values.append(QString("%1, ").arg(metadata.second.toString())); // "variable" placeholder, value added later
                    else
                        values.append(QString("\"%1\", ").arg(metadata.second.toString())); // string
                else
                    values.append(QString("%1, ").arg(metadata.second.toString())); // number or some other kind of data
            else
                values.append("NULL, ");
        }
        insert = insert.first(insert.length() - 2).append(") ");
        values = values.first(values.length() - 2).append(")");
        //qDebug() << insert + values;
    }
    return insert + values;
}

QString MetaDatabaseWorker::BuildDatabaseUpdateQueryString(std::map<QString, QVariant>* metadata_map, QString table) const
{
    QString update;
    QString where_id;
    if (metadata_map and metadata_map->size()) {
        update = QString("UPDATE \"%1\" SET ").arg(table);
        where_id = "WHERE ";
        for (auto& metadata : *metadata_map) {

            // Match File ID and (General::FilePath or Graphic::TrackId or Audio::TrackId or UserMeta::UserId)
            if (metadata.first == Field::General::Id) {
                where_id.append(QString("\"%1\" = \"%2\" AND ").arg(metadata.first).arg(file_id));
                continue;
            }
            else if (table == Table::General and metadata.first == Field::General::FilePath) {
                where_id.append(QString("\"%1\" = \"%2\" AND ").arg(metadata.first).arg(file_path_str));
                continue;
            }
            else if (table == Table::Graphic and metadata.first == Field::Graphic::TrackId and metadata.second.isValid()) {
                where_id.append(QString("\"%1\" = %2 AND ").arg(metadata.first).arg(metadata.second.toString()));
                continue;
            }
            else if (table == Table::Audio and metadata.first == Field::Audio::TrackId and metadata.second.isValid()) {
                where_id.append(QString("\"%1\" = %2 AND ").arg(metadata.first).arg(metadata.second.toString()));
                continue;
            }
            else if (table == Table::UserMeta and metadata.first == Field::UserMeta::UserId) {
                where_id.append(QString("\"%1\" = %2 AND ").arg(metadata.first).arg(user_id));
                continue;
            }
            else {
                update.append(QString("\"%1\" = ").arg(metadata.first));
            }

            if (metadata.second.isValid())
                if (metadata.second.typeId() == QMetaType::QString)
                    if (metadata.second.toString().length() and metadata.second.toString().at(0) == QString(":"))
                        update.append(QString("%1, ").arg(metadata.second.toString())); // "variable" placeholder, value added later
                    else
                        update.append(QString("\"%1\", ").arg(metadata.second.toString())); // string
                else
                    update.append(QString("%1, ").arg(metadata.second.toString())); // number or some other kind of data
            else
                update.append("NULL, ");
        }
        update = update.first(update.length() - 2).append(" ");
        where_id = where_id.first(where_id.length() - 5).append(";");
        if (update.length() < 20) {
            qWarning() << "ERROR: Database Update will Fail, No Fields SET!";
            qWarning() << "ERROR: Query String:" << update + where_id;
            update = "";
            where_id = "";
        }
        if (where_id.length() < 7) {
            qWarning() << "ERROR: Database Update will Fail, WHERE Id is missing!";
            qWarning() << "ERROR: Query String:" << update + where_id;
            update = "";
            where_id = "";
        }
        //qDebug() << update + where_id;
    }
    return update + where_id;
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
