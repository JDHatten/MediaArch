#pragma once
#ifndef USERDATAWIDGET_H
#define USERDATAWIDGET_H

#include <filesystem>

#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
//#include <QResizeEvent>
//#include <QScrollArea>
//#include <QScrollBar>
#include <QString>
#include <QTextEdit>
#include <QTime>

//#include <MediaInfoDLL.h>
#include "MediaSpaces.h"
//#include "MetaDatabaseWorker.h"

class UserMetaWidget : public QWidget
{
    Q_OBJECT

public:

    UserMetaWidget(int user_id, QWidget* parent = Q_NULLPTR);
    ~UserMetaWidget();

    int getStarRating();
    QString getUserNotes();
    void setUserMeta(std::filesystem::path file_path, std::map<QString, QVariant>* user_meta);
    void changeUserId(int user_id); // TODO

signals:

    void userMetaUpdated(std::filesystem::path file_path, std::map<QString, QVariant>* user_meta);

public slots:

    void setReadyToUse(bool ready);
    void enableUserMetaWidget(bool enabled);
    void fileOpenedUpdateUserMeta(std::filesystem::path file_path, std::map<QString, QVariant>* user_meta);

private slots:

    

private:
    int user_id = 0;
    std::filesystem::path file_path;
    bool is_ready_to_use = false;

    QLabel* file_name_label = nullptr;
    QLabel* star_rating_label = nullptr;
    QPixmap* star_pixmap = nullptr;
    QWidget* date_last_opened_widget = nullptr;
    QLabel* date_last_opened_label = nullptr;
    QLabel* date_last_opened = nullptr;
    QTextEdit* user_file_notes = nullptr;
    QPushButton* edit_notes_button = nullptr;

    std::map<QString, QVariant>* user_meta;

    int current_star_count = 0;
    int current_star_count_highlighted = -1;

    void UpdateUserMeta();
    void UpdateStarRating(int rating, bool save = false);
    void UpdateLastOpened();

protected:

    void mouseMoveEvent(QMouseEvent* mouse_event) override;
    void mousePressEvent(QMouseEvent* mouse_event) override;
    //void mouseDoubleClickEvent(QMouseEvent* mouse_event) override;
    //void paintEvent(QPaintEvent* paint_event) override;
};

#endif // USERDATAWIDGET_H