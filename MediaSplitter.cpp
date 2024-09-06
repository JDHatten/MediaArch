#include "MediaSplitter.h"

// TODO: make compatible with vertical orientation
MediaSplitter::MediaSplitter(QWidget* parent) : QSplitter(parent)
{
    setMouseTracking(true);
    connect(this, &QSplitter::splitterMoved, this, &MediaSplitter::SplitterMoving);
}
MediaSplitter::~MediaSplitter() {}

void MediaSplitter::addWidget(QWidget* widget, bool lock_resizing, bool allow_handle_resizing)
{
    qDebug() << "MediaSplitter::addWidget";
    QSplitter::addWidget(widget);
    if (lock_resizing)
        lockWidgetResizing(count() - 1, allow_handle_resizing);
}

void MediaSplitter::insertWidget(int index, QWidget* widget, bool lock_resizing, bool allow_handle_resizing)
{
    qDebug() << "MediaSplitter::insertWidget";
    QSplitter::insertWidget(index, widget);
    if (lock_resizing)
        lockWidgetResizing(index, allow_handle_resizing);
}

void MediaSplitter::SplitterMoving(int position, int right_widget_index)
{
    int  left_widget_index = right_widget_index - 1;
    bool left_widget_collapsed = widget(left_widget_index)->property(MA::Property::Collapsed).toBool();
    bool left_widget_visible = widget(left_widget_index)->visibleRegion().isEmpty();
    bool right_widget_collapsed = widget(right_widget_index)->property(MA::Property::Collapsed).toBool();
    bool right_widget_visible = widget(right_widget_index)->visibleRegion().isEmpty();
    /*/
    qDebug() << "SplitterMoving (indexes:" << left_widget_index << "<>" << right_widget_index
        << " |  position:" << position
        << " |  collapsed:" << left_widget_visible
        << "<>" << right_widget_visible << ")";//*/

    if (not is_splitter_resizing) {
        setIsResizing(true);
    }
    if (not left_widget_collapsed && left_widget_visible) {
        widget(left_widget_index)->setProperty(MA::Property::Collapsed, true);
        emit widgetCollapsed(left_widget_index);
        qDebug() << "Splitter widget collapsed at index:" << left_widget_index;
    }
    else if (left_widget_collapsed && not left_widget_visible) {
        widget(left_widget_index)->setProperty(MA::Property::Collapsed, false);
        emit widgetRestored(left_widget_index);
        qDebug() << "Splitter widget restored at index:" << left_widget_index;
    }
    else if (not right_widget_collapsed && right_widget_visible) {
        widget(right_widget_index)->setProperty(MA::Property::Collapsed, true);
        emit widgetCollapsed(right_widget_index);
        qDebug() << "Splitter widget collapsed at index:" << right_widget_index;
    }
    else if (right_widget_collapsed && not right_widget_visible) {
        widget(right_widget_index)->setProperty(MA::Property::Collapsed, false);
        emit widgetRestored(right_widget_index);
        qDebug() << "Splitter widget restored at index:" << right_widget_index;
    }
}

bool MediaSplitter::isResizing() const
{
    return is_splitter_resizing;
}

void MediaSplitter::setIsResizing(bool resizing)
{
    is_splitter_resizing = resizing;
    if (resizing) {
        qDebug() << "Splitter Resizing Started";
        for (int i : handle_resizing_only_widgets) {
            widget(i)->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            widget(i)->setMinimumWidth(40);
            widget(i)->setMaximumWidth(QWIDGETSIZE_MAX);
        }
    }
    else {
        qDebug() << "Splitter Resizing Ended";
        for (int i : handle_resizing_only_widgets) {
            widget(i)->setFixedWidth(sizes().at(i));
            widget(i)->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Expanding);
        }
    }
}

bool MediaSplitter::restoreState(const QByteArray& state)
{
    setIsResizing(true);
    bool state_restored = QSplitter::restoreState(state);
    setIsResizing(false);
    return state_restored;
}

void MediaSplitter::setSizes(const QList<int>& sizes)
{
    setIsResizing(true);
    QSplitter::setSizes(sizes);
    setIsResizing(false);
}

void MediaSplitter::lockWidgetResizing(int index, bool allow_handle_resizing)
{
    if (count()) {
        if (index < 0)
            index = 0;
        if (index >= count())
            index = count() - 1;

        //widget(index)->setFixedWidth(sizes().at(index));
        widget(index)->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Expanding);
        if (allow_handle_resizing) {
            if (not handle_resizing_only_widgets.contains(index))
                handle_resizing_only_widgets.append(index);
        }
    }
    else {
        qWarning() << "WARNING: Failed to lock widget as none have been added to the MediaSplitter yet.";
    }
}

bool MediaSplitter::isWidgetCollapsed(int index)
{
    return widget(index)->property(MA::Property::Collapsed).toBool();
}

bool MediaSplitter::event(QEvent* ms_event)
{
    return QSplitter::event(ms_event);
}

void MediaSplitter::resizeEvent(QResizeEvent* resize_event)
{
    QSplitter::resizeEvent(resize_event);
}

void MediaSplitter::paintEvent(QPaintEvent* paint_event)
{
    QSplitter::paintEvent(paint_event);
}
