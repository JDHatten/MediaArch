#pragma once
#ifndef MEDIASPLITTER_H
#define MEDIASPLITTER_H

//#include <QEvent>
//#include <QPaintEvent>
//#include <QResizeEvent>
#include <QSplitter>
#include "MediaSpaces.h"

class MediaSplitter : public QSplitter
{
    Q_OBJECT

public:

    MediaSplitter(QWidget* parent = nullptr);
    ~MediaSplitter();

    void addWidget(QWidget* widget, bool lock_resizing = false, bool allow_handle_resizing = false);
    void insertWidget(int index, QWidget* widget, bool lock_resizing = false, bool allow_handle_resizing = false);
    bool isResizing() const;
    void setIsResizing(bool resizing);
    bool restoreState(const QByteArray& state);
    void setSizes(const QList<int>& sizes);
    void lockWidgetResizing(int index, bool allow_handle_resizing = false);
    bool isWidgetCollapsed(int index);

private:

    bool is_splitter_resizing = false;
    QList<int> handle_resizing_only_widgets;

signals:

    void widgetCollapsed(int index);
    void widgetRestored(int index);

private slots:

    void SplitterMoving(int position, int index);

protected:

    bool event(QEvent* ms_event) override;
    void resizeEvent(QResizeEvent* resize_event) override;
    void paintEvent(QPaintEvent* paint_event) override;

};

#endif // MEDIASPLITTER_H