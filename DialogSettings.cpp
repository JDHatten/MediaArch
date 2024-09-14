#include "DialogSettings.h"

DialogSettings::DialogSettings(UserSettings* user_settings, QWidget* parent) : QDialog(), user_settings(user_settings)
{
    //resize(600, minimumHeight());
    setMinimumWidth(500);
    if (parent->width() > 900)
        resize(parent->width() * 0.7, height());
    else
        resize(parent->width() * 0.95, height());
    qDebug() << parent->width() << "x" << parent->height() << " -- " << width() << "x" << height();
    move(parent->pos().x() + parent->width() / 2 - width() / 2, parent->pos().y() + parent->height() / 2 - height() / 2);

    setWindowFlags(Qt::WindowType::FramelessWindowHint);// No Border
    //setWindowFlags(Qt::WindowType::Popup); // No Border, Can click out
    //setWindowFlags(Qt::WindowType::Tool);
    //setWindowFlags(Qt::WindowType::SplashScreen);
    setWindowTitle(DialogSettings::tr("Settings"));
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_Hover);
    setLayout(new QVBoxLayout(this));
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    setStyleSheet("QDialog { border: 2px solid #000000; }");

    settings_tabs = new QTabWidget(this);
    settings_tabs->setTabShape(QTabWidget::TabShape::Triangular);
    settings_tabs->setDocumentMode(true);

    //parent->setMouseTracking(true);
    setMouseTracking(true);
    settings_tabs->setMouseTracking(true);
    //settings_tabs->window()->setMouseTracking(true);
    //window()->setMouseTracking(true);
    
    layout()->addWidget(settings_tabs);

    // Tabs (All Items Placed In-Order)
    general_tab = CreateTab(settings_tabs, Text::from(Table::General, ""));
    thumbnail_view_tab = CreateTab(settings_tabs, Text::from(MA::Media::Viewer::Grid, Text::LabelAlt));
    QWidget* detail_view_tab = CreateTab(settings_tabs, Text::from(MA::Media::Viewer::Detailed));

    //settings_tabs->widget(0)->setMouseTracking(true);


    // General Settings

    QWidget* date_time_format = CreateWidget(general_tab);
    QLabel* date_time_format_label = CreateLabel( date_time_format,
        Text::from(Table::Settings, Field::Settings::CustomDateFormat, Text::Label, ":"),
        Text::from(Table::Settings, Field::Settings::CustomDateFormat, Text::Description)
    );
    date_time_format_edit = CreateLineEdit(date_time_format, Field::Settings::CustomDateFormat);
    
    QWidget* directory_history_limit = CreateWidget(general_tab);
    QLabel* directory_history_limit_label = CreateLabel( directory_history_limit,
        Text::from(Table::Settings, Field::Settings::MaxHistoryCount, Text::Label, ":"),
        Text::from(Table::Settings, Field::Settings::MaxHistoryCount, Text::Description)
    );
    directory_history_limit_spinner = CreateSpinBox(directory_history_limit, Field::Settings::MaxHistoryCount);
    
    QWidget* media_divider_handle_width = CreateWidget(general_tab);
    QLabel* media_divider_handle_width_label = CreateLabel( media_divider_handle_width,
        Text::from(Table::Settings, Field::Settings::MediaDividerHandleWidth, Text::Label, ":"),
        Text::from(Table::Settings, Field::Settings::MediaDividerHandleWidth, Text::Description)
    );
    media_divider_handle_width_spinner = CreateSpinBox(media_divider_handle_width, Field::Settings::MediaDividerHandleWidth);

    QWidget* sub_dir_sort_method = CreateWidget(general_tab);
    QLabel* sub_dir_sort_method_label = CreateLabel( sub_dir_sort_method,
        Text::from(Table::Settings, Field::Settings::SubDirectorySortMethod, Text::LabelAlt, ":"),
        Text::from(Table::Settings, Field::Settings::SubDirectorySortMethod, Text::DescriptionAlt)
    );
    sub_dir_sort_method_combo = CreateComboBox( sub_dir_sort_method,
        Field::Settings::SubDirectorySortMethod,
        Text::getSubDirectorySortMethodOptions()
    );

    QWidget* modifier_key_select = CreateWidget(general_tab);
    QLabel* modifier_key_select_label = CreateLabel(modifier_key_select,
        Text::from(Table::Settings, Field::Settings::ModifierKeySelect, Text::Label, ":"),
        Text::from(Table::Settings, Field::Settings::ModifierKeySelect, Text::Description)
    );
    modifier_key_select_combo = CreateComboBox(modifier_key_select,
        Field::Settings::ModifierKeySelect,
        Text::getModifierKeyOptions()
    );
    QWidget* modifier_key_multi_select = CreateWidget(general_tab);
    QLabel* modifier_key_multi_select_label = CreateLabel(modifier_key_multi_select,
        Text::from(Table::Settings, Field::Settings::ModifierKeyMultiSelect, Text::Label, ":"),
        Text::from(Table::Settings, Field::Settings::ModifierKeyMultiSelect, Text::Description)
    );
    modifier_key_multi_select_combo = CreateComboBox(modifier_key_multi_select,
        Field::Settings::ModifierKeyMultiSelect,
        Text::getModifierKeyOptions()
    );
    QWidget* modifier_key_unselect = CreateWidget(general_tab);
    QLabel* modifier_key_unselect_label = CreateLabel(modifier_key_unselect,
        Text::from(Table::Settings, Field::Settings::ModifierKeyUnselect, Text::Label, ":"),
        Text::from(Table::Settings, Field::Settings::ModifierKeyUnselect, Text::Description)
    );
    modifier_key_unselect_combo = CreateComboBox(modifier_key_unselect,
        Field::Settings::ModifierKeyUnselect,
        Text::getModifierKeyOptions()
    );

    QWidget* selection_looping = CreateWidget(general_tab);
    QLabel* selection_looping_label = CreateLabel(selection_looping,
        Text::from(Table::Settings, Field::Settings::MediaItemSelectionLooping, Text::Label, ":"),
        Text::from(Table::Settings, Field::Settings::MediaItemSelectionLooping, Text::Description)
    );
    selection_looping_switch = CreateSwitch(selection_looping, Field::Settings::MediaItemSelectionLooping);


    // Grid / List View Settings

    QWidget* media_item_size = CreateWidget(thumbnail_view_tab);
    QLabel* media_item_size_label = CreateLabel( media_item_size,
        Text::from(Table::Settings, Field::Settings::MediaItemScaleSize, Text::Label, ":"),
        Text::from(Table::Settings, Field::Settings::MediaItemScaleSize, Text::Description)
    );
    media_item_size_spinner = CreateSpinBox( media_item_size,
        Field::Settings::MediaItemScaleSize, 50, 300, 50,
        Text::from(Text::Measurement::Pixel, Text::Label, "", " ")
    );

    QWidget* thumbnail_timer = CreateWidget(thumbnail_view_tab);
    QLabel* thumbnail_timer_label = CreateLabel( thumbnail_timer,
        Text::from(Table::Settings, Field::Settings::ThumbnailTimer, Text::Label, ":"),
        Text::from(Table::Settings, Field::Settings::ThumbnailTimer, Text::Description)
    );
    thumbnail_timer_spinner = CreateSpinBox( thumbnail_timer,
        Field::Settings::ThumbnailTimer, 250, 3000, 50,
        Text::from(Text::Measurement::MilliSecond, Text::Label, "", " ")
    );

    QWidget* thumbnail_limit = CreateWidget(thumbnail_view_tab);
    QLabel* thumbnail_limit_label = CreateLabel( thumbnail_limit,
        Text::from(Table::Settings, Field::Settings::VideoThumbnailCount, Text::Label, ":"),
        Text::from(Table::Settings, Field::Settings::VideoThumbnailCount, Text::Description)
    );
    thumbnail_limit_spinner = CreateSpinBox(thumbnail_limit, Field::Settings::VideoThumbnailCount, 0, 16);

    QWidget* max_frames_to_search = CreateWidget(thumbnail_view_tab);
    QLabel* max_frames_to_search_label = CreateLabel( max_frames_to_search,
        Text::from(Table::Settings, Field::Settings::MaxFramesToSearch, Text::Label, ":"),
        Text::from(Table::Settings, Field::Settings::MaxFramesToSearch, Text::Description)
    );
    max_frames_to_search_spinner = CreateSpinBox(max_frames_to_search, Field::Settings::MaxFramesToSearch, 4000, 999999999, 100);

    QWidget* auto_generate_thumbnail = CreateWidget(thumbnail_view_tab);
    QLabel* auto_generate_thumbnail_label = CreateLabel( auto_generate_thumbnail,
        Text::from(Table::Settings, Field::Settings::AutoGenerateThumbnails, Text::Label, ":"),
        Text::from(Table::Settings, Field::Settings::AutoGenerateThumbnails, Text::Description)
    );
    auto_generate_thumbnail_switch = CreateSwitch(auto_generate_thumbnail, Field::Settings::AutoGenerateThumbnails);

    QWidget* auto_gen_video_size_limit = CreateWidget(thumbnail_view_tab);
    QLabel* auto_gen_video_size_limit_label = CreateLabel( auto_gen_video_size_limit,
        Text::from(Table::Settings, Field::Settings::AutoGenerateVideoSizeLimit, Text::LabelAlt, ":"),
        Text::from(Table::Settings, Field::Settings::AutoGenerateVideoSizeLimit, Text::Description)
    );
    auto_gen_video_size_limit_spinner = CreateSpinBox( auto_gen_video_size_limit,
        Field::Settings::AutoGenerateVideoSizeLimit, 0, 1000, 1,
        Text::from(Text::Measurement::MB, Text::Label, "", " ")
    );


    // Detailed View Settings

    QWidget* viewable_metadata_columns = CreateWidget(detail_view_tab);
    QLabel* viewable_metadata_columns_label = CreateLabel(viewable_metadata_columns,
        Text::from(Table::Settings, Field::Settings::ColumnLabelsField, Text::Label, ":"),
        Text::from(Table::Settings, Field::Settings::ColumnLabelsField, Text::Description)
    );
    detailed_viewer_columns = DetailedViewer::ColumnLabels(
        user_settings->getUserSetting(Field::Settings::ColumnLabelsTable).toStringList(),
        user_settings->getUserSetting(Field::Settings::ColumnLabelsField).toStringList()
    );
    viewable_metadata_columns_combo = CreateComboCheckBox(viewable_metadata_columns,
        Field::Settings::ColumnLabelsField,
        Text::getMetadataSortMethodOptions(detailed_viewer_columns.table_list, detailed_viewer_columns.field_list)
    );


    // Buttons 
    QWidget* button_box = CreateWidget(this, false);
    QPushButton* defaults_button = CreateButton(button_box, Text::from(MA::Button::RestoreDefaults));
    QPushButton* reset_button = CreateButton(button_box, Text::from(MA::Button::Reset));
    QPushButton* cancel_button = CreateButton(button_box, Text::from(MA::Button::Cancel)); 
    QPushButton* apply_button = CreateButton(button_box, Text::from(MA::Button::Apply));
    QPushButton* ok_button = CreateButton(button_box, Text::from(MA::Button::Ok));

    connect(defaults_button, &QPushButton::clicked, this, &DialogSettings::RestoreDefaultSettings);
    connect(reset_button, &QPushButton::clicked, this, &DialogSettings::RevertSettingChanges);
    connect(cancel_button, &QPushButton::clicked, this,
        [this](bool checked) {
            DialogSettings::user_settings->clearUnsavedUserSettings();
            if (settings_updated) {
                emit settingsUpdated();
                reject();
            }
            else
                reject();
        });
    connect(apply_button, &QPushButton::clicked, this, &DialogSettings::UpdateSettings);
    connect(ok_button, &QPushButton::clicked, this,
        [this](bool checked) {
            UpdateSettings();
            if (settings_updated)
                emit settingsUpdated();
            accept();
        });
}

DialogSettings::~DialogSettings() {
    if (custom_tooltip) {
        custom_tooltip->deleteLater();
    }
    qDebug() << "DialogSettings Deconstructed!";
}

QWidget* DialogSettings::CreateTab(QTabWidget* tab_widget, QString title, bool grid_layout)
{
    QWidget* widget = new QWidget(tab_widget);
    //widget->setMouseTracking(true);
    if (grid_layout)
        widget->setLayout(new QGridLayout(widget));
    else
        widget->setLayout(new QVBoxLayout(widget));
    tab_widget->addTab(widget, title);
    return widget;
}

QWidget* DialogSettings::CreateWidget(QWidget* parent_widget, bool grid_layout)
{
    QWidget* setting_widget = new QWidget(parent_widget);
    //setting_widget->setMouseTracking(true);
    setting_widget->setLayout(new QHBoxLayout(setting_widget));
    if (grid_layout) {
        QGridLayout* grid = qobject_cast<QGridLayout*>(parent_widget->layout());
        if (width() > 900) {
            int column = 1;
            int row = grid->rowCount();
            if (grid->count() % 2 == 1) {
                column = 2;
                row -= 1;
            }
            grid->addWidget(setting_widget, row, column);
        }
        else
            grid->addWidget(setting_widget, grid->rowCount(), 1);
    }
    else {
        QHBoxLayout* layout = new QHBoxLayout(setting_widget);
        parent_widget->layout()->addWidget(setting_widget);
    }
    return setting_widget;
}

QLabel* DialogSettings::CreateLabel(QWidget* parent_widget, QString text, QString tooltip)
{
    QLabel* label = new QLabel(parent_widget);
    label->setFont(MA::Font::SettingsLabelQ);
    //label->setMouseTracking(true);
    label->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
    label->setText(text);
    label->setToolTip(
        MA::Styles::ToolTip::createCustomToolTip(
            text.removeLast(),
            tooltip, 600
        ));
    parent_widget->layout()->addWidget(label);
    return label;
}

QLineEdit* DialogSettings::CreateLineEdit(QWidget* parent_widget, QString setting)
{
    QLineEdit* line_edit = new QLineEdit(parent_widget);
    line_edit->setFont(MA::Font::SettingsValueQ);
    line_edit->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Maximum);
    line_edit->setText(user_settings->getUserSetting(setting).toString());
    parent_widget->layout()->addWidget(line_edit);

    connect(line_edit, &QLineEdit::textChanged, this,
        [=](const QString& text) {
            qDebug() << text;
            user_settings->setUserSetting(setting, text);
        });

    return line_edit;
}

QComboBox* DialogSettings::CreateComboBox(QWidget* parent_widget, QString setting, QVector<Text::Option> item_list)
{
    long long settings_value = user_settings->getUserSetting(setting).toLongLong();
    int current_index = -1;

    // Note: The QComboBox index can be directly from settings, or from it's data that is matched from item_list.
    bool index_from_data_match = false;

    QComboBox* combo_box = new QComboBox(parent_widget);
    combo_box->setFont(MA::Font::SettingsValueQ);
    //combo_box->setMouseTracking(true);
    combo_box->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Maximum);
    for (qsizetype i = 0; i < item_list.size(); i++) {
        combo_box->addItem(item_list.at(i).label);
        //combo_box->setItemData(i, item_list.at(i).description, Qt::ItemDataRole::ToolTipRole);
        if (item_list.at(i).data.isValid()) {
            combo_box->setItemData(i, item_list.at(i).data, Qt::ItemDataRole::UserRole);
            if (settings_value == item_list.at(i).data.toLongLong()) {
                index_from_data_match = true;
                current_index = i; // from Data Match
            }
        }
    }
    //qobject_cast<QWidget*>(combo_box->children().at(1))->setToolTip("test");

    if (current_index < 0)
        current_index = user_settings->getUserSetting(setting).toInt(); // from Settings

    // Failsafe
    if (current_index < 0 or current_index >= combo_box->count()) {
        qWarning() << setting << "QComboBox's index value is out-of-range. Fix options in item_list.";
        current_index = 0;
    }

    combo_box->setCurrentIndex(current_index);
    if (item_list.at(current_index).description.length()) {
        combo_box->setToolTip(
            MA::Styles::ToolTip::createCustomToolTip(
                item_list.at(current_index).label,
                item_list.at(current_index).description
            ));
    }
    parent_widget->layout()->addWidget(combo_box);

    connect(combo_box, &QComboBox::currentIndexChanged, this,
        [=](int index) {
            qDebug() << "QComboBox::currentIndexChanged:" << index;

            if (index_from_data_match)
                user_settings->setUserSetting(setting, item_list.at(index).data.toLongLong());
            else
                user_settings->setUserSetting(setting, index);

            if (item_list.at(current_index).description.length()) {
                combo_box->setToolTip(
                    MA::Styles::ToolTip::createCustomToolTip(
                        item_list.at(index).label,
                        item_list.at(index).description
                    ));
            }
            if (custom_tooltip) {
                custom_tooltip->deleteLater();
                custom_tooltip = nullptr;
            }
        });
    connect(combo_box, &QComboBox::activated, this,
        [=](int index) {
            qDebug() << "QComboBox::activated";
            if (custom_tooltip) {
                custom_tooltip->deleteLater();
                custom_tooltip = nullptr;
            }
        });
    connect(combo_box, &QComboBox::highlighted, this,
        [=](int index) {
            qDebug() << "QComboBox::highlighted";

            if (custom_tooltip)
                custom_tooltip->deleteLater();

            if (item_list.at(index).description.length()) {

                //custom_tooltip = new QMessageBox(this);
                custom_tooltip = new QDialog(this, Qt::WindowType::ToolTip);
                //custom_tooltip = new QDialog(this, Qt::WindowType::MSWindowsFixedSizeDialogHint);
                //custom_tooltip = new QDialog(this, Qt::WindowType::Popup);
                custom_tooltip->setAttribute(Qt::WA_StyledBackground, true);
                custom_tooltip->setAttribute(Qt::WA_TranslucentBackground);
                custom_tooltip->setFixedWidth(combo_box->width());
                custom_tooltip->setLayout(new QVBoxLayout(custom_tooltip));
                custom_tooltip->setMouseTracking(true);
                custom_tooltip->setStyleSheet(MA::Styles::ToolTip::General);
                //custom_tooltip->setStyleSheet(MA::Styles::ToolTip::General + MA::Styles::MakeStyle("QLabel", { MA::Styles::ToolTip::Table }) + MA::Styles::MakeStyle("ToolTip", { MA::Styles::ToolTip::Bold }));
                //qDebug() << MA::Styles::ToolTip::General + MA::Styles::MakeStyle("QLabel", { MA::Styles::ToolTip::Table }) + MA::Styles::MakeStyle("ToolTip", { MA::Styles::ToolTip::Bold });

                custom_tooltip->setWindowFlags(custom_tooltip->windowFlags() | Qt::FramelessWindowHint);// | Qt::WindowSystemMenuHint);
                custom_tooltip->layout()->setContentsMargins(4, 4, 4, 4);

                QWidget* container = new QWidget(custom_tooltip);
                container->setLayout(new QVBoxLayout(container));
                container->layout()->setContentsMargins(8, 3, 8, 4);
                container->layout()->setSpacing(0);

                QLabel* title_label = new QLabel(custom_tooltip);
                title_label->setStyleSheet(MA::Styles::ToolTip::Table + MA::Styles::General::Bold);
                title_label->setText(item_list.at(index).label);
                title_label->setWordWrap(true);

                QLabel* tooltip_label = new QLabel(custom_tooltip);
                //tooltip_label->setContentsMargins(0, 0, 0, 0);
                tooltip_label->setObjectName("ToolTip");
                tooltip_label->setStyleSheet(MA::Styles::ToolTip::Table);
                tooltip_label->setText(item_list.at(index).description);
                tooltip_label->setWordWrap(true);

                QGraphicsDropShadowEffect* drop_shadow = new QGraphicsDropShadowEffect();
                drop_shadow->setBlurRadius(10);
                drop_shadow->setColor(QColor(0, 0, 0, 180));
                drop_shadow->setOffset(2, 2);
                container->setGraphicsEffect(drop_shadow);

                container->layout()->addWidget(title_label);
                container->layout()->addWidget(tooltip_label);
                custom_tooltip->layout()->addWidget(container);

                connect(custom_tooltip, &QDialog::accepted, this,
                    [=]() {
                        qDebug() << "QDialog::accepted";
                        custom_tooltip->deleteLater();
                        custom_tooltip = nullptr;
                    });
                connect(custom_tooltip, &QDialog::finished, this,
                    [=](int result) {
                        qDebug() << "QDialog::finished";
                        custom_tooltip->deleteLater();
                        custom_tooltip = nullptr;
                    });
                connect(custom_tooltip, &QDialog::rejected, this,
                    [=]() {
                        qDebug() << "QDialog::rejected";
                        custom_tooltip->deleteLater();
                        custom_tooltip = nullptr;
                    });

                custom_tooltip->show();

                //qDebug() << pos() << "--" << settings_tabs->pos() << "--" << parent_widget->pos() << "--" << combo_box->pos();
                //QPoint tooltip_size = QPoint(0, custom_tooltip->height());
                //qDebug() << custom_tooltip->width() << "x" << custom_tooltip->height();
                custom_tooltip->move(pos() + settings_tabs->pos() + parent_widget->pos());
            }
        });
    return combo_box;
}

QComboBox* DialogSettings::CreateComboCheckBox(QWidget* parent_widget, QString setting, QVector<Text::MetadataMethod> item_list)
{
    QComboBox* combo_box = new QComboBox(parent_widget);
    combo_box->setFont(MA::Font::SettingsValueQ);
    //combo_box->setMouseTracking(true);
    combo_box->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Maximum);

    QStandardItemModel* model = new QStandardItemModel();
    QList<QStandardItem*> items;
    for (qsizetype i = 0; i < item_list.size(); i++) {
        QStandardItem* item = new QStandardItem(item_list.at(i).label);
        if (item_list.at(i).label.length()) {

            if (item_list.at(i).field == Field::General::FilePath)
                item->setFlags(Qt::ItemFlag::ItemIsUserCheckable);
            else
                item->setFlags(Qt::ItemFlag::ItemIsUserCheckable | Qt::ItemFlag::ItemIsEnabled);

            if (item_list.at(i).checked)
                item->setData(Qt::CheckState::Checked, Qt::ItemDataRole::CheckStateRole);
            else
                item->setData(Qt::CheckState::Unchecked, Qt::ItemDataRole::CheckStateRole);
            item->setData(item_list.at(i).table, MA::Role::Table);
            item->setData(item_list.at(i).field, MA::Role::Field);
        }
        else {
            item->setFlags(Qt::ItemFlag::NoItemFlags);
        }
        items.append(item);
    }
    model->appendColumn(items);
    combo_box->setModel(model);

    connect(model, &QStandardItemModel::itemChanged, this,
        [=](QStandardItem* item) {
            qDebug() << "QStandardItemModel::itemChanged" << item->text();

            QString table = item->data(MA::Role::Table).toString();
            QString field = item->data(MA::Role::Field).toString();

            if (item->checkState() == Qt::CheckState::Checked) {
                detailed_viewer_columns.addColumn(table, field);
            }
            else {
                detailed_viewer_columns.removeColumn(table, field);
            }
            detailed_viewer_columns.saveColumns(user_settings); // setUserSetting

        });
    connect(model, &QStandardItemModel::dataChanged, this,
        [=](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles = QList<int>()) {
            qDebug() << "QStandardItemModel::dataChanged";// << item->text();
        });

    int current_index = 0;
    combo_box->setCurrentIndex(current_index);
    if (item_list.at(current_index).description.length()) {
        combo_box->setToolTip(
            MA::Styles::ToolTip::createCustomToolTip(
                item_list.at(current_index).label,
                item_list.at(current_index).description
            ));
    }
    parent_widget->layout()->addWidget(combo_box);

    return combo_box;
}

QSpinBox* DialogSettings::CreateSpinBox(QWidget* parent_widget, QString setting, int minimum, int maximum, int step, QString suffix)
{
    QSpinBox* spin_box = new QSpinBox(parent_widget);
    spin_box->setFont(MA::Font::SettingsValueQ);
    spin_box->setMinimumWidth(80);
    spin_box->setRange(minimum, maximum);
    spin_box->setSingleStep(step);
    spin_box->setSizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Fixed);
    spin_box->setSuffix(suffix);
    spin_box->setValue(user_settings->getUserSetting(setting).toInt());
    parent_widget->layout()->addWidget(spin_box);

    connect(spin_box, &QSpinBox::valueChanged, this,
        [=](int number) {
            user_settings->setUserSetting(setting, number);
        });

    return spin_box;
}

SwitchButton* DialogSettings::CreateSwitch(QWidget* parent_widget, QString setting, SwitchButton::Style style)
{
    SwitchButton* switch_button = new SwitchButton(MA::Color::OutlineQ, MA::Color::PrimaryLG, MA::Color::SecondaryLG, style, parent_widget);
    switch_button->setValue(user_settings->getUserSetting(setting).toBool());
    parent_widget->layout()->addWidget(switch_button);

    connect(switch_button, &SwitchButton::valueChanged, this,
        [=](bool new_value) {
            user_settings->setUserSetting(setting, new_value);
        });

    return switch_button;
}

QPushButton* DialogSettings::CreateButton(QWidget* parent_widget, QString text)
{
    QPushButton* button = new QPushButton(parent_widget);
    button->setFont(MA::Font::SettingsValueQ);
    button->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
    button->setStyleSheet(MA::Styles::Button::QPushButtonGeneral);
    button->setText(text);
    parent_widget->layout()->addWidget(button);
    return button;
}

void DialogSettings::UpdateSettings()
{
    if (user_settings->saveUserSettings())
        settings_updated = true;
    setStatusTip("Settings Updated!!!!!");
}

void DialogSettings::RevertSettingChanges()
{
    // General Settings
    date_time_format_edit->setText(user_settings->getUserSetting(Field::Settings::CustomDateFormat).toString());
    directory_history_limit_spinner->setValue(user_settings->getUserSetting(Field::Settings::MaxHistoryCount).toInt());
    media_divider_handle_width_spinner->setValue(user_settings->getUserSetting(Field::Settings::MediaDividerHandleWidth).toInt());
    sub_dir_sort_method_combo->setCurrentIndex(user_settings->getUserSetting(Field::Settings::SubDirectorySortMethod).toInt());
    modifier_key_select_combo->setCurrentIndex(MA::Keys::ModifierKeys.indexOf(user_settings->getUserSetting(Field::Settings::ModifierKeySelect).toInt()));
    modifier_key_multi_select_combo->setCurrentIndex(MA::Keys::ModifierKeys.indexOf(user_settings->getUserSetting(Field::Settings::ModifierKeyMultiSelect).toInt()));
    modifier_key_unselect_combo->setCurrentIndex(MA::Keys::ModifierKeys.indexOf(user_settings->getUserSetting(Field::Settings::ModifierKeyUnselect).toInt()));
    selection_looping_switch->setValue(DefaultValue::Settings::MediaItemSelectionLooping.toBool());

    // Grid / List View Settings
    media_item_size_spinner->setValue(user_settings->getUserSetting(Field::Settings::MediaItemScaleSize).toInt());
    thumbnail_timer_spinner->setValue(user_settings->getUserSetting(Field::Settings::ThumbnailTimer).toInt());
    thumbnail_limit_spinner->setValue(user_settings->getUserSetting(Field::Settings::VideoThumbnailCount).toInt());
    max_frames_to_search_spinner->setValue(user_settings->getUserSetting(Field::Settings::MaxFramesToSearch).toInt());
    auto_generate_thumbnail_switch->setValue(user_settings->getUserSetting(Field::Settings::AutoGenerateThumbnails).toBool());
    auto_gen_video_size_limit_spinner->setValue(user_settings->getUserSetting(Field::Settings::AutoGenerateVideoSizeLimit).toInt());

    // Detailed View Settings
    detailed_viewer_columns = DetailedViewer::ColumnLabels(
        user_settings->getUserSetting(Field::Settings::ColumnLabelsTable).toStringList(),
        user_settings->getUserSetting(Field::Settings::ColumnLabelsField).toStringList()
    );
    UpdateComboCheckBoxOptions(
        viewable_metadata_columns_combo,
        detailed_viewer_columns.getTableList(),
        detailed_viewer_columns.getFieldList()
    );
    detailed_viewer_columns.saveColumns(user_settings);
}

void DialogSettings::RestoreDefaultSettings()
{
    // General Settings
    date_time_format_edit->setText(DefaultValue::Settings::CustomDateFormat.toString());
    directory_history_limit_spinner->setValue(DefaultValue::Settings::MaxHistoryCount.toInt());
    media_divider_handle_width_spinner->setValue(DefaultValue::Settings::MediaDividerHandleWidth.toInt());
    sub_dir_sort_method_combo->setCurrentIndex(DefaultValue::Settings::SubDirectorySortMethod.toInt());
    modifier_key_select_combo->setCurrentIndex(MA::Keys::ModifierKeys.indexOf(DefaultValue::Settings::ModifierKeySelect.toInt()));
    modifier_key_multi_select_combo->setCurrentIndex(MA::Keys::ModifierKeys.indexOf(DefaultValue::Settings::ModifierKeyMultiSelect.toInt()));
    modifier_key_unselect_combo->setCurrentIndex(MA::Keys::ModifierKeys.indexOf(DefaultValue::Settings::ModifierKeyUnselect.toInt()));
    selection_looping_switch->setValue(DefaultValue::Settings::MediaItemSelectionLooping.toBool());

    // Grid / List View Settings
    media_item_size_spinner->setValue(DefaultValue::Settings::MediaItemScaleSize.toInt());
    thumbnail_timer_spinner->setValue(DefaultValue::Settings::ThumbnailTimer.toInt());
    thumbnail_limit_spinner->setValue(DefaultValue::Settings::VideoThumbnailCount.toInt());
    max_frames_to_search_spinner->setValue(DefaultValue::Settings::MaxFramesToSearch.toInt());
    auto_generate_thumbnail_switch->setValue(DefaultValue::Settings::AutoGenerateThumbnails.toBool());
    auto_gen_video_size_limit_spinner->setValue(DefaultValue::Settings::AutoGenerateVideoSizeLimit.toInt());

    // Detailed View Settings
    detailed_viewer_columns = DetailedViewer::ColumnLabels(
        DefaultValue::Settings::ColumnLabelsTable.toString().split("|"),
        DefaultValue::Settings::ColumnLabelsField.toString().split("|")
    );
    UpdateComboCheckBoxOptions(
        viewable_metadata_columns_combo,
        detailed_viewer_columns.getTableList(),
        detailed_viewer_columns.getFieldList()
    );
    detailed_viewer_columns.saveColumns(user_settings);
}

void DialogSettings::UpdateComboCheckBoxOptions(QComboBox* combo_box, QStringList tables, QStringList fields)
{
    const QVector<Text::MetadataMethod> options = Text::getMetadataSortMethodOptions(tables, fields);
    combo_box->blockSignals(true);
    combo_box->model()->blockSignals(true);
    for (qsizetype i = 0; i < combo_box->count(); i++) {
        if (options.at(i).checked)
            combo_box->setItemData(i, Qt::CheckState::Checked, Qt::ItemDataRole::CheckStateRole);
        else
            combo_box->setItemData(i, Qt::CheckState::Unchecked, Qt::ItemDataRole::CheckStateRole);
    }
    combo_box->model()->blockSignals(false);
    combo_box->blockSignals(false);
}

bool DialogSettings::event(QEvent* event)
{
    //qDebug() << "(Event)" << event->type();
    if (event->type() == QEvent::Type::HoverEnter) {
        if (custom_tooltip) {
            custom_tooltip->deleteLater();
            custom_tooltip = nullptr;
        }
    }
    return QDialog::event(event);
}

void DialogSettings::keyPressEvent(QKeyEvent* key_event)
{
    qDebug() << "(Event) Key Pressed" << key_event->key();
    if (custom_tooltip) {
        custom_tooltip->deleteLater();
        custom_tooltip = nullptr;
    }
    QDialog::keyPressEvent(key_event);
}

void DialogSettings::mouseMoveEvent(QMouseEvent* mouse_event)
{
    //qDebug() << "(Event) Mouse over" << mouse_event->pos();
    QDialog::mouseMoveEvent(mouse_event);
}

void DialogSettings::mousePressEvent(QMouseEvent* mouse_event)
{
    qDebug() << "(QMouseEvent) Clicked" << mouse_event->button() << " -Type:" << mouse_event->type();

    if (custom_tooltip) {
        custom_tooltip->deleteLater();
        custom_tooltip = nullptr;
    }

    if (mouse_event->button() == Qt::MouseButton::RightButton) {

    }

    QWidget::mousePressEvent(mouse_event);
}
