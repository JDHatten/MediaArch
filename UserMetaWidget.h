#pragma once
#ifndef USERMETAWIDGET_H
#define USERMETAWIDGET_H

#include <filesystem>

#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
//#include <QResizeEvent>
#include <QScrollArea>
//#include <QScrollBar>
#include <QString>
#include <QTextEdit>
#include <QTime>

#include "MediaSpaces.h"
#include "UserSettings.h"
//#include "MetaDatabaseWorker.h"

class UserMetaWidget : public QWidget
{
    Q_OBJECT

public:

    UserMetaWidget(UserSettings* user_settings, QWidget* parent = Q_NULLPTR);
    ~UserMetaWidget();

    int getStarRating();
    QString getUserNotes();
    void setUserMeta(std::filesystem::path file_path, std::map<QString, QVariant>* user_meta);
    //void changeUserId(int user_id); // TODO

signals:

    void userMetaUpdated(std::filesystem::path file_path, std::map<QString, QVariant>* user_meta);

public slots:

    void setReadyToUse(bool ready);
    void enableUserMetaWidget(bool enabled);
    void fileOpenedUpdateUserMeta(std::filesystem::path file_path, std::map<QString, QVariant>* user_meta);

private slots:

    

private:
    int user_id = 0;
    UserSettings* user_settings = nullptr;
    std::filesystem::path file_path;
    bool is_ready_to_use = false;

    QLabel* file_name_label = nullptr;
    QLabel* star_rating_label = nullptr;
    QPixmap* star_pixmap = nullptr;
    QWidget* date_last_opened_widget = nullptr;
    QLabel* date_last_opened_label = nullptr;
    QLabel* date_last_opened = nullptr;
    QWidget* open_count_widget = nullptr;
    QLabel* open_count_label = nullptr;
    QLabel* open_count = nullptr;
    QTextEdit* user_file_notes = nullptr;
    QPushButton* edit_notes_button = nullptr;

    std::map<QString, QVariant>* user_meta;

    int current_star_count = -1;
    int current_star_count_highlighted = 0;

    void UpdateUserMeta();
    void UpdateStarRating(int rating, bool save = false);
    void UpdateLastOpened();

protected:

    void mouseMoveEvent(QMouseEvent* mouse_event) override;
    void mousePressEvent(QMouseEvent* mouse_event) override;
    //void mouseDoubleClickEvent(QMouseEvent* mouse_event) override;
};

#endif // USERMETAWIDGET_H