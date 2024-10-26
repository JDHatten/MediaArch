#include "UserSettings.h"

UserSettings::UserSettings(int used_id) : user_id(used_id)
{
    QString db_name = QString(DatabaseSchema::Database::Name) + QString("-%1").arg(rand() % INT16_MAX);
    database = QSqlDatabase::cloneDatabase(DatabaseSchema::Database::ConnectionName, db_name);
    loadUserSettings();
}

UserSettings::~UserSettings() {
    database.close();
}

void UserSettings::loadUserSettings()
{
    if (database.open()) {
       QString select = QString("SELECT * FROM \"%1\" WHERE \"%2\" = %3")
            .arg(Table::Settings)
            .arg(Field::Settings::UserId)
            .arg(user_id);
        QSqlQuery query(database);
        query.prepare(select);
        if (query.exec()) {
            if (query.next()) {
                user_record = query.record();
            }
            else {
                qDebug() << "No User Settings Found for Id:" << user_id << ", Creating New User..." << query.lastQuery();
                if (CreateNewUserSettingsRecord())
                    loadUserSettings(); // try again to get user_record
                else
                    qDebug() << "Failed to Create New User:" << user_id;
            }
        }
        else {
            qDebug() << "ERROR: Query Failed:" << query.lastError();
            qDebug() << "ERROR:" << query.lastQuery();
        }
        query.finish();
    }
    database.close();

    // Convert String Blobs to String Lists
    QStringList string_blob_fields = {
        Field::Settings::DirectoryListHistory,
        Field::Settings::ColumnLabelsTable,
        Field::Settings::ColumnLabelsField,
        Field::Settings::ListOfPlaylists,
        Field::Settings::ListOfPlaylistFolders
    };
    for (auto& field : string_blob_fields) {
        if (user_record.contains(field)) {
            QString string_blob = user_record.value(field).toString();
            QStringList string_list;
            if (string_blob.length()) {
                string_list = string_blob.split("|");
            }
            user_record.setValue(field, string_list);
        }
    }
}

bool UserSettings::CreateNewUserSettingsRecord()
{
    QString insert = QString("INSERT INTO \"%1\" (\"%2\") VALUES (%3)")
        .arg(Table::Settings)
        .arg(Field::Settings::UserId)
        .arg(user_id);
    QSqlQuery query(database);
    query.prepare(insert);
    if (query.exec()) {
        return true;
    }
    else {
        qWarning() << "ERROR: Query Failed:" << query.lastError();
        qWarning() << "ERROR:" << query.lastQuery();
    }
    return false;
}

bool UserSettings::saveUserSettings()
{
    if (areSettingsUnsaved()) {
        if (database.open()) {
            QStringList update_list;
            QHash<QString, QByteArray> byte_array_settings;
            int byte_array_settings_count = 0;
            QString update = QString("UPDATE \"%1\" SET ").arg(Table::Settings);
            for (auto unsaved_setting = unsaved_settings.cbegin(), end = unsaved_settings.cend(); unsaved_setting != end; ++unsaved_setting) {
                QString field = unsaved_setting.key();
                if (field == Field::Settings::UserId)
                    continue; // Skip updating UserId
                QVariant value = unsaved_setting.value();
                if (value.typeId() == QMetaType::QString) {
                    //QString str = value.toString().replace("'", "\'"); // Sterilize
                    update_list.push_back(QString("\"%1\" = \"%2\"").arg(field).arg(value.toString())); // string
                }
                else if (value.typeId() == QMetaType::QStringList) {
                    // Save StringList as one String
                    QString list_to_str = value.toStringList().join("|");
                    update_list.push_back(QString("\"%1\" = \"%2\"").arg(field).arg(list_to_str)); // string / blob
                }
                else if (value.typeId() == QMetaType::QByteArray) {
                    QString settings_var = QString(":setting_%1").arg(++byte_array_settings_count);
                    byte_array_settings.emplace(settings_var, value.toByteArray());
                    update_list.push_back(QString("\"%1\" = %2").arg(field).arg(settings_var)); // data / blob
                }
                else
                    update_list.push_back(QString("\"%1\" = %2").arg(field).arg(value.toString())); // number or some other kind of data
            }
            update += update_list.join(", ");
            update += QString(" WHERE \"%1\" = %2")
                .arg(Field::Settings::UserId)
                .arg(user_id);
            QSqlQuery query(database);
            query.prepare(update);

            // Add/Bind QByteArray data
            for (auto setting = byte_array_settings.cbegin(), end = byte_array_settings.cend(); setting != end; ++setting) {
                query.bindValue(setting.key(), setting.value());
            }
            //qDebug() << "Query:" << update;
            if (query.exec()) {
                database.close();
                unsaved_settings.clear();
                loadUserSettings(); // TODO: OR just add unsaved_settings to user_record in loop above? only use loadUserSettings if query failed, because user_record would have "unsaved_settings"
                return true;
            }
            else {
                qWarning() << "ERROR: Query Failed:" << query.lastError();
                qWarning() << "ERROR:" << query.lastQuery();
            }
        }
    }
    else {
        qDebug() << "saveUserSettings() called when there has been no changes made to user settings.";
        return false;
    }
    database.close();
    return false;
}

bool UserSettings::DeleteUserSettings()
{
    if (database.open()) {
        QString delete_from = QString("DELETE FROM \"%1\"").arg(Table::Settings);
        qDebug() << delete_from;
        QSqlQuery query(database);
        query.prepare(delete_from);

        if (query.exec()) {
            user_record.clear();
            database.close();
            return true;
        }
        else {
            qDebug() << "ERROR: Query Failed:" << query.lastError();
            qDebug() << "ERROR:" << query.lastQuery();
        }
    }
    database.close();
    return false;
}

bool UserSettings::areSettingsUnsaved() const
{
    return unsaved_settings.size();
}

bool UserSettings::hasSettingChanged(QString field) const
{
    if (user_record.contains(field) and unsaved_settings.contains(field)) {
        if (user_record.value(field) == unsaved_settings.value(field)) {
            return false;
        }
        return true;
    }
    return false;
}

QVariant UserSettings::getUserSetting(QString field) const
{
    //qDebug() << user_record.value(field);
    return user_record.value(field);
}

QVariant UserSettings::getUnsavedUserSetting(QString field) const
{
    return unsaved_settings.value(field);
}

bool UserSettings::setUserSetting(QString field, QVariant value, bool save_user_settings)
{
    if (user_record.contains(field)) {
        //user_record.setValue(field, value);
        if (user_record.value(field) != value) { // else nothing changed

            if (unsaved_settings.contains(field))
                unsaved_settings.find(field)->setValue(value);
            else
                unsaved_settings.insert(field, value);
            
            if (save_user_settings)
                return saveUserSettings();
        }
        else {
            if (unsaved_settings.contains(field))
                unsaved_settings.remove(field); // value was changed previously, so change it back.
            return false;
        }
        return true;
    }
    qWarning() << "ERROR:" << field << "setting does not exist.";
    return false;
}

void UserSettings::clearUnsavedUserSettings()
{
    unsaved_settings.clear();
}

void UserSettings::printSettings() const
{
    for (auto& field : Field::Settings::getAllFields()) {
        QVariant qval = getUserSetting(field);
        qDebug() << field << ":" << qval;
    }
}
