#pragma once

#ifndef DIALOGSETTINGS_H
#define DIALOGSETTINGS_H

#include <QComboBox>
#include <QDialog>
#include <QGraphicsDropShadowEffect>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardItem>
#include <QString>
#include <QTabWidget>

#include "UserSettings.h"
#include "MediaSpaces.h"
#include "SwitchButton.h"
#include "DetailedViewer.h"

class DialogSettings : public QDialog
{
    Q_OBJECT

public:

    DialogSettings(UserSettings* user_settings, QWidget* parent = nullptr);
    ~DialogSettings();

private:

    UserSettings* user_settings = nullptr;
    DetailedViewer::ColumnLabels detailed_viewer_columns;

    QTabWidget* settings_tabs = nullptr;
    QWidget* general_tab = nullptr;
    QWidget* thumbnail_view_tab = nullptr;
    QLineEdit* date_time_format_edit = nullptr;
    QSpinBox* directory_history_limit_spinner = nullptr;
    QSpinBox* media_divider_handle_width_spinner = nullptr;
    QComboBox* sub_dir_sort_method_combo = nullptr;
    QComboBox* modifier_key_select_combo = nullptr;
    QComboBox* modifier_key_multi_select_combo = nullptr;
    QComboBox* modifier_key_unselect_combo = nullptr;
    SwitchButton* selection_looping_switch = nullptr;

    QSpinBox* media_item_size_spinner = nullptr;
    QSpinBox* thumbnail_timer_spinner = nullptr;
    QSpinBox* thumbnail_limit_spinner = nullptr;
    QSpinBox* max_frames_to_search_spinner = nullptr;
    SwitchButton* auto_generate_thumbnail_switch = nullptr;
    QSpinBox* auto_gen_video_size_limit_spinner = nullptr;

    QComboBox* viewable_metadata_columns_combo = nullptr;

    QDialog* custom_tooltip = nullptr;
    //QMessageBox* custom_tooltip = nullptr;

    bool settings_updated = false;

    QWidget* CreateTab(QTabWidget* tab_widget, QString title, bool grid_layout = true);
    QWidget* CreateWidget(QWidget* parent_widget, bool grid_layout = true);
    QLabel* CreateLabel(QWidget* parent_widget, QString text, QString tooltip = "");
    QLineEdit* CreateLineEdit(QWidget* parent_widget, QString setting);
    QComboBox* CreateComboBox(QWidget* parent_widget, QString setting, QVector<Text::Option> item_list);
    QComboBox* CreateComboCheckBox(QWidget* parent_widget, QString setting, QVector<Text::MetadataMethod> item_list);
    QSpinBox* CreateSpinBox(QWidget* parent_widget, QString setting, int minumum = 0, int maximum = 100, int step = 1, QString suffix = "");
    SwitchButton* CreateSwitch(QWidget* parent_widget, QString setting, SwitchButton::Style style = SwitchButton::Style::YESNO);
    QPushButton* CreateButton(QWidget* parent_widget, QString text);

    void UpdateComboCheckBoxOptions(QComboBox* combo_box, QStringList tables, QStringList fields);

signals:

    void settingsUpdated();

private slots:

    void UpdateSettings();
    void RevertSettingChanges();
    void RestoreDefaultSettings();

protected:

    virtual bool event(QEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* key_event) override;
    virtual void mouseMoveEvent(QMouseEvent* mouse_event) override;
    virtual void mousePressEvent(QMouseEvent* mouse_event) override;
    
};

#endif // DIALOGSETTINGS_H