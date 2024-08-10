#include "UserMetaWidget.h"

UserMetaWidget::UserMetaWidget(int user_id, QWidget* parent) : QWidget(parent), user_id(user_id)
{
    auto user_meta_layout = new QVBoxLayout(this);
    setLayout(user_meta_layout);
    //setFixedWidth(); // Padding/Border?
    setMouseTracking(true);
    setAttribute(Qt::WA_StyledBackground, true); // Note: setStyleSheet will only work with this set
    //setStyleSheet(QString("QLabel { padding: 0px; }"));

    file_name_label = new QLabel(this);
    file_name_label->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);
    file_name_label->setWordWrap(true);
    
    star_rating_label = new QLabel(this);
    star_rating_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    star_rating_label->setMouseTracking(true);
    star_rating_label->setFixedSize(240, 48);
    star_pixmap = new QPixmap(240, 48);

    UpdateStarRating(current_star_count);


    // TODO: Tags Widget


    date_last_opened_widget = new QWidget(this);
    auto date_last_opened_layout = new QHBoxLayout(this);
    date_last_opened_widget->setLayout(date_last_opened_layout);
    date_last_opened_label = new QLabel(this);
    date_last_opened_label->setText(UserMetaWidget::tr("Date Last Opened (Count): "));
    date_last_opened_label->setAlignment(Qt::AlignmentFlag::AlignLeft);
    date_last_opened_label->setContentsMargins(QMargins(0,0,0,0));
    date_last_opened_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    date_last_opened = new QLabel(this);
    date_last_opened->setText("0/0/0 0:00:00 (00)");
    date_last_opened->setAlignment(Qt::AlignmentFlag::AlignLeft);
    date_last_opened_layout->addWidget(date_last_opened_label);
    date_last_opened_layout->addWidget(date_last_opened);

    user_file_notes = new QTextEdit(this);
    user_file_notes->setReadOnly(true);
    user_file_notes->setFrameStyle(QFrame::Shape::NoFrame);
    user_file_notes->setAcceptRichText(false); // TODO: allow rich/html text?

    edit_notes_button = new QPushButton(this);
    edit_notes_button->setCheckable(true);
    edit_notes_button->setChecked(false);
    edit_notes_button->setText(UserMetaWidget::tr("Edit Notes"));

    connect(edit_notes_button, &QPushButton::clicked, this,
        [this](bool checked) {
            if (checked) {
                user_file_notes->setReadOnly(false);
                user_file_notes->setFrameShape(QFrame::Shape::WinPanel);
                user_file_notes->setFrameShadow(QFrame::Shadow::Sunken);
                edit_notes_button->setText(UserMetaWidget::tr("Save Notes"));
                
            }
            else {
                UpdateUserMeta(); // TODO: only if text changed
                user_file_notes->setReadOnly(true);
                user_file_notes->setFrameStyle(QFrame::Shape::NoFrame);
                edit_notes_button->setText(UserMetaWidget::tr("Edit Notes"));
            }
        });

    layout()->addWidget(file_name_label);
    layout()->addWidget(star_rating_label);
    layout()->addWidget(date_last_opened_widget);
    layout()->addWidget(user_file_notes);
    layout()->addWidget(edit_notes_button);

    enableUserMetaWidget(false);
}

UserMetaWidget::~UserMetaWidget() {}

void UserMetaWidget::setReadyToUse(bool ready)
{
    is_ready_to_use = ready;
}

void UserMetaWidget::enableUserMetaWidget(bool enabled)
{
    if (not enabled) {
        file_name_label->setText("");
        user_file_notes->setText("");
        user_meta = nullptr;
    }
    else {
        QWidget* parent_obj = qobject_cast<QWidget*>(parent());
        //parent_obj->layout()->setSpacing(0);
        //layout()->setSpacing(0);
        file_name_label->setFixedWidth(parent_obj->width() - 18); // padding/margins?
        //qDebug() << "UserMetaWidget Spacing:" << parent_obj->layout()->spacing() << "/" << layout()->spacing();
    }

    user_file_notes->setFrameStyle(QFrame::Shape::NoFrame);
    edit_notes_button->setChecked(false);
    edit_notes_button->setText(UserMetaWidget::tr("Edit Notes"));

    setVisible(enabled); // Switching tabs resets this automatically
    setEnabled(enabled);
}

int UserMetaWidget::getStarRating()
{
    return current_star_count;
}

QString UserMetaWidget::getUserNotes()
{
    return edit_notes_button->text();
}

void UserMetaWidget::setUserMeta(std::filesystem::path file_path, std::map<QString, QVariant>* user_meta)
{
    enableUserMetaWidget(true);
    UserMetaWidget::file_path = file_path;
    file_name_label->setText(QString::fromStdString(file_path.filename().string()));
    if (user_meta) {
        UserMetaWidget::user_meta = user_meta;

        // If no user_id set on file (no UserMeta ever set), set it to the currently set user_id.
        if (user_meta->at(Field::UserMeta::UserId).isValid()) {
            int user_id_on_file = user_meta->at(Field::UserMeta::UserId).toInt();
            if (user_id_on_file != user_id)
                qWarning() << "ERROR: Received wrong user id on file, that doesn't match the user id currently set.";
        }
        else
            user_meta->at(Field::UserMeta::UserId).setValue(user_id);

        current_star_count = user_meta->at(Field::UserMeta::Rating).toInt();
        UpdateStarRating(current_star_count);

        UpdateLastOpened();

        user_file_notes->setText(user_meta->at(Field::UserMeta::Notes).toString());
    }
}

void UserMetaWidget::fileOpenedUpdateUserMeta(std::filesystem::path file_path, std::map<QString, QVariant>* user_meta)
{
    if (is_ready_to_use and user_meta) {
        UserMetaWidget::user_meta = user_meta;
        int open_count = user_meta->at(Field::UserMeta::OpenCount).toInt() + 1;
        user_meta->at(Field::UserMeta::OpenCount).setValue(open_count);
        long long current_date_time = QDateTime::currentDateTime().toMSecsSinceEpoch();
        user_meta->at(Field::UserMeta::LastOpened).setValue(current_date_time);
        UpdateLastOpened();
        emit userMetaUpdated(file_path, user_meta);
    }
}

void UserMetaWidget::UpdateUserMeta()
{
    if (is_ready_to_use) {
        if (user_meta) {
            user_meta->at(Field::UserMeta::Rating).setValue(current_star_count);
            // TODO: tags
            user_meta->at(Field::UserMeta::Notes).setValue(user_file_notes->toPlainText());

            emit userMetaUpdated(file_path, user_meta);
        }
    }
    else {
        // TODO: inform user, statusbar?
        qDebug() << "Failed To Save User Meta. Wait Until All Media Data Loaded.";
    }
}

void UserMetaWidget::UpdateStarRating(int rating, bool save)
{
    if (rating == current_star_count_highlighted and not save)
        return;
    else
        current_star_count_highlighted = rating;

    QImage star_image;
    if (rating == 10)
        star_image = QImage(QString::fromStdString((BUILD_DIR_PATH / "images/stars-5.0.svg").string()));
    if (rating == 9)
        star_image = QImage(QString::fromStdString((BUILD_DIR_PATH / "images/stars-4.5.svg").string()));
    if (rating == 8)
        star_image = QImage(QString::fromStdString((BUILD_DIR_PATH / "images/stars-4.0.svg").string()));
    if (rating == 7)
        star_image = QImage(QString::fromStdString((BUILD_DIR_PATH / "images/stars-3.5.svg").string()));
    if (rating == 6)
        star_image = QImage(QString::fromStdString((BUILD_DIR_PATH / "images/stars-3.0.svg").string()));
    if (rating == 5)
        star_image = QImage(QString::fromStdString((BUILD_DIR_PATH / "images/stars-2.5.svg").string()));
    if (rating == 4)
        star_image = QImage(QString::fromStdString((BUILD_DIR_PATH / "images/stars-2.0.svg").string()));
    if (rating == 3)
        star_image = QImage(QString::fromStdString((BUILD_DIR_PATH / "images/stars-1.5.svg").string()));
    if (rating == 2)
        star_image = QImage(QString::fromStdString((BUILD_DIR_PATH / "images/stars-1.0.svg").string()));
    if (rating == 1)
        star_image = QImage(QString::fromStdString((BUILD_DIR_PATH / "images/stars-0.5.svg").string()));
    if (rating == 0)
        star_image = QImage(QString::fromStdString((BUILD_DIR_PATH / "images/stars-0.0.svg").string()));
    
    star_pixmap->convertFromImage(star_image.scaled(star_rating_label->size()));
    star_rating_label->setPixmap(*star_pixmap);

    if (save) {
        current_star_count = current_star_count_highlighted;
        UpdateUserMeta();
    }
}

void UserMetaWidget::UpdateLastOpened()
{
    // File last opened / open count
    int open_count = user_meta->at(Field::UserMeta::OpenCount).toInt();
    long long current_date_time = user_meta->at(Field::UserMeta::LastOpened).toLongLong();
    QString date_last_opened_str = "0000/00/00  00:00:00";;
    if (current_date_time)
        date_last_opened_str = QDateTime::fromMSecsSinceEpoch(current_date_time).toLocalTime().toString("yyyy/MM/dd  HH:mm:ss");
    date_last_opened->setText(date_last_opened_str + QString("  (%1)").arg(open_count));
}

void UserMetaWidget::mouseMoveEvent(QMouseEvent* mouse_event)
{
    if (star_rating_label->underMouse()) {

        int five_star_width = star_rating_label->width();
        int one_star_width = round(five_star_width / 5.0);
        int half_star_width = round(five_star_width / 10.0);
        int relative_mouse_x = mouse_event->pos().x();

        if (relative_mouse_x > five_star_width - 3) {
            UpdateStarRating(10);
        }
        else if (relative_mouse_x > one_star_width * 4 + half_star_width) {
            UpdateStarRating(9);
        }
        else if (relative_mouse_x > one_star_width * 4) {
            UpdateStarRating(8);
        }
        else if (relative_mouse_x > one_star_width * 3 + half_star_width) {
            UpdateStarRating(7);
        }
        else if (relative_mouse_x > one_star_width * 3) {
            UpdateStarRating(6);
        }
        else if (relative_mouse_x > one_star_width * 2 + half_star_width) {
            UpdateStarRating(5);
        }
        else if (relative_mouse_x > one_star_width * 2) {
            UpdateStarRating(4);
        }
        else if (relative_mouse_x > one_star_width + half_star_width) {
            UpdateStarRating(3);
        }
        else if (relative_mouse_x > one_star_width) {
            UpdateStarRating(2);
        }
        else if (relative_mouse_x > half_star_width) {
            UpdateStarRating(1);
        }
        else {
            UpdateStarRating(0);
        }
        //qDebug().nospace() << "(Event) Mouse Over: " << mouse_event->pos() << " (" << five_star_width << ")";
    }
    else {
        UpdateStarRating(current_star_count);
    }
    QWidget::mouseMoveEvent(mouse_event);
}

void UserMetaWidget::mousePressEvent(QMouseEvent* mouse_event)
{
    if (mouse_event->button() == Qt::MouseButton::LeftButton) {
        if (star_rating_label->underMouse()) {
            UpdateStarRating(current_star_count_highlighted, true);
        }
    }
    QWidget::mousePressEvent(mouse_event);
}
