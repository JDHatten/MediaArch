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
#include "MediaViewer.h"

class UserSettings
{

private:

    int UserId = 0;
    QSqlDatabase database;
    QSqlRecord user_record;
    
    bool CreateNewUserSettingsRecord();
    bool DeleteUserSettings();

public:

    UserSettings(int user_id);
    ~UserSettings();

    void loadUserSettings();
    bool saveUserSettings();// All Fields
    bool saveUserSettings(QString field);
    bool saveUserSettings(QList<QString> fields);

    QVariant getUserSetting(QString field) const;
    bool setUserSetting(QString field, QVariant value, bool save_user_settings = false);
    void printSettings() const;

};

#endif // USERSETTINGS_H