/**********************************
* Namespaces, Enums and Constants *
***********************************/

inline std::filesystem::path BUILD_DIR_PATH;
inline std::filesystem::path WORKING_DIR_PATH;

inline namespace DatabaseSchema
{
    namespace Database {
        const char Type[] = "QSQLITE";
        //const char Type[] = "QPSQL"; //Postgre SQL
        const char ConnectionName[] = "MediaArchConnection";
        const char Host[] = "MediaArch";
        const char Name[] = "MediaArchDB";
        const char UserName[] = "MediaArch";
        const char Password[] = "MediaArch";
    }
    namespace DataType {
        const QString Boolean = "int";
        const QString Number = "int";
        const QString Precision = "real";
        const QString Text = "text";
        const QString Blob = "blob";
        const QString NotNull = " not null";
        const QString Primary = " primary key";
    }
    static const struct Tables {
        inline static const QString General = "General";
        inline static const QString Graphic = "Graphic";
        inline static const QString Audio = "Audio";
        inline static const QString Music = "Music";
        inline static const QString User = "User";
        inline static const QString Settings = "Settings";

        static const QList<QString> getAllTables() {
            return QList<QString>(
                { General, Graphic, Audio, Music, User, Settings }
            );
        }
    };
    namespace Table {
        namespace General {
            const QString Title = DatabaseSchema::Tables::General;
            static const struct Field {
                inline static const QString Id = "Id"; // Primary Key, Not Null
                inline static const QString MediaType = "Media Type";
                inline static const QString FilePath = "File Path";
                inline static const QString FileSize = "File Size";
                inline static const QString FileCreated = "File Created";
                inline static const QString FileLastMod = "File Last Modified";
                inline static const QString FileReadOnly = "File Read Only";
                inline static const QString FileHidden = "File Hidden";
                inline static const QString AudioLanguageList = "Audio Language List"; // TODO: General -> Audio?
                inline static const QString Format = "Format";
                inline static const QString FormatInfo = "Format Info";
                inline static const QString CommonFormats = "Common Formats";
                inline static const QString MimeType = "Mime Type";
                inline static const QString CodecId = "Codec Id";
                inline static const QString Duration = "Duration";
                inline static const QString OverallBitRate = "Overall Bit Rate";
                inline static const QString OverallBitRateMode = "Overall Bit Rate Mode";
                inline static const QString FrameRate = "Frame Rate";
                inline static const QString FrameCount = "Frame Count";
                inline static const QString StreamSize = "Stream Size";
                inline static const QString Streamable = "Streamable";
                inline static const QString EncodedDate = "Encoded Date";
                inline static const QString TaggedDate = "Tagged Date";
                inline static const QString Thumbnail = "Thumbnail";

                static const QList<QString> getAllFields() {
                    return QList<QString>(
                        { Id, MediaType, FilePath, FileSize, FileCreated, FileLastMod, FileReadOnly, FileHidden, AudioLanguageList, Format, FormatInfo, CommonFormats,
                        MimeType, CodecId, Duration, OverallBitRate, OverallBitRateMode, FrameRate, FrameCount, StreamSize, Streamable, EncodedDate,
                        TaggedDate, Thumbnail }
                    );
                }
            };
            static const struct DataType {
                inline static const QString Id = DatabaseSchema::DataType::Text + DatabaseSchema::DataType::NotNull + DatabaseSchema::DataType::Primary;
                inline static const QString MediaType = DatabaseSchema::DataType::Number;
                inline static const QString FilePath = DatabaseSchema::DataType::Text; // TODO: array of paths?
                inline static const QString FileSize = DatabaseSchema::DataType::Number;
                inline static const QString FileCreated = DatabaseSchema::DataType::Number;
                inline static const QString FileLastMod = DatabaseSchema::DataType::Number;
                inline static const QString FileReadOnly = DatabaseSchema::DataType::Boolean;
                inline static const QString FileHidden = DatabaseSchema::DataType::Boolean;
                inline static const QString AudioLanguageList = DatabaseSchema::DataType::Text;
                inline static const QString Format = DatabaseSchema::DataType::Text;
                inline static const QString FormatInfo = DatabaseSchema::DataType::Text;
                inline static const QString CommonFormats = DatabaseSchema::DataType::Text;
                inline static const QString MimeType = DatabaseSchema::DataType::Text;
                inline static const QString CodecId = DatabaseSchema::DataType::Text;
                inline static const QString Duration = DatabaseSchema::DataType::Number;
                inline static const QString OverallBitRate = DatabaseSchema::DataType::Number;
                inline static const QString OverallBitRateMode = DatabaseSchema::DataType::Text;
                inline static const QString FrameRate = DatabaseSchema::DataType::Number;
                inline static const QString FrameCount = DatabaseSchema::DataType::Number;
                inline static const QString StreamSize = DatabaseSchema::DataType::Number;
                inline static const QString Streamable = DatabaseSchema::DataType::Number;
                inline static const QString EncodedDate = DatabaseSchema::DataType::Number;
                inline static const QString TaggedDate = DatabaseSchema::DataType::Number;
                inline static const QString Thumbnail = DatabaseSchema::DataType::Blob;

                static const QList<QString> getAllDataTypes() {
                    return QList<QString>(
                        { Id, MediaType, FilePath, FileSize, FileCreated, FileLastMod, FileReadOnly, FileHidden, AudioLanguageList, Format, FormatInfo,
                        CommonFormats, MimeType, CodecId, Duration, OverallBitRate, OverallBitRateMode, FrameRate, FrameCount, StreamSize, Streamable,
                        EncodedDate, TaggedDate, Thumbnail }
                    );
                }
            };
        }
        namespace Graphic {
            const QString Title = "Graphic";
            static const struct Field {
                inline static const QString Id = DatabaseSchema::Table::General::Field::Id;
                inline static const QString TrackId = "Track Id";
                inline static const QString Format = "Format";
                inline static const QString FormatInfo = "Format Info";
                inline static const QString FormatURL = "Format URL";
                inline static const QString FormatProfile = "Format Profile";
                inline static const QString FormatSettings = "Format Settings";
                inline static const QString CodecId = "Codec Id";
                inline static const QString CodecIdInfo = "Codec Id Info";
                inline static const QString Duration = "Duration";
                inline static const QString BitRate = "Bit Rate";
                inline static const QString Width = "Width";
                inline static const QString Height = "Height";
                inline static const QString AspectRatio = "Aspect Ratio";
                inline static const QString Rotation = "Rotation";
                inline static const QString FrameRate = "Frame Rate"; // FPS can be gotten with math [Duration/1000 / FrameCount]
                inline static const QString FrameCount = "Frame Count";
                inline static const QString FrameRateMode = "Frame Rate Mode";
                inline static const QString BitDepth = "Bit Depth";
                inline static const QString ColorSpace = "Color Space";
                inline static const QString ChromaSubsampling = "Chroma Subsampling";
                inline static const QString CompressionMode = "Compression Mode";
                inline static const QString ScanType = "Scan Type";
                inline static const QString StreamSize = "Stream Size";
                inline static const QString Title = "Title";
                inline static const QString Language = "Language";
                inline static const QString WritingLibrary = "Writing Library";
                inline static const QString EncodingSettings = "Encoding Settings";
                inline static const QString EncodedDate = "Encoded Date";
                inline static const QString TaggedDate = "Tagged Date";
                inline static const QString ColorRange = "Color Range";
                inline static const QString ColorPrimaries = "Color Primaries";
                inline static const QString TransferCharacteristics = "Transfer Characteristics";
                inline static const QString MatrixCoefficients = "Matrix Coefficients";

                static const QList<QString> getAllFields() {
                    return QList<QString>(
                        { Id, TrackId, Format, FormatInfo, FormatURL, FormatProfile, FormatSettings, CodecId, CodecIdInfo, Duration, BitRate, Width,
                        Height, AspectRatio, Rotation, FrameRate, FrameCount, FrameRateMode, BitDepth, ColorSpace, ChromaSubsampling, CompressionMode,
                        ScanType, StreamSize, Title, Language, WritingLibrary, EncodingSettings, EncodedDate, TaggedDate, ColorRange, ColorPrimaries,
                        TransferCharacteristics, MatrixCoefficients }
                    );
                }
            };
            static const struct DataType {
                inline static const QString Id = DatabaseSchema::DataType::Text + DatabaseSchema::DataType::NotNull;
                inline static const QString TrackId = DatabaseSchema::DataType::Number;
                //inline static const QString Type = DatabaseSchema::DataType::Number;
                inline static const QString Format = DatabaseSchema::DataType::Text;
                inline static const QString FormatInfo = DatabaseSchema::DataType::Text;
                inline static const QString FormatURL = DatabaseSchema::DataType::Text;
                inline static const QString FormatProfile = DatabaseSchema::DataType::Text;
                inline static const QString FormatSettings = DatabaseSchema::DataType::Text;
                inline static const QString CodecId = DatabaseSchema::DataType::Text;
                inline static const QString CodecIdInfo = DatabaseSchema::DataType::Text;
                inline static const QString Duration = DatabaseSchema::DataType::Number;
                inline static const QString BitRate = DatabaseSchema::DataType::Number;
                inline static const QString Width = DatabaseSchema::DataType::Number;
                inline static const QString Height = DatabaseSchema::DataType::Number;
                inline static const QString AspectRatio = DatabaseSchema::DataType::Text;
                inline static const QString Rotation = DatabaseSchema::DataType::Precision;
                inline static const QString FrameRate = DatabaseSchema::DataType::Number;
                inline static const QString FrameCount = DatabaseSchema::DataType::Number;
                inline static const QString FrameRateMode = DatabaseSchema::DataType::Text;
                inline static const QString BitDepth = DatabaseSchema::DataType::Number;
                inline static const QString ColorSpace = DatabaseSchema::DataType::Text;
                inline static const QString ChromaSubsampling = DatabaseSchema::DataType::Text;
                inline static const QString CompressionMode = DatabaseSchema::DataType::Text;
                inline static const QString ScanType = DatabaseSchema::DataType::Text;
                inline static const QString StreamSize = DatabaseSchema::DataType::Number;
                inline static const QString Title = DatabaseSchema::DataType::Text;
                inline static const QString Language = DatabaseSchema::DataType::Text;
                inline static const QString WritingLibrary = DatabaseSchema::DataType::Text;
                inline static const QString EncodingSettings = DatabaseSchema::DataType::Text;
                inline static const QString EncodedDate = DatabaseSchema::DataType::Number;
                inline static const QString TaggedDate = DatabaseSchema::DataType::Number;
                inline static const QString ColorRange = DatabaseSchema::DataType::Text;
                inline static const QString ColorPrimaries = DatabaseSchema::DataType::Text;
                inline static const QString TransferCharacteristics = DatabaseSchema::DataType::Text;
                inline static const QString MatrixCoefficients = DatabaseSchema::DataType::Text;

                static const QList<QString> getAllDataTypes() {
                    return QList<QString>(
                        { Id, TrackId, Format, FormatInfo, FormatURL, FormatProfile, FormatSettings, CodecId, CodecIdInfo, Duration, BitRate, Width,
                        Height, AspectRatio, Rotation, FrameRate, FrameCount, FrameRateMode, BitDepth, ColorSpace, ChromaSubsampling, CompressionMode,
                        ScanType, StreamSize, Title, Language, WritingLibrary, EncodingSettings, EncodedDate, TaggedDate, ColorRange, ColorPrimaries,
                        TransferCharacteristics, MatrixCoefficients }
                    );
                }
            };
        }
        namespace Audio {
            const QString Title = "Audio";
            static const struct Field {
                inline static const QString Id = DatabaseSchema::Table::General::Field::Id;
                inline static const QString TrackId = "Track Id";
                inline static const QString Format = "Format";
                inline static const QString FormatInfo = "Format Info";
                inline static const QString FormatProfile = "Format Profile";
                inline static const QString FormatSettings = "Format Settings";
                inline static const QString CodecId = "Codec Id";
                inline static const QString Duration = "Duration";
                inline static const QString BitRateMode = "Bit Rate Mode";
                inline static const QString BitRate = "Bit Rate";
                inline static const QString Channels = "Channels";
                inline static const QString ChannelPositions = "Channel Positions";
                inline static const QString ChannelLayout = "Channel Layout";
                inline static const QString SamplingRate = "Sampling Rate";
                inline static const QString SamplingCount = "Sampling Count";
                inline static const QString FrameRate = "Frame Rate"; // FPS and "Samples per frame" can be gotten with math [Duration/1000 or Samples count / Frame count]
                inline static const QString FrameCount = "Frame Count";
                inline static const QString CompressionMode = "Compression Mode";
                inline static const QString ReplayGain = "Replay Gain";
                inline static const QString ReplayGainPeak = "Replay Gain Peak";
                inline static const QString StreamSize = "Stream Size";
                inline static const QString Title = "Title";
                inline static const QString Language = "Language";
                inline static const QString EncodedDate = "Encoded Date";
                inline static const QString TaggedDate = "Tagged Date";
                inline static const QString WritingLibrary = "Writing Library";

                static const QList<QString> getAllFields() {
                    return QList<QString>(
                        { Id, TrackId, Format, FormatInfo, FormatProfile, FormatSettings, CodecId, Duration, BitRateMode, BitRate, Channels, ChannelPositions,
                        ChannelLayout, SamplingRate, SamplingCount, FrameRate, FrameCount, CompressionMode, ReplayGain, ReplayGainPeak, StreamSize, Title,
                        Language, EncodedDate, TaggedDate, WritingLibrary }
                    );
                }
            };
            static const struct DataType {
                inline static const QString Id = DatabaseSchema::DataType::Text + DatabaseSchema::DataType::NotNull;
                inline static const QString TrackId = DatabaseSchema::DataType::Number;
                inline static const QString Format = DatabaseSchema::DataType::Text;
                inline static const QString FormatInfo = DatabaseSchema::DataType::Text;
                inline static const QString FormatProfile = DatabaseSchema::DataType::Text;
                inline static const QString FormatSettings = DatabaseSchema::DataType::Text;
                inline static const QString CodecId = DatabaseSchema::DataType::Text;
                inline static const QString Duration = DatabaseSchema::DataType::Number;
                inline static const QString BitRateMode = DatabaseSchema::DataType::Text;
                inline static const QString BitRate = DatabaseSchema::DataType::Number;
                inline static const QString Channels = DatabaseSchema::DataType::Number;
                inline static const QString ChannelPositions = DatabaseSchema::DataType::Text;
                inline static const QString ChannelLayout = DatabaseSchema::DataType::Text;
                inline static const QString SamplingRate = DatabaseSchema::DataType::Number;
                inline static const QString SamplingCount = DatabaseSchema::DataType::Number;
                inline static const QString FrameRate = DatabaseSchema::DataType::Precision;
                inline static const QString FrameCount = DatabaseSchema::DataType::Number;
                inline static const QString CompressionMode = DatabaseSchema::DataType::Text;
                inline static const QString ReplayGain = DatabaseSchema::DataType::Precision;
                inline static const QString ReplayGainPeak = DatabaseSchema::DataType::Precision;
                inline static const QString StreamSize = DatabaseSchema::DataType::Number;
                inline static const QString Title = DatabaseSchema::DataType::Text;
                inline static const QString Language = DatabaseSchema::DataType::Text;
                inline static const QString EncodedDate = DatabaseSchema::DataType::Number;
                inline static const QString TaggedDate = DatabaseSchema::DataType::Number;
                inline static const QString WritingLibrary = DatabaseSchema::DataType::Text;

                static const QList<QString> getAllDataTypes() {
                    return QList<QString>(
                        { Id, TrackId, Format, FormatInfo, FormatProfile, FormatSettings, CodecId, Duration, BitRateMode, BitRate, Channels, ChannelPositions,
                        ChannelLayout, SamplingRate, SamplingCount, FrameRate, FrameCount, CompressionMode, ReplayGain, ReplayGainPeak, StreamSize, Title,
                        Language, EncodedDate, TaggedDate, WritingLibrary }
                    );
                }
            };
        }
        namespace Music {
            const QString Title = "Music";
            static const struct Field {
                // Music (Note: these are placed/categorized in the general media info metadata for some reason)
                inline static const QString Id = DatabaseSchema::Table::General::Field::Id;
                inline static const QString Album = "Album";
                inline static const QString AlbumPerformer = "Album Performer";
                inline static const QString Performer = "Performer";
                inline static const QString Title = "Title"; // Same as track name?
                inline static const QString TrackName = "Track Name";
                inline static const QString TrackNumber = "Track Number";
                inline static const QString TrackPerformer = "Track Performer";
                inline static const QString Lyricist = "Lyricist";
                inline static const QString Publisher = "Publisher";
                inline static const QString Label = "Label";
                inline static const QString Compilation = "Compilation";
                inline static const QString Grouping = "Grouping";
                inline static const QString Genre = "Genre";
                inline static const QString ISRC = "ISRC";
                inline static const QString Copyright = "Copyright";
                inline static const QString AlbumReplayGain = "Album Replay Gain";
                inline static const QString AlbumReplayGainPeak = "Album Replay Gain Peak";
                inline static const QString RecordedDate = "Recorded Date";
                inline static const QString AudioURL = "Audio URL"; // Performer/Url > Official audio file webpage > Official audio source webpage
                inline static const QString CoverURL = "Cover URL";
                inline static const QString Comment = "Comment";
                inline static const QString ReleaseCountry = "Release Country";
                inline static const QString Owner = "Owner";

                /* TODO?
                Part                                     : 1
                Label                                    : 2008 KG                          | Label > Publisher  ?
                Performer/Url                            : www.t.me/pmedia_music
                Encoded by                               : PMEDIA                           | music or general?
                x Lyricist                                 : www.t.me/pmedia_music
                Writing library                          : LAME3.99.0
                x ISRC                                     : PMEDIA
                x Copyright                                : PMEDIA
                Cover                                    : Yes
                Cover type                               : Cover (front)
                Cover MIME                               : image/png
                Rating                                   : 5
                x ownr                                     : PMEDIA / PMEDIA / www.t.me/pmedia_music
                Official audio file webpage              : www.t.me/pmedia_music
                Official audio source webpage            : www.t.me/pmedia_music
                */

                static const QList<QString> getAllFields() {
                    return QList<QString>(
                        { Id, Album, AlbumPerformer, Performer, TrackName, TrackNumber, TrackPerformer, Lyricist, Publisher, Label, Compilation, Grouping, Genre,
                        ISRC, Copyright, AlbumReplayGain, AlbumReplayGainPeak, RecordedDate, AudioURL, CoverURL, Comment, ReleaseCountry, Owner }
                    );
                }
                static const bool doesFieldExist(QString field) {
                    for (auto& f : getAllFields()) {
                        if (field == f)
                            return true;
                    }
                    return false;
                }
            };
            static const struct DataType {
                inline static const QString Id = DatabaseSchema::DataType::Text + DatabaseSchema::DataType::NotNull;
                inline static const QString Album = DatabaseSchema::DataType::Text;
                inline static const QString AlbumPerformer = DatabaseSchema::DataType::Text;
                inline static const QString Performer = DatabaseSchema::DataType::Text;
                inline static const QString TrackName = DatabaseSchema::DataType::Text;
                inline static const QString TrackNumber = DatabaseSchema::DataType::Number;
                inline static const QString TrackPerformer = DatabaseSchema::DataType::Text;
                inline static const QString Lyricist = DatabaseSchema::DataType::Text;
                inline static const QString Publisher = DatabaseSchema::DataType::Text;
                inline static const QString Label = DatabaseSchema::DataType::Text;
                inline static const QString Compilation = DatabaseSchema::DataType::Text;
                inline static const QString Grouping = DatabaseSchema::DataType::Text;
                inline static const QString Genre = DatabaseSchema::DataType::Text;
                inline static const QString ISRC = DatabaseSchema::DataType::Text;
                inline static const QString Copyright = DatabaseSchema::DataType::Text;
                inline static const QString AlbumReplayGain = DatabaseSchema::DataType::Precision;
                inline static const QString AlbumReplayGainPeak = DatabaseSchema::DataType::Precision;
                inline static const QString RecordedDate = DatabaseSchema::DataType::Number;
                inline static const QString AudioURL = DatabaseSchema::DataType::Text;
                inline static const QString CoverURL = DatabaseSchema::DataType::Text;
                inline static const QString Comment = DatabaseSchema::DataType::Text;
                inline static const QString ReleaseCountry = DatabaseSchema::DataType::Text;
                inline static const QString Owner = DatabaseSchema::DataType::Text;

                static const QList<QString> getAllDataTypes() {
                    return QList<QString>(
                        { Id, Album, AlbumPerformer, Performer, TrackName, TrackNumber, TrackPerformer, Lyricist, Publisher, Label, Compilation, Grouping, Genre,
                        ISRC, Copyright, AlbumReplayGain, AlbumReplayGainPeak, RecordedDate, AudioURL, CoverURL, Comment, ReleaseCountry, Owner }
                    );
                }
            };
        }
        namespace User {
            const QString Title = "User";
            static const struct Field {
                inline static const QString Id = DatabaseSchema::Table::General::Field::Id;
                inline static const QString Notes = "User Notes";
                inline static const QString Rating = "User Rating";
                inline static const QString Tags = "Tags";
                inline static const QString LastOpened = "Last Opened";
                inline static const QString OpenCount = "Open Count";

                static const QList<QString> getAllFields() {
                    return QList<QString>(
                        { Id, Notes, Rating, Tags, LastOpened, OpenCount }
                    );
                }
            };
            static const struct DataType {
                inline static const QString Id = DatabaseSchema::DataType::Text + DatabaseSchema::DataType::NotNull;
                inline static const QString Notes = DatabaseSchema::DataType::Text;
                inline static const QString Rating = DatabaseSchema::DataType::Number;
                inline static const QString Tags = DatabaseSchema::DataType::Text;
                inline static const QString LastOpened = DatabaseSchema::DataType::Number;
                inline static const QString OpenCount = DatabaseSchema::DataType::Number;

                static const QList<QString> getAllDataTypes() {
                    return QList<QString>(
                        { Id, Notes, Rating, Tags, LastOpened, OpenCount }
                    );
                }
            };
        }
        namespace Settings {
            const QString Title = DatabaseSchema::Tables::Settings;
            static const struct Field {
                inline static const QString MediaItemSize = "Media Item Size";
                inline static const QString LastViewed = "Last Viewed";

                static const QList<QString> getAllFields() {
                    return QList<QString>(
                        { MediaItemSize, LastViewed }
                    );
                }
            };
            static const struct DataType {
                inline static const QString MediaItemSize = DatabaseSchema::DataType::Number;
                inline static const QString LastViewed = DatabaseSchema::DataType::Text;

                static const QList<QString> getAllDataTypes() {
                    return QList<QString>(
                        { MediaItemSize, LastViewed }
                    );
                }
            };
        }
    }
    static const QList<QString> getAllFieldsFor(QString table) {
        if (DatabaseSchema::Tables::General == table)
            return DatabaseSchema::Table::General::Field::getAllFields();
        if (DatabaseSchema::Tables::Graphic == table)
            return DatabaseSchema::Table::Graphic::Field::getAllFields();
        if (DatabaseSchema::Tables::Audio == table)
            return DatabaseSchema::Table::Audio::Field::getAllFields();
        if (DatabaseSchema::Tables::Music == table)
            return DatabaseSchema::Table::Music::Field::getAllFields();
        if (DatabaseSchema::Tables::User == table)
            return DatabaseSchema::Table::User::Field::getAllFields();
        if (DatabaseSchema::Tables::Settings == table)
            return DatabaseSchema::Table::Settings::Field::getAllFields();
    }
    static const QList<QString> getAllDataTypesFor(QString table) {
        if (DatabaseSchema::Tables::General == table)
            return DatabaseSchema::Table::General::DataType::getAllDataTypes();
        if (DatabaseSchema::Tables::Graphic == table)
            return DatabaseSchema::Table::Graphic::DataType::getAllDataTypes();
        if (DatabaseSchema::Tables::Audio == table)
            return DatabaseSchema::Table::Audio::DataType::getAllDataTypes();
        if (DatabaseSchema::Tables::Music == table)
            return DatabaseSchema::Table::Music::DataType::getAllDataTypes();
        if (DatabaseSchema::Tables::User == table)
            return DatabaseSchema::Table::User::DataType::getAllDataTypes();
        if (DatabaseSchema::Tables::Settings == table)
            return DatabaseSchema::Table::Settings::DataType::getAllDataTypes();
    }
}

inline namespace MEDIA_INFO
{
    static const struct GENERAL {
        inline static const std::wstring const AUDIO_LANGUAGE_LIST = L"Audio_Language_List";
        inline static const std::wstring const FILE_PATH = L"Complete name";
        inline static const std::wstring const FORMAT = L"Format";
        inline static const std::wstring const FORMAT_PROFILE = L"Format profile";
        inline static const std::wstring const COMMON_FORMATS = L"Format/Extensions usually used";
        inline static const std::wstring const MIME_TYPE = L"Internet media type";
        inline static const std::wstring const CODEC_ID = L"Codec ID";
        inline static const std::wstring const FILE_SIZE = L"File size";
        inline static const std::wstring const DURATION = L"Duration";
        inline static const std::wstring const OVERALL_BIT_RATE_MODE = L"Overall bit rate mode";
        inline static const std::wstring const OVERALL_BIT_RATE = L"Overall bit rate";
        inline static const std::wstring const FRAME_RATE = L"Frame rate";
        inline static const std::wstring const FRAME_COUNT = L"Frame count";
        inline static const std::wstring const STREAM_SIZE = L"Stream size";
        inline static const std::wstring const STREAMABLE = L"IsStreamable";
        inline static const std::wstring const ENCODED_DATE = L"Encoded date";
        inline static const std::wstring const TAGGED_DATE = L"Tagged date";
        inline static const std::wstring const FILE_CREATION_DATE = L"File creation date";
        inline static const std::wstring const FILE_LAST_MOD_DATE = L"File last modification date";
        // Music
        inline static const std::wstring const ALBUM = L"Album";
        inline static const std::wstring const ALBUM_PERFORMER = L"Album/Performer";
        inline static const std::wstring const ALBUM_REPLAY_GAIN = L"Album replay gain";
        inline static const std::wstring const ALBUM_REPLAY_GAIN_PEAK = L"Album replay gain peak";
        inline static const std::wstring const LABEL = L"Label";
        inline static const std::wstring const PART = L"Part";
        inline static const std::wstring const TRACK_NAME = L"Track name";
        inline static const std::wstring const TRACK_NUMBER = L"Track name/Position";
        inline static const std::wstring const GROUPING = L"Grouping";
        inline static const std::wstring const COMPILATION = L"Compilation";
        inline static const std::wstring const PERFORMER = L"Performer";
        inline static const std::wstring const PERFORMER_URL = L"Performer/Url";
        inline static const std::wstring const LYRICIST = L"Lyricist";
        inline static const std::wstring const ENCODED_BY = L"Encoded by";
        inline static const std::wstring const PUBLISHER = L"Publisher";
        inline static const std::wstring const GENRE = L"Genre";
        inline static const std::wstring const RECORDED_DATE = L"Recorded date";
        inline static const std::wstring const WRITING_LIBRARY = L"Writing library";
        inline static const std::wstring const ISRC = L"ISRC";
        inline static const std::wstring const COPYRIGHT = L"Copyright";
        inline static const std::wstring const COVER = L"Cover";
        inline static const std::wstring const COVER_TYPE = L"Cover type";
        inline static const std::wstring const COVER_MIME = L"Cover MIME";
        inline static const std::wstring const LYRICS = L"Lyrics";
        inline static const std::wstring const COMMENT = L"Comment";
        inline static const std::wstring const RELEASE_COUNTRY = L"RELEASECOUNTRY";
        inline static const std::wstring const RATING = L"Rating";
        inline static const std::wstring const OWNR = L"ownr";
        inline static const std::wstring const WEBPAGE = L"Official audio file webpage";
        inline static const std::wstring const WEBPAGE_SOURCE = L"Official audio source webpage";

        static const int param_count = 49;

        static const std::array<const std::wstring const, param_count> getAllParams() {
            return std::array<const std::wstring const, param_count> {
                FILE_PATH, FORMAT, FORMAT_PROFILE, COMMON_FORMATS, MIME_TYPE, CODEC_ID, FILE_SIZE, DURATION, OVERALL_BIT_RATE_MODE,
                    OVERALL_BIT_RATE, FRAME_RATE, FRAME_COUNT, STREAM_SIZE, STREAMABLE, ENCODED_DATE, TAGGED_DATE, FILE_CREATION_DATE,
                    FILE_LAST_MOD_DATE, ALBUM, ALBUM_PERFORMER, ALBUM_REPLAY_GAIN, ALBUM_REPLAY_GAIN_PEAK, LABEL, PART, TRACK_NAME,
                    TRACK_NUMBER, GROUPING, COMPILATION, PERFORMER, PERFORMER_URL, LYRICIST, ENCODED_BY, PUBLISHER, GENRE, RECORDED_DATE,
                    WRITING_LIBRARY, ISRC, COPYRIGHT, COVER, COVER_TYPE, COVER_MIME, LYRICS, COMMENT, RELEASE_COUNTRY, RATING, OWNR,
                    WEBPAGE, WEBPAGE_SOURCE
            };
        }
        static const bool paramExist(const std::wstring str) {
            for (auto& info : getAllParams())
                //if (strcmp(info, str) == 0)
                if (info == str)
                    return true;
            return false;
        }
        static const int getPreferredAlt(const std::wstring str) {
            enum PreferredAlt {
                FILE_PATH = 0, FORMAT = 1, FORMAT_PROFILE = 0, COMMON_FORMATS = 0, MIME_TYPE = 0, CODEC_ID = 0, FILE_SIZE = 0, DURATION = 0, OVERALL_BIT_RATE_MODE = 1,
                OVERALL_BIT_RATE = 0, FRAME_RATE = 0, FRAME_COUNT = 0, STREAM_SIZE = 0, STREAMABLE = 0, ENCODED_DATE = 0, TAGGED_DATE = 0, FILE_CREATION_DATE = 0,
                FILE_LAST_MOD_DATE = 0, ALBUM = 0, ALBUM_PERFORMER = 0, ALBUM_REPLAY_GAIN = 0, ALBUM_REPLAY_GAIN_PEAK = 0, LABEL = 0, PART = 0, TRACK_NAME = 0,
                TRACK_NUMBER = 0, GROUPING = 0, COMPILATION = 0, PERFORMER = 0, PERFORMER_URL = 0, LYRICIST = 0, ENCODED_BY = 0, PUBLISHER = 0, GENRE = 0, RECORDED_DATE = 0,
                WRITING_LIBRARY = 0, ISRC = 0, COPYRIGHT = 0, COVER = 0, COVER_TYPE = 0, COVER_MIME = 0, LYRICS = 0, COMMENT = 0, RELEASE_COUNTRY = 0, RATING = 0, OWNR = 0,
                WEBPAGE = 0, WEBPAGE_SOURCE = 0
            };
            const std::array<int, param_count> preferred_param_alts{
                FILE_PATH, FORMAT, FORMAT_PROFILE, COMMON_FORMATS, MIME_TYPE, CODEC_ID, FILE_SIZE, DURATION, OVERALL_BIT_RATE_MODE,
                OVERALL_BIT_RATE, FRAME_RATE, FRAME_COUNT, STREAM_SIZE, STREAMABLE, ENCODED_DATE, TAGGED_DATE, FILE_CREATION_DATE,
                FILE_LAST_MOD_DATE, ALBUM, ALBUM_PERFORMER, ALBUM_REPLAY_GAIN, ALBUM_REPLAY_GAIN_PEAK, LABEL, PART, TRACK_NAME,
                TRACK_NUMBER, GROUPING, COMPILATION, PERFORMER, PERFORMER_URL, LYRICIST, ENCODED_BY, PUBLISHER, GENRE, RECORDED_DATE,
                WRITING_LIBRARY, ISRC, COPYRIGHT, COVER, COVER_TYPE, COVER_MIME, LYRICS, COMMENT, RELEASE_COUNTRY, RATING, OWNR,
                WEBPAGE, WEBPAGE_SOURCE
            };
            int preferred_param_alt_index = -1;
            for (auto& info : getAllParams()) {
                preferred_param_alt_index++;
                if (info == str)
                    return preferred_param_alts.at(preferred_param_alt_index);
            }
            return 0;
        }
    };
    static const struct VIDEO {
        inline static const std::wstring const ID = L"ID";
        inline static const std::wstring const FORMAT = L"Format";
        inline static const std::wstring const FORMAT_INFO = L"Format/Info";
        inline static const std::wstring const FORMAT_URL = L"Format/Url";
        inline static const std::wstring const FORMAT_PROFILE = L"Format profile";
        inline static const std::wstring const FORMAT_SETTINGS = L"Format settings";
        //inline static const std::wstring const FORMAT_SETTINGS_CABAC = L"Format settings, CABAC"; // These are custom fields split off from "Format settings"
        //inline static const std::wstring const FORMAT_SETTINGS_FRAMES = L"Format settings, Reference frames";
        inline static const std::wstring const CODEC_ID = L"Codec ID";
        inline static const std::wstring const CODEC_ID_INFO = L"Codec ID/Info";
        inline static const std::wstring const DURATION = L"Duration";
        inline static const std::wstring const BIT_RATE = L"Bit rate";
        inline static const std::wstring const WIDTH = L"Width";
        inline static const std::wstring const HEIGHT = L"Height";
        inline static const std::wstring const DISPLAY_ASPECT_RATIO = L"Display aspect ratio";
        inline static const std::wstring const ROTATION = L"Rotation";
        inline static const std::wstring const FRAME_RATE_MODE = L"Frame rate mode";
        inline static const std::wstring const FRAME_RATE = L"Frame rate";              // FPS can be gotten with math [Duration/1000 / Frame count]
        inline static const std::wstring const FRAME_COUNT = L"Frame count";
        inline static const std::wstring const COLOR_SPACE = L"Color space";
        inline static const std::wstring const CHROMA_SUBSAMPLING = L"Chroma subsampling";
        inline static const std::wstring const BIT_DEPTH = L"Bit depth";
        inline static const std::wstring const SCAN_TYPE = L"Scan type";
        inline static const std::wstring const BITS_PER_FRAME = L"Bits/(Pixel*Frame)"; // This number can be gotten with math [BIT_RATE / (WIDTH * HEIGHT * FRAME_RATE)]
        inline static const std::wstring const STREAM_SIZE = L"Stream size";
        inline static const std::wstring const LANGUAGE = L"Language";
        inline static const std::wstring const TITLE = L"Title";
        inline static const std::wstring const WRITING_LIBRARY = L"Writing library";
        inline static const std::wstring const ENCODING_SETTINGS = L"Encoding settings";
        inline static const std::wstring const COLOR_RANGE = L"Color range";
        inline static const std::wstring const COLOR_PRIMARIES = L"Color primaries";
        inline static const std::wstring const TRANSFER_CHARACTERISTICS = L"Transfer characteristics";
        inline static const std::wstring const MATRIX_COEFFICIENTS = L"Matrix coefficients";
        inline static const std::wstring const ENCODED_DATE = L"Encoded date";
        inline static const std::wstring const TAGGED_DATE = L"Tagged date";
        inline static const std::wstring const CODEX_CONFIG_BOX = L"Codec configuration box";

        static const int param_count = 34;

        static const std::array<const std::wstring const, param_count> getAllParams() {
            return std::array<const std::wstring const, param_count> {
                ID, FORMAT, FORMAT_INFO, FORMAT_URL, FORMAT_PROFILE, FORMAT_SETTINGS, CODEC_ID, CODEC_ID_INFO, DURATION, BIT_RATE,
                    WIDTH, HEIGHT, DISPLAY_ASPECT_RATIO, ROTATION, FRAME_RATE_MODE, FRAME_RATE, FRAME_COUNT, COLOR_SPACE,
                    CHROMA_SUBSAMPLING, BIT_DEPTH, SCAN_TYPE, BITS_PER_FRAME, STREAM_SIZE, LANGUAGE, TITLE, WRITING_LIBRARY,
                    ENCODING_SETTINGS, COLOR_RANGE, COLOR_PRIMARIES, TRANSFER_CHARACTERISTICS, MATRIX_COEFFICIENTS, ENCODED_DATE,
                    TAGGED_DATE, CODEX_CONFIG_BOX
            };
        }
        static const bool paramExist(const std::wstring str) {
            for (auto& info : getAllParams())
                if (info == str)
                    return true;
            return false;
        }
        static const int getPreferredAlt(const std::wstring str) {
            enum PreferredAlt {
                ID = 0, FORMAT = 1, FORMAT_INFO = 0, FORMAT_URL = 0, FORMAT_PROFILE = 0, FORMAT_SETTINGS = 0, CODEC_ID = 0, CODEC_ID_INFO = 0, DURATION = 0, BIT_RATE = 0,
                WIDTH = 0, HEIGHT = 0, DISPLAY_ASPECT_RATIO = 1, ROTATION = 0, FRAME_RATE_MODE = 1, FRAME_RATE = 0, FRAME_COUNT = 0, COLOR_SPACE = 0,
                CHROMA_SUBSAMPLING = 0, BIT_DEPTH = 0, SCAN_TYPE = 0, BITS_PER_FRAME = 0, STREAM_SIZE = 0, LANGUAGE = 0, TITLE = 0, WRITING_LIBRARY = 0,
                ENCODING_SETTINGS = 0, COLOR_RANGE = 0, COLOR_PRIMARIES = 0, TRANSFER_CHARACTERISTICS = 0, MATRIX_COEFFICIENTS = 0, ENCODED_DATE = 0,
                TAGGED_DATE = 0, CODEX_CONFIG_BOX = 0
            };
            const std::array<int, param_count> preferred_param_alts{
                ID, FORMAT, FORMAT_INFO, FORMAT_URL, FORMAT_PROFILE, FORMAT_SETTINGS,CODEC_ID, CODEC_ID_INFO, DURATION, BIT_RATE,
                WIDTH, HEIGHT, DISPLAY_ASPECT_RATIO, ROTATION, FRAME_RATE_MODE, FRAME_RATE, FRAME_COUNT, COLOR_SPACE,
                CHROMA_SUBSAMPLING, BIT_DEPTH, SCAN_TYPE, BITS_PER_FRAME, STREAM_SIZE, LANGUAGE, TITLE, WRITING_LIBRARY,
                ENCODING_SETTINGS, COLOR_RANGE, COLOR_PRIMARIES, TRANSFER_CHARACTERISTICS, MATRIX_COEFFICIENTS, ENCODED_DATE,
                TAGGED_DATE, CODEX_CONFIG_BOX
            };
            int preferred_param_alt_index = -1;
            for (auto& info : getAllParams()) {
                preferred_param_alt_index++;
                if (info == str)
                    return preferred_param_alts.at(preferred_param_alt_index);
            }
            return 0;
        }
    };
    static const struct AUDIO {
        inline static const std::wstring const ID = L"ID";
        inline static const std::wstring const FORMAT = L"Format";
        inline static const std::wstring const FORMAT_INFO = L"Format/Info";
        inline static const std::wstring const FORMAT_VERSION = L"Format version";
        inline static const std::wstring const FORMAT_PROFILE = L"Format profile";
        inline static const std::wstring const FORMAT_SETTINGS = L"Format settings";
        inline static const std::wstring const CODEC_ID = L"Codec ID";
        inline static const std::wstring const DURATION = L"Duration";
        inline static const std::wstring const BIT_RATE_MODE = L"Bit rate mode";
        inline static const std::wstring const BIT_RATE = L"Bit rate";
        inline static const std::wstring const MAX_BIT_RATE = L"Maximum bit rate";
        inline static const std::wstring const CHANNELS = L"Channel(s)";
        inline static const std::wstring const CHANNEL_POSITIONS = L"Channel positions";
        inline static const std::wstring const CHANNELS_LAYOUT = L"Channel layout";
        inline static const std::wstring const SAMPLING_RATE = L"Sampling rate";
        inline static const std::wstring const SAMPLING_COUNT = L"Samples count";
        inline static const std::wstring const FRAME_RATE = L"Frame rate";              // FPS and Samples per frame can be gotten with math [Duration/1000 or Samples count / Frame count]
        inline static const std::wstring const FRAME_COUNT = L"Frame count";
        inline static const std::wstring const COMPRESSION_MODE = L"Compression mode";
        inline static const std::wstring const REPLAY_GAIN = L"Replay gain";
        inline static const std::wstring const REPLAY_GAIN_PEAK = L"Replay gain peak";
        inline static const std::wstring const STREAM_SIZE = L"Stream size";
        inline static const std::wstring const TITLE = L"Title";
        inline static const std::wstring const LANGUAGE = L"Language";
        inline static const std::wstring const DEFAULT = L"Default"; // Is this the default stream/track id?
        inline static const std::wstring const ALTERNATE_GROUP = L"Alternate group";
        inline static const std::wstring const ENCODED_DATE = L"Encoded date";
        inline static const std::wstring const TAGGED_DATE = L"Tagged date";
        inline static const std::wstring const WRITING_LIBRARY = L"Writing library";
        inline static const std::wstring const MD5 = L"MD5 of the unencoded content";

        static const int param_count = 30;

        static const std::array<const std::wstring const, param_count> getAllParams() {
            return std::array<const std::wstring const, param_count> {
                ID, FORMAT, FORMAT_INFO, FORMAT_VERSION, FORMAT_PROFILE, FORMAT_SETTINGS, CODEC_ID, DURATION, BIT_RATE_MODE, BIT_RATE,
                    MAX_BIT_RATE, CHANNELS, CHANNEL_POSITIONS, CHANNELS_LAYOUT, SAMPLING_RATE, SAMPLING_COUNT, FRAME_RATE, FRAME_COUNT,
                    COMPRESSION_MODE, REPLAY_GAIN, REPLAY_GAIN_PEAK, STREAM_SIZE, TITLE, LANGUAGE, DEFAULT, ALTERNATE_GROUP,
                    ENCODED_DATE, TAGGED_DATE, WRITING_LIBRARY, MD5
            };
        }
        static const bool paramExist(const std::wstring str) {
            for (auto& info : getAllParams())
                if (info == str)
                    return true;
            return false;
        }
        static const int getPreferredAlt(const std::wstring str) {
            enum PreferredAlt {
                ID = 0, FORMAT = 1, FORMAT_INFO = 0, FORMAT_VERSION = 0, FORMAT_PROFILE = 0, FORMAT_SETTINGS = 0, CODEC_ID = 0, DURATION = 0, BIT_RATE_MODE = 1, BIT_RATE = 0,
                MAX_BIT_RATE = 0, CHANNELS = 0, CHANNEL_POSITIONS = 0, CHANNELS_LAYOUT = 0, SAMPLING_RATE = 0, SAMPLING_COUNT = 0, FRAME_RATE = 0, FRAME_COUNT = 0,
                COMPRESSION_MODE = 0, REPLAY_GAIN = 0, REPLAY_GAIN_PEAK = 0, STREAM_SIZE = 0, TITLE = 0, LANGUAGE = 1, DEFAULT = 0, ALTERNATE_GROUP = 0,
                ENCODED_DATE = 0, TAGGED_DATE = 0, WRITING_LIBRARY = 0, MD5 = 0
            };
            const std::array<int, param_count> preferred_param_alts{
                ID, FORMAT, FORMAT_INFO, FORMAT_VERSION, FORMAT_PROFILE, FORMAT_SETTINGS, CODEC_ID, DURATION, BIT_RATE_MODE, BIT_RATE,
                MAX_BIT_RATE, CHANNELS, CHANNEL_POSITIONS, CHANNELS_LAYOUT, SAMPLING_RATE, SAMPLING_COUNT, FRAME_RATE, FRAME_COUNT,
                COMPRESSION_MODE, REPLAY_GAIN, REPLAY_GAIN_PEAK, STREAM_SIZE, TITLE, LANGUAGE, DEFAULT, ALTERNATE_GROUP,
                ENCODED_DATE, TAGGED_DATE, WRITING_LIBRARY, MD5
            };
            int preferred_param_alt_index = -1;
            for (auto& info : getAllParams()) {
                preferred_param_alt_index++;
                if (info == str)
                    return preferred_param_alts.at(preferred_param_alt_index);
            }
            return 0;
        }
    };
    static const struct IMAGE {
        inline static const std::wstring const FORMAT = L"Format";
        inline static const std::wstring const WIDTH = L"Width";
        inline static const std::wstring const HEIGHT = L"Height";
        inline static const std::wstring const COLOR_SPACE = L"Color space";
        inline static const std::wstring const CHROMA_SUBSAMPLING = L"Chroma subsampling";
        inline static const std::wstring const BIT_DEPTH = L"Bit depth";
        inline static const std::wstring const COMPRESSION_MODE = L"Compression mode";
        inline static const std::wstring const STREAM_SIZE = L"Stream size";

        static const int param_count = 8;

        static const std::array<const std::wstring const, param_count> getAllParams() {
            return std::array<const std::wstring const, param_count> {
                FORMAT, WIDTH, HEIGHT, COLOR_SPACE, CHROMA_SUBSAMPLING, BIT_DEPTH, COMPRESSION_MODE, STREAM_SIZE
            };
        }
        static const bool paramExist(const std::wstring str) {
            for (auto& info : getAllParams())
                if (info == str)
                    return true;
            return false;
        }
        static const int getPreferredAlt(const std::wstring str) {
            enum PreferredAlt {
                FORMAT = 0, WIDTH = 0, HEIGHT = 0, COLOR_SPACE = 0, CHROMA_SUBSAMPLING = 0, BIT_DEPTH = 0, COMPRESSION_MODE = 0, STREAM_SIZE = 0
            };
            const std::array<int, param_count> preferred_param_alts{
                FORMAT, WIDTH, HEIGHT, COLOR_SPACE, CHROMA_SUBSAMPLING, BIT_DEPTH, COMPRESSION_MODE, STREAM_SIZE
            };
            int preferred_param_alt_index = -1;
            for (auto& info : getAllParams()) {
                preferred_param_alt_index++;
                if (info == str)
                    return preferred_param_alts.at(preferred_param_alt_index);
            }
            return 0;
        }
    };
    //static const struct TEXT { // TODO: Subtitle files?

    static const QString print(const MediaInfoDLL::stream_t type) {
        switch (type) {
        case MediaInfoDLL::stream_t::Stream_General:
            return "General";
        case MediaInfoDLL::stream_t::Stream_Video:
            return "Video";
        case MediaInfoDLL::stream_t::Stream_Audio:
            return "Audio";
        case MediaInfoDLL::stream_t::Stream_Text:
            return "Text";
        case MediaInfoDLL::stream_t::Stream_Other:
            return "Other";
        case MediaInfoDLL::stream_t::Stream_Image:
            return "Image";
        case MediaInfoDLL::stream_t::Stream_Menu:
            return "Menu";
        defualt:
            return "Unknown";
        }
    };
    static const int getPreferredAlt(const std::wstring str, const MediaInfoDLL::stream_t type) {
        switch (type) {
        case MediaInfoDLL::stream_t::Stream_General:
            return MEDIA_INFO::GENERAL::getPreferredAlt(str);
        case MediaInfoDLL::stream_t::Stream_Video:
            return MEDIA_INFO::VIDEO::getPreferredAlt(str);
        case MediaInfoDLL::stream_t::Stream_Audio:
            return MEDIA_INFO::AUDIO::getPreferredAlt(str);
        case MediaInfoDLL::stream_t::Stream_Text:
            return 0;
        case MediaInfoDLL::stream_t::Stream_Other:
            return 0;
        case MediaInfoDLL::stream_t::Stream_Image:
            return MEDIA_INFO::IMAGE::getPreferredAlt(str);
        case MediaInfoDLL::stream_t::Stream_Menu:
            return 0;
        defualt:
            return 0;
        }
    };
}
