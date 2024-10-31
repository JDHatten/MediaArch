#pragma once

#ifndef DIALOGMESSAGE_H
#define DIALOGMESSAGE_H

//#include <QComboBox>
#include <QDialog>
//#include <QGraphicsDropShadowEffect>
#include <QLayout>
#include <QLineEdit>
//#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QString>

#include "UserSettings.h"
#include "MediaSpaces.h"
#include "SwitchButton.h"
//#include "DetailedViewer.h"

class DialogMessage : public QDialog
{
    Q_OBJECT

public:

    DialogMessage(UserSettings* user_settings, QWidget* parent = nullptr);
    ~DialogMessage();

    void updateMessage(QString title, QString message, int buttons = MA::Button::Ok);

private:

    UserSettings* user_settings = nullptr;
    QWidget* parent_caller = nullptr;

    QLabel* message_label = nullptr;

    QPushButton* defaults_button = nullptr;
    QPushButton* reset_button = nullptr;
    QPushButton* refresh_button = nullptr;
    QPushButton* cancel_button = nullptr;
    QPushButton* overwrite_button = nullptr;
    QPushButton* rename_button = nullptr;
    QPushButton* rename_original_button = nullptr;
    QPushButton* delete_button = nullptr;
    QPushButton* skip_button = nullptr;
    QPushButton* apply_button = nullptr;
    QPushButton* continue_button = nullptr;
    QPushButton* ok_button = nullptr;

    //QDialog* custom_tooltip = nullptr;
    //QMessageBox* custom_tooltip = nullptr;

    QWidget* CreateWidget(QWidget* parent_widget, bool grid_layout = true);
    QLabel* CreateLabel(QWidget* parent_widget, QString text, QString tooltip = "");
    QLineEdit* CreateLineEdit(QWidget* parent_widget, QString setting);
    QPushButton* CreateButton(QWidget* parent_widget, MA::Button::ButtonRole role, bool use_alt_text = false);
    //QComboBox* CreateComboBox(QWidget* parent_widget, QString setting, QVector<Text::Option> item_list);
    SwitchButton* CreateSwitch(QWidget* parent_widget, QString setting, SwitchButton::Style style = SwitchButton::Style::YESNO);

signals:

    void buttonPressed(MA::Button::ButtonRole);

private slots:



protected:

    //virtual bool event(QEvent* event) override;
    //virtual void keyPressEvent(QKeyEvent* key_event) override;
    //virtual void mouseMoveEvent(QMouseEvent* mouse_event) override;
    //virtual void mousePressEvent(QMouseEvent* mouse_event) override;

};

#endif // DIALOGMESSAGE_H