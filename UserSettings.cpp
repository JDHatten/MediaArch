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
            .arg(DatabaseSchema::Table::Settings::Field::UserId)
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
}

bool UserSettings::CreateNewUserSettingsRecord()
{
    QString insert = QString("INSERT INTO \"%1\" (\"%2\") VALUES (%3)")
        .arg(DatabaseSchema::Tables::Settings)
        .arg(DatabaseSchema::Table::Settings::Field::UserId)
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
    return saveUserSettings(DatabaseSchema::Table::Settings::Field::getAllFields());
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
            if (field == DatabaseSchema::Table::Settings::Field::UserId)
                continue; // Skip updating UserId
            QVariant qval = getUserSetting(field);
            if (qval.typeId() == QMetaType::QString)
                update_list.push_back(QString("\"%1\" = '%2'").arg(field).arg(qval.toString())); // string
            else
                update_list.push_back(QString("\"%1\" = %2").arg(field).arg(qval.toString())); // number or some other kind of data
        }
        update += update_list.join(", ");
        update += QString(" WHERE \"%1\" = %2")
            .arg(DatabaseSchema::Table::Settings::Field::UserId)
            .arg(UserId);
        //qDebug() << update;
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
    for (auto& field : DatabaseSchema::Table::Settings::Field::getAllFields()) {
        QVariant qval = getUserSetting(field);
        qDebug() << field << ":" << qval;
    }
}
