#pragma once
#ifndef SWITCHBUTTON_H
#define SWITCHBUTTON_H

#include <QLabel>
//#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QString>
#include <QWidget>


class SwitchButton : public QWidget
{
    Q_OBJECT
        Q_DISABLE_COPY(SwitchButton)

public:

    enum Style {
        YESNO, ONOFF, BOOL, CUSTOM, EMPTY
    };

public:

    explicit SwitchButton(QWidget* parent = nullptr);
    explicit SwitchButton(QWidget* parent = nullptr, Style style = Style::ONOFF);
    explicit SwitchButton(QWidget* parent = nullptr, QStringList custom_text = { "ON","OFF" });
    explicit SwitchButton(QColor on_outline_color, QLinearGradient on_color_gradient, QLinearGradient off_color_gradient, QWidget* parent = nullptr);
    explicit SwitchButton(QColor on_outline_color, QLinearGradient on_color_gradient, QLinearGradient off_color_gradient, Style style = Style::ONOFF, QWidget* parent = nullptr);
    explicit SwitchButton(QColor on_outline_color, QLinearGradient on_color_gradient, QLinearGradient off_color_gradient, QStringList custom_text = { "ON","OFF" }, QWidget* parent = nullptr);
    ~SwitchButton() override;

    void mousePressEvent(QMouseEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void setEnabled(bool);

    void setDuration(int);
    void setValue(bool);
    bool value() const;

signals:

    void valueChanged(bool new_value);

private:

    class SwitchCircle;
    class SwitchBackground;
    void _initialDefaults();
    void _initialBuild(Style style, QStringList custom_text);
    void _update();

private:

    bool _value;
    int  _duration;

    QLinearGradient _lg;
    QLinearGradient _lg_disabled;
    QLinearGradient _on_color_lg;
    QLinearGradient _off_color_lg;

    QColor _pen_color;
    QColor _off_color;
    QColor _on_color;
    int    _border_radius;

    // This order is important (these widgets overlap)
    QLabel* _label_off;
    SwitchBackground* _background;
    QLabel* _label_on;
    SwitchCircle* _circle;

    bool _enabled;

    QPropertyAnimation* __btn_move;
    QPropertyAnimation* __back_move;
};


class SwitchButton::SwitchBackground : public QWidget
{
    Q_OBJECT
        Q_DISABLE_COPY(SwitchBackground)

public:

    explicit SwitchBackground(bool rect = false, QWidget* parent = nullptr);
    explicit SwitchBackground(QColor on_outline_color, QLinearGradient on_color_gradient, QLinearGradient off_color_gradient, bool rect = false, QWidget* parent = nullptr);
    ~SwitchBackground() override;

    void paintEvent(QPaintEvent*) override;
    void setEnabled(bool);

private:

    bool _enabled;
    bool            _rect;
    int             _border_radius;
    QColor          _color;
    QColor          _color_disabled;
    QColor          _pen_color;
    QLinearGradient _lg;
    QLinearGradient _lg_disabled;
    
    void _initialBuild(bool use_defaults);
};


class SwitchButton::SwitchCircle : public QWidget
{
    Q_OBJECT
        Q_DISABLE_COPY(SwitchCircle)

public:

    explicit SwitchCircle(QWidget* parent = nullptr, QColor color = QColor(255, 255, 255), bool rect = false);
    ~SwitchCircle() override;

    void paintEvent(QPaintEvent*) override;
    void setEnabled(bool);

private:

    bool            _rect;
    int             _border_radius;
    QColor          _color;
    QColor          _pen_color;
    QRadialGradient _rg;
    QLinearGradient _lg;
    QLinearGradient _lg_disabled;

    bool _enabled;

};

#endif //SWITCHBUTTON_H