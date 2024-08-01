#include "UserSettings.h"

UserSettings::UserSettings(int used_id) : UserId(used_id)
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
    //DeleteUserSettings();
    if (database.open()) {
       QString select = QString("SELECT * FROM \"%1\" WHERE \"%2\" = %3")
            .arg(DatabaseSchema::Tables::Settings)
            .arg(Field::Settings::UserId)
            .arg(UserId);
        QSqlQuery query(database);
        query.prepare(select);
        if (query.exec()) {
            if (query.next()) {
                user_record = query.record();
            }
            else {
                qDebug() << "No User Settings Found for Id:" << UserId << ", Creating New User..." << query.lastQuery();
                if (CreateNewUserSettingsRecord())
                    loadUserSettings(); // try again to get user_record
                else
                    qDebug() << "Failed to Create New User:" << UserId;
            }
        }
        else {
            qDebug() << "ERROR: Query Failed:" << query.lastError();
            qDebug() << "ERROR:" << query.lastQuery();
        }
        query.finish();
    }
    database.close();

    // Convert "DirectoryListHistory" String to StringList
    if (user_record.contains(Field::Settings::DirectoryListHistory)) {
        QString directory_history_str = user_record.value(Field::Settings::DirectoryListHistory).toString();
        QStringList directory_list_history;
        if (directory_history_str.length()) {
            directory_list_history = directory_history_str.split("|");
        }
        setUserSetting(Field::Settings::DirectoryListHistory, directory_list_history);
    }
}

bool UserSettings::CreateNewUserSettingsRecord()
{
    QString insert = QString("INSERT INTO \"%1\" (\"%2\") VALUES (%3)")
        .arg(DatabaseSchema::Tables::Settings)
        .arg(Field::Settings::UserId)
        .arg(UserId);
    //qDebug() << insert;
    QSqlQuery query(database);
    query.prepare(insert);
    if (query.exec()) {
        return true;
    }
    else {
        qDebug() << "ERROR: Query Failed:" << query.lastError();
        qDebug() << "ERROR:" << query.lastQuery();
    }
    return false;
}

bool UserSettings::saveUserSettings()
{
    return saveUserSettings(Field::Settings::getAllFields());
}
bool UserSettings::saveUserSettings(QString field)
{
    return saveUserSettings(QList<QString>(field));
}
bool UserSettings::saveUserSettings(QList<QString> fields)
{
    if (database.open()) {
        QList<QString> update_list;
        QString update = QString("UPDATE \"%1\" SET ").arg(DatabaseSchema::Tables::Settings);
        for (auto& field : fields) {
            if (field == Field::Settings::UserId)
                continue; // Skip updating UserId
            QVariant qval = getUserSetting(field);
            if (qval.typeId() == QMetaType::QString) {
                //QString str = qval.toString().replace("'", "\'"); // Sterilize
                update_list.push_back(QString("\"%1\" = \"%2\"").arg(field).arg(qval.toString())); // string
            }
            else if (qval.typeId() == QMetaType::QStringList) {
                // Save StringList as one String
                QString list_to_str = qval.toStringList().join("|");
                update_list.push_back(QString("\"%1\" = \"%2\"").arg(field).arg(list_to_str)); // data / blob
            }
            else
                update_list.push_back(QString("\"%1\" = %2").arg(field).arg(qval.toString())); // number or some other kind of data
        }
        update += update_list.join(", ");
        update += QString(" WHERE \"%1\" = %2")
            .arg(Field::Settings::UserId)
            .arg(UserId);
        QSqlQuery query(database);
        query.prepare(update);
        if (query.exec()) {
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

bool UserSettings::DeleteUserSettings()
{
    if (database.open()) {
        QString delete_from = QString("DELETE FROM \"%1\"").arg(DatabaseSchema::Tables::Settings);
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

QVariant UserSettings::getUserSetting(QString field) const
{
    return user_record.value(field);
}
bool UserSettings::setUserSetting(QString field, QVariant value, bool save_user_settings)
{
    if (user_record.contains(field)) {
        user_record.setValue(field, value);
        if (save_user_settings)
            saveUserSettings();
        return true;
    }
    return false;
}

void UserSettings::printSettings() const
{
    for (auto& field : Field::Settings::getAllFields()) {
        QVariant qval = getUserSetting(field);
        qDebug() << field << ":" << qval;
    }
}
