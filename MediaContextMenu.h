#pragma once
#ifndef MEDIACONTEXTMENU_H
#define MEDIACONTEXTMENU_H

#include <QLayout>
#include <QMenu>
#include <QScrollArea>
#include <QTreeWidget>

//#include "DirectoryTreeViewer.h"
#include "MediaSpaces.h"


class MediaContextMenu : public QMenu
{
    Q_OBJECT
        Q_DISABLE_COPY(MediaContextMenu)

public:

    explicit MediaContextMenu(MA::ContextMenu::Menu menu, QWidget* parent = Q_NULLPTR);
    ~MediaContextMenu();

    void addMediaActions(QList<MA::ContextMenu::Action> action_items, bool always_on = false);
    void addMediaAction(MA::ContextMenu::Action action_item, bool always_on = false, bool bold = false);
    void addMediaSubMenu(MA::ContextMenu::Menu menu);
    void changeTextOf(MA::ContextMenu::Action item, Text::Type to = Text::Type::Label);

private:

    MA::ContextMenu::Menu menu;
    QWidget* parent_widget = nullptr;

signals:

    void actionTriggered(MA::ContextMenu::Action);

public slots:

    void enableContextMenuItems(bool enabled);

private slots:

    void ShowContextMenu(const QPoint& pos);
    
protected:

};

#endif // MEDIACONTEXTMENU_H