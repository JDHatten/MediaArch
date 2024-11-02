#include "DialogMessage.h"

DialogMessage::DialogMessage(UserSettings* user_settings, QWidget* parent) : QDialog(), user_settings(user_settings), parent_caller(parent)
{
    move(parent->pos().x() + parent->width() / 2 - 200, parent->pos().y() + parent->height() / 2 - 100);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    setLayout(new QVBoxLayout(this));
    setMouseTracking(true);
    setStyleSheet("QDialog { border: 1px solid #000000; }");
    //setWindowFlags(Qt::WindowType::FramelessWindowHint);// No Border
    //setWindowFlags(Qt::WindowType::Popup); // No Border, Can click out
    //setWindowFlags(Qt::WindowType::Tool);
    //setWindowFlags(Qt::WindowType::SplashScreen);
    setWindowTitle(MediaArchApp::AppName);

    QWidget* message_box = CreateWidget(this, false);
    message_label = CreateLabel(message_box, "[ ? ]");

    // Buttons (In Order As Written) // TODO: Custom order?
    QWidget* button_box = CreateWidget(this, false);
    cancel_button = CreateButton(button_box, MA::Button::Cancel);
    defaults_button = CreateButton(button_box, MA::Button::RestoreDefaults);
    reset_button = CreateButton(button_box, MA::Button::Reset);
    refresh_button = CreateButton(button_box, MA::Button::Reset);
    overwrite_button = CreateButton(button_box, MA::Button::Overwrite);
    rename_button = CreateButton(button_box, MA::Button::Rename);
    rename_original_button = CreateButton(button_box, MA::Button::RenameOriginal);
    delete_button = CreateButton(button_box, MA::Button::Delete);
    skip_button = CreateButton(button_box, MA::Button::Skip);
    apply_button = CreateButton(button_box, MA::Button::Apply);
    continue_button = CreateButton(button_box, MA::Button::Continue);
    ok_button = CreateButton(button_box, MA::Button::Ok);
}

DialogMessage::~DialogMessage()
{
    qDebug() << "DialogMessage Deconstructed!";
}

void DialogMessage::updateMessage(QString title, QString message, int buttons)
{
    setWindowTitle(title);
    message_label->setText(message);

    defaults_button->setVisible(MA::Button::RestoreDefaults & buttons);
    reset_button->setVisible(MA::Button::Reset & buttons);
    refresh_button->setVisible(MA::Button::Refresh & buttons);
    overwrite_button->setVisible(MA::Button::Overwrite & buttons);
    cancel_button->setVisible(MA::Button::Cancel & buttons);
    rename_button->setVisible(MA::Button::Rename & buttons);
    rename_original_button->setVisible(MA::Button::RenameOriginal & buttons);
    delete_button->setVisible(MA::Button::Delete & buttons);
    skip_button->setVisible(MA::Button::Skip & buttons);
    apply_button->setVisible(MA::Button::Apply & buttons);
    continue_button->setVisible(MA::Button::Continue & buttons);
    ok_button->setVisible(MA::Button::Ok & buttons);

    move(parent_caller->pos().x() + parent_caller->width() / 2 - width() / 2, parent_caller->pos().y() + parent_caller->height() / 2 - height() / 2);
}

QWidget* DialogMessage::CreateWidget(QWidget* parent_widget, bool grid_layout)
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

QLabel* DialogMessage::CreateLabel(QWidget* parent_widget, QString text, QString tooltip)
{
    QLabel* label = new QLabel(parent_widget);
    label->setFont(MA::Font::SettingsLabelQ);
    //label->setMouseTracking(true);
    label->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
    label->setText(text);
    /*label->setToolTip(
        MA::Styles::ToolTip::createCustomToolTip(
            text.removeLast(),
            tooltip, 600
        ));*/
    label->setWordWrap(true);
    parent_widget->layout()->addWidget(label);
    return label;
}

QLineEdit* DialogMessage::CreateLineEdit(QWidget* parent_widget, QString setting)
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

QPushButton* DialogMessage::CreateButton(QWidget* parent_widget, MA::Button::ButtonRole role, bool use_alt_text)
{
    Text::Type label_type = Text::Label;
    Text::Type desc_type = Text::Description;
    if (use_alt_text) {
        label_type = Text::LabelAlt;
        desc_type = Text::DescriptionAlt;
    }
    QPushButton* button = new QPushButton(parent_widget);
    button->setFont(MA::Font::SettingsValueQ);
    button->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
    button->setStyleSheet(MA::Styles::Button::QPushButtonGeneral);
    button->setText(Text::from(role, label_type));
    button->setToolTip(Text::from(role, desc_type));
    button->setVisible(false);
    parent_widget->layout()->addWidget(button);

    if (role & MA::Button::Cancel)
        connect(button, &QPushButton::clicked, this,
            [=](bool checked) {
                emit buttonPressed(role);
                reject();
            });
    else
        connect(button, &QPushButton::clicked, this,
            [=](bool checked) {
                emit buttonPressed(role);
                accept();
            });

    return button;
}

SwitchButton* DialogMessage::CreateSwitch(QWidget* parent_widget, QString setting, SwitchButton::Style style)
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
