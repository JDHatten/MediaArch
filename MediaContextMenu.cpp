#include "MediaContextMenu.h"

MediaContextMenu::MediaContextMenu(MA::ContextMenu::Menu menu, QWidget* parent)
    : QMenu(parent), menu(menu), parent_widget(parent)
{
    parent_widget->setContextMenuPolicy(Qt::CustomContextMenu);
    setTitle(Text::from(menu));
    connect(parent_widget, &QWidget::customContextMenuRequested, this, &MediaContextMenu::ShowContextMenu);
}

MediaContextMenu::~MediaContextMenu() {}

void MediaContextMenu::addMediaActions(QList<MA::ContextMenu::Action> action_items, bool always_on)
{
    for (auto& item : action_items) {
        addMediaAction(item, always_on);
    }
}

void MediaContextMenu::addMediaAction(MA::ContextMenu::Action action_item, bool always_on, bool bold)
{
    auto* action = new QAction(Text::from(action_item), this);
    action->setData(action_item);
    action->setProperty(MA::Property::AlwaysEnabled, always_on);

    if (action_item == MA::ContextMenu::Action::Divider) {
        action->setSeparator(true);
        action->setProperty(MA::Property::AlwaysEnabled, true);
    }
    else {
        if (bold)
            action->setFont(MA::Font::ContextMenuQBold);
        else
            action->setFont(MA::Font::ContextMenuQ);

        connect(action, &QAction::triggered,
            [=](bool checked) {
                emit actionTriggered(action_item);
            });
    }
    addAction(action);
}

void MediaContextMenu::addMediaSubMenu(MA::ContextMenu::Menu menu)
{
    auto* sub_menu = new QMenu(Text::from(menu), this);
    addMenu(sub_menu);
}

void MediaContextMenu::changeTextOf(MA::ContextMenu::Action item, Text::Type to)
{
    for (auto& action : actions()) {
        if (action->data() == item) {
            if (to == Text::Type::LabelAlt)
                action->setText(Text::from(item, Text::Type::LabelAlt));
            else
                action->setText(Text::from(item));
        }
    }
}

void MediaContextMenu::ShowContextMenu(const QPoint& pos)
{
    enableContextMenuItems(false);

    if (MA::ContextMenu::Menu::GridView == menu) {
        QLayout* grid = qobject_cast<QScrollArea*>(parent_widget)->widget()->layout();

        if (grid) {
            for (int i = 0; i < grid->count(); i++) {
                QWidget* item = grid->itemAt(i)->widget();
                bool item_under_mouse = item->geometry().intersects(QRect(pos, QSize()));
                if (item_under_mouse) {
                    enableContextMenuItems(true);
                    break;
                }
            }
        }
    }
    else if (MA::ContextMenu::Menu::DetailedView == menu) {
        QTreeWidgetItem* item = qobject_cast<QTreeWidget*>(parent_widget)->currentItem();

        if (item and item->isSelected())
            enableContextMenuItems(true);
    }
    else if (MA::ContextMenu::Menu::DirectoryView == menu) {
        // If SelectedItemValid property set, then check if the selected item is actually clicked or if empty space was clicked.
        // Note: Used when current/selected item isn't changed or unselected either way.
        QVariant selected_item_valid = qobject_cast<QTreeWidget*>(parent_widget)->property(MA::Property::SelectedItemValid);
        bool selected_item_clicked = true;
        if (selected_item_valid.isValid())
            selected_item_clicked = selected_item_valid.toBool();

        if (selected_item_clicked)
            enableContextMenuItems(true);
    }
    exec(parent_widget->mapToGlobal(pos));
}

void MediaContextMenu::enableContextMenuItems(bool enabled)
{
    for (auto& action : actions()) {
        if (action->property(MA::Property::AlwaysEnabled).toBool()) {
            action->setEnabled(true);
        }
        else {
            action->setEnabled(enabled);
        }
    }
}
