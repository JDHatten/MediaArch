#include "DetailedViewer.h"

DetailedViewer::DetailedViewer(UserSettings* user_settings, QWidget* parent) : QTreeWidget(parent), user_settings(user_settings)
{
    setAlternatingRowColors(true);
    //setContextMenuPolicy(Qt::CustomContextMenu); // done in MediaContextMenu
    setObjectName("DetailedViewer");
    //setSortingEnabled(true);
    header()->setSortIndicatorShown(true);
    header()->setSectionsClickable(true);
    //header()->setHighlightSections(true);
    //headerItem()->setData(0, Qt::ItemDataRole::FontRole, MA::Font::SettingsLabelQ); // Font
    //headerItem()->setData(0, Qt::ItemDataRole::BackgroundRole, MA::Color::PrimaryLightQ); // Not background color?
    //headerItem()->setData(0, Qt::ItemDataRole::ForegroundRole, MA::Color::PrimaryDarkQ); // Font Color
    
    //setSelectionMode(QAbstractItemView::SelectionMode::MultiSelection);

    setStyleSheet(QString(
        "QHeaderView {" // ?
            "font-family: %8;"
            "font-size: %6px;"
        "}"
        "QHeaderView::section {"
            "background-color: %1;"
            "color: %5;"
            "font-family: %7;"
            "font-size: %6px;"
            //"font: bold italic %5 %8;"
        "}"

        "DetailedViewer {"
            "alternate-background-color: %2;"
            "background: %3;"
            "border: 1px solid %4;"
            "color: %5;"
            "font-family: %7;"//Courier New;"
            "font-size: %8px;"
        "}"

        //"DetailedViewer::item {"
        //    "background-color: #66FF66;"
        //    //"border: 0;"
        //    "border: 2px solid #0FEFEF;"
        //    "color: #DB9B00;"
        //    "font-family: Courier New;"
        //    "margin-right: 160px"
        //    //"margin-left: 60px"
        //    "padding: 0px;"
        //    //"margin-right: 160px"
        //    //"margin-left: 60px"
        //    "border-top-color: transparent;"
        //    "border-bottom-color: transparent;"
        //"}"

    )
        .arg(MA::Color::PrimaryLightHex) // %1
        .arg(MA::Color::SecondaryHex) // %2
        .arg(MA::Color::SecondaryLightHex) // %3
        .arg(MA::Color::OutlineHex) // %4
        .arg(MA::Color::OutlineDarkHex) // %5
        .arg(MA::Font::SizeTitle) // %6
        .arg(MA::Font::General) // %7
        .arg(MA::Font::Settings) // %8
    );
    qDebug() << styleSheet();

    setColumnLabels(
        user_settings->getUserSetting(Field::Settings::ColumnLabelsTable).toStringList(),
        user_settings->getUserSetting(Field::Settings::ColumnLabelsField).toStringList());

    // Right-Click Context Menu
    media_context_menu = new MediaContextMenu(MA::ContextMenu::Menu::DetailedView, this);
    media_context_menu->addMediaAction(MA::ContextMenu::Action::Open, false, true);
    media_context_menu->addMediaActions({
        MA::ContextMenu::Action::RefreshMetadata,
        MA::ContextMenu::Action::CreateThumbnail
        });
    media_context_menu->addMediaAction(MA::ContextMenu::Action::RefreshDirectory, true);
    connect(media_context_menu, &MediaContextMenu::actionTriggered, this,
        [this](MA::ContextMenu::Action action) {
            if (MA::ContextMenu::Open == action) {
                if (currently_selected_media_item)
                    emit openItemInitiated(currently_selected_media_item);
            }
            else if (MA::ContextMenu::RefreshMetadata == action) {
                if (currently_selected_media_item) {
                    currently_selected_media_item->setLoading(true);
                    emit updateMediaItemData(currently_selected_media_item->getFilePath(), MA::Database::Update::Metadata);
                }
            }
            else if (MA::ContextMenu::CreateThumbnail == action) {
                if (currently_selected_media_item) {
                    currently_selected_media_item->setLoading(true);
                    emit updateMediaItemData(currently_selected_media_item->getFilePath(), MA::Database::Update::AllFileThumbnails);
                    currently_selected_media_item->stopThumbnailTimer();
                }
            }
            else if (MA::ContextMenu::RefreshDirectory == action) {
                emit refresh();
            }
        });//*/

    // Column Sorting
    connect(header(), &QHeaderView::sectionClicked,
        [this](int logical_column_index) {

            //current_metadata_sorter.table = columns.getTable(logical_column_index);
            //current_metadata_sorter.field = columns.getField(logical_column_index);

            current_metadata_sorter.table = columns.getTable(columns.indexOf(logical_column_index));
            current_metadata_sorter.field = columns.getField(columns.indexOf(logical_column_index));

            qDebug() << "columns.field(logical_column_index)" << columns.getField(logical_column_index) << "-"<< logical_column_index;
            qDebug() << "columns.field(logical_column_index)" << columns.getField(columns.indexOf(logical_column_index));



            // TODO: Always sort ascending when clicking new/different column [Done] Or String Ascending, Number Descending [How?]

            if (last_sorted_column == logical_column_index) {
                if (current_metadata_sorter.order == MA::Sort::Order::Ascending) {
                    header()->setSortIndicator(logical_column_index, Qt::SortOrder::DescendingOrder);
                    current_metadata_sorter.order = MA::Sort::Order::Descending;
                }
                else {
                    header()->setSortIndicator(logical_column_index, Qt::SortOrder::AscendingOrder);
                    current_metadata_sorter.order = MA::Sort::Order::Ascending;
                }
            }
            else {
                header()->setSortIndicator(logical_column_index, Qt::SortOrder::AscendingOrder);
                current_metadata_sorter.order = MA::Sort::Order::Ascending;
            }
            last_sorted_column = logical_column_index;

            emit columnSorted(current_metadata_sorter);
        });
    connect(header(), &QHeaderView::sectionMoved,
        [this](int original_index, int previous_index, int new_index) {
            qDebug() << "QHeaderView::sectionMoved() original / previous / new:" << original_index << "/" << previous_index << "/" << new_index;
            columns.moveColumnIndex(previous_index, new_index);
            columns.saveColumns(DetailedViewer::user_settings, true);
            qDebug() << columns.field_list;
        });


    // Item Selecting
    connect(this, &QTreeWidget::itemActivated, this, &DetailedViewer::SetCurrentlySelectedMediaItem);
    connect(this, &QTreeWidget::itemPressed, this, &DetailedViewer::SetCurrentlySelectedMediaItem);
    connect(this, &QTreeWidget::itemDoubleClicked,
        [this](QTreeWidgetItem* item, int column) {
            emit openItemInitiated(GetMediaItem(item));
            //item->setSelected(true);
        });

}

DetailedViewer::~DetailedViewer()
{
    //emit itemUnselected();
    media_context_menu->deleteLater();
}

void DetailedViewer::populate(QVector<MediaItem*>* media_item_list, bool full_list)
{
    if (full_list) {
        media_item_full_list = media_item_list;
        media_item_display_list = nullptr;
        currently_selected_media_item = nullptr;
        emit itemUnselected();
    }
    else {
        media_item_display_list = media_item_list;
    }
    int currently_selected_media_item_index = -2;
    if (currently_selected_media_item) {
        currently_selected_media_item_index = media_item_list->indexOf(currently_selected_media_item);
    }
    
    clear();
    qsizetype media_item_index = -1;
    for (auto* media_item : *media_item_list) {
        qsizetype i = 0;
        QTreeWidgetItem* file = new QTreeWidgetItem(this);
        QCheckBox* file_selector = new QCheckBox(this);
        file_selector->setChecked(media_item->isSelected());
        connect(file_selector, &QCheckBox::toggled, media_item, &MediaItem::setSelected);
        connect(file_selector, &QCheckBox::toggled,
            [this](bool checked) {
                if (last_sorted_column == 0)
                    emit columnSorted(current_metadata_sorter);
            });
        setItemWidget(file, i, file_selector);

        qsizetype file_name_column_index = columns.indexOf(Table::General, Field::General::FilePath);

        if (file_name_column_index > -1) {
            file->setText(file_name_column_index, media_item->getFileName());
            file->setData(file_name_column_index, MA::Role::ItemIndex, ++media_item_index);
            file->setData(file_name_column_index, MA::Role::FilePath, media_item->getFilePathString());

            if (media_item->isFolder())
                file->setIcon(file_name_column_index, QIcon(MA::Resources::IconFolderOn));
            else if (media_item->mediaType() & MA::Media::Type::Audio)
                file->setIcon(file_name_column_index, QIcon(MA::Resources::IconAudioOn));
            else if (media_item->mediaType() & MA::Media::Type::Other)
                file->setIcon(file_name_column_index, QIcon(MA::Resources::IconOtherOn));
            else if (media_item->mediaType() & MA::Media::Type::Image)
                file->setIcon(file_name_column_index, QIcon(MA::Resources::IconImageOn));
            else if (media_item->mediaType() & MA::Media::Type::Music)
                file->setIcon(file_name_column_index, QIcon(MA::Resources::IconMusicOn));
            else if (media_item->mediaType() & MA::Media::Type::Video)
                file->setIcon(file_name_column_index, QIcon(MA::Resources::IconVideoOn));
        }

        if (media_item->isMetadataLoadedIn()) {
            for (i = 1; i < columnCount(); i++) {
                if (file_name_column_index != i)
                    //file->setText(i, media_item->getFormattedMetadata(columns.getTable(i), columns.getField(i)));
                    file->setText(i, media_item->getFormattedMetadata(columns.getTable(columns.indexOf(i)), columns.getField(columns.indexOf(i))));
            }
        }

        if (currently_selected_media_item_index == media_item_index) {
            file->setSelected(true);
            setCurrentItem(file);
            SetCurrentlySelectedMediaItem(file, 1);
        }
        //file->setData(0, Qt::ItemDataRole::FontRole, MA::Font::SettingsValueQ);
        //file->setFont(MA::Font::SettingsValueQ);

    }
    for (qsizetype i = 0; i < columnCount(); i++) {
        //headerItem()->setData(i, Qt::ItemDataRole::FontRole, MA::Font::SettingsLabelQ); // Font (Only way to make this particular font work)
        //headerItem()->setData(i, Qt::ItemDataRole::BackgroundRole, MA::Color::PrimaryLightQ); // Not background color?
        //headerItem()->setData(i, Qt::ItemDataRole::ForegroundRole, MA::Color::PrimaryDarkQ); // Font Color

        //headerItem()->setBackground(i, QBrush(MA::Color::PrimaryLG));
        //header()->section(i, QBrush(MA::Color::PrimaryLG));

        resizeColumnToContents(i); // TODO: user setting, AutoResizeColumnsToContents, otherwise save each column's width?
    }

    //QPalette palette = header()->palette();
    //// Set the normal/active, background color
    //// QPalette::Background is obsolete, use QPalette::Window
    //palette.setColor(QPalette::Normal, QPalette::Window, Qt::red);
    //// Set the palette on the header.
    //header()->setPalette(palette);
}

MediaItem* DetailedViewer::GetMediaItem(QTreeWidgetItem* item)
{
    if (media_item_display_list)
        return media_item_display_list->at(item->data(1, MA::Role::ItemIndex).toInt());
    else
        return media_item_full_list->at(item->data(1, MA::Role::ItemIndex).toInt());
}

void DetailedViewer::SetCurrentlySelectedMediaItem(QTreeWidgetItem* item, int column)
{
    currently_selected_media_item = GetMediaItem(item);
    emit itemSelected(currently_selected_media_item);
}

void DetailedViewer::setColumnLabels(QStringList table, QStringList field)
{
    columns = ColumnLabels(table, field);
    setColumnCount(columns.count());

    for (qsizetype i = 0; i < columnCount(); i++) {
        //setHeaderLabel(columns.text(i));
        headerItem()->setText(i, columns.columnText(i));
        headerItem()->setData(i, MA::Role::Table, columns.getTable(i));
        headerItem()->setData(i, MA::Role::Field, columns.getField(i));
    }
    //setHeaderLabels(table);

    //header()->setBackgroundRole(MA::Color::PrimaryLightQ);
    //header()->setForegroundRole(QPalette::ColorRole::Base (MA::Color::OutlineQ, MA::Color::PrimaryQ));
    //headerItem()->setBackground(0, QBrush(MA::Color::PrimaryQ));
    //headerItem()->set(0, QBrush(MA::Color::PrimaryQ));

    //header()->setFont(MA::Font::SettingsLabelQ); 
    //header()->setStyleSheet("");

}

void DetailedViewer::setColumnSorter(MA::Sort::Metadata sorter)
{
    current_metadata_sorter = sorter;
    UpdateSortIndicator();
}
void DetailedViewer::setColumnSorter(QString table, QString field, MA::Sort::Order order)
{
    current_metadata_sorter.table = table;
    current_metadata_sorter.field = field;
    current_metadata_sorter.order = order;
    UpdateSortIndicator();
}
void DetailedViewer::setColumnSorter(QString table, QString field)
{
    current_metadata_sorter.table = table;
    current_metadata_sorter.field = field;
    UpdateSortIndicator();
}
void DetailedViewer::setColumnSorter(MA::Sort::Order order)
{
    current_metadata_sorter.order = order;
    UpdateSortIndicator();
}

void DetailedViewer::UpdateSortIndicator()
{
    int column_index = columns.indexOf(current_metadata_sorter.table, current_metadata_sorter.field); // Note: -1 = Off
    Qt::SortOrder order = Qt::SortOrder(current_metadata_sorter.order);
    header()->setSortIndicator(column_index, order);
    last_sorted_column = column_index;
}

void DetailedViewer::mouseMoveEvent(QMouseEvent* mouse_event)
{
    //qDebug() << "(Event) Mouse over" << MediaViewerName;

    QTreeWidget::mouseMoveEvent(mouse_event);
}

void DetailedViewer::mousePressEvent(QMouseEvent* mouse_event)
{
    //qDebug() << "(QMouseEvent) Clicked" << MediaViewerName << " -Type:" << mouse_event->type();

    //QTreeWidgetItem();

    currently_selected_media_item = nullptr;
    clearSelection();

    QTreeWidget::mousePressEvent(mouse_event);
    //qDebug() << selectedItems() << currentItem();

    if (selectedItems().empty())
        emit itemUnselected();
}

void DetailedViewer::mouseReleaseEvent(QMouseEvent* mouse_event)
{
    //if (selection_band->isVisible()) {
    //    selection_band->hide();
    //    for (int i = 0; i < grid->count(); i++) {
    //        QWidget* item = grid->itemAt(i)->widget();
    //        bool selected = selection_band->geometry().intersects(item->geometry());
    //        qDebug() << item->objectName() << i << "Selected:" << selected << " - Location:" << item->geometry() << "Selector:" << selection_band->geometry();
    //        // TODO: finish after making multiple items selectable
    //    }
    //}


    QTreeWidget::mouseReleaseEvent(mouse_event);
}
