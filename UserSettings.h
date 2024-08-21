#pragma once
#ifndef USERSETTINGS_H
#define USERSETTINGS_H

#include <QString>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>

#include "MediaSpaces.h"

class UserSettings
{
public:

    UserSettings(int user_id);
    ~UserSettings();

    void loadUserSettings();
    bool saveUserSettings();
    //bool saveUserSettings(QString field);
    //bool saveUserSettings(QStringList fields);
    bool areSettingsUnsaved() const;
    bool hasSettingChanged(QString field) const;
    QVariant getUnsavedUserSetting(QString field) const;
    QVariant getUserSetting(QString field) const;
    bool setUserSetting(QString field, QVariant value, bool save_user_settings = false);
    void printSettings() const;

private:

    int user_id = 0;
    QSqlDatabase database;
    QSqlRecord user_record;
    QVariantHash unsaved_settings;
    QStringList fields_changed;
    
    bool CreateNewUserSettingsRecord();
    bool DeleteUserSettings();

};

#endif // USERSETTINGS_H