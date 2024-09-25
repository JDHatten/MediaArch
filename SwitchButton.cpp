#include "SwitchButton.h"


SwitchButton::SwitchButton(QWidget* parent)
    : QWidget(parent), _value(false), _duration(100), _enabled(true)
{
    _initialDefaults();
    _initialBuild(Style::ONOFF, { "ON","OFF" });
}
SwitchButton::SwitchButton(QWidget* parent, Style style)
    : QWidget(parent), _value(false), _duration(100), _enabled(true)
{
    _initialDefaults();
    _initialBuild(style, { "ON","OFF" });
}
SwitchButton::SwitchButton(QWidget* parent, QStringList custom_text)
    : QWidget(parent), _value(false), _duration(100), _enabled(true)
{
    _initialDefaults();
    _initialBuild(Style::CUSTOM, custom_text);
}
SwitchButton::SwitchButton(QColor on_outline_color, QLinearGradient on_color_gradient, QLinearGradient off_color_gradient, QWidget* parent)
    : QWidget(parent), _value(false), _duration(100), _enabled(true), _on_color(on_outline_color), _on_color_lg(on_color_gradient), _off_color_lg(off_color_gradient)
{
    _initialBuild(Style::ONOFF, { "ON","OFF" });
}
SwitchButton::SwitchButton(QColor on_outline_color, QLinearGradient on_color_gradient, QLinearGradient off_color_gradient, Style style, QWidget* parent)
    : QWidget(parent), _value(false), _duration(100), _enabled(true), _on_color(on_outline_color), _on_color_lg(on_color_gradient), _off_color_lg(off_color_gradient)
{
    _initialBuild(style, { "ON","OFF" });
}
SwitchButton::SwitchButton(QColor on_outline_color, QLinearGradient on_color_gradient, QLinearGradient off_color_gradient, QStringList custom_text, QWidget* parent)
    : QWidget(parent), _value(false), _duration(100), _enabled(true), _on_color(on_outline_color), _on_color_lg(on_color_gradient), _off_color_lg(off_color_gradient)
{
    _initialBuild(Style::CUSTOM, custom_text);
}

SwitchButton::~SwitchButton()
{
    delete _circle;
    delete _background;
    delete _label_off;
    delete _label_on;
    delete __btn_move;
    delete __back_move;
}

void SwitchButton::_initialDefaults()
{
    _on_color = QColor(154, 205, 50); // Green
    _off_color = QColor(255, 255, 0); // TODO: not used, unsure where/how to use it
    
    // Green
    _on_color_lg = QLinearGradient(0, 25, 70, 0);
    _on_color_lg.setColorAt(0, QColor(154, 194, 50));
    _on_color_lg.setColorAt(0.25, QColor(154, 210, 50));
    _on_color_lg.setColorAt(0.95, QColor(154, 194, 50));

    _off_color_lg = QLinearGradient(50, 30, 35, 0);
    _off_color_lg.setColorAt(0, QColor(230, 230, 230));
    _off_color_lg.setColorAt(0.25, QColor(255, 255, 255));
    _off_color_lg.setColorAt(0.82, QColor(255, 255, 255));
    _off_color_lg.setColorAt(1, QColor(230, 230, 230));
}

void SwitchButton::_initialBuild(Style style, QStringList custom_text)
{
    _pen_color = QColor(120, 120, 120);

    _lg = QLinearGradient(35, 30, 35, 0);
    _lg.setColorAt(0, QColor(210, 210, 210));
    _lg.setColorAt(0.25, QColor(255, 255, 255));
    _lg.setColorAt(0.82, QColor(255, 255, 255));
    _lg.setColorAt(1, QColor(210, 210, 210));

    _lg_disabled = QLinearGradient(50, 30, 35, 0);
    _lg_disabled.setColorAt(0, QColor(200, 200, 200));
    _lg_disabled.setColorAt(0.25, QColor(230, 230, 230));
    _lg_disabled.setColorAt(0.82, QColor(230, 230, 230));
    _lg_disabled.setColorAt(1, QColor(200, 200, 200));

    _border_radius = 12;
    _label_off = new QLabel(this);
    //_background = new SwitchBackground(this, _on_color);
    _background = new SwitchBackground(_on_color, _on_color_lg, _lg_disabled, false, this);
    _label_on = new QLabel(this);
    _circle = new SwitchCircle(this, _off_color);
    __btn_move = new QPropertyAnimation(this);
    __back_move = new QPropertyAnimation(this);

    __btn_move->setTargetObject(_circle);
    __btn_move->setPropertyName("pos");
    __back_move->setTargetObject(_background);
    __back_move->setPropertyName("size");

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    _label_off->setText("Off");
    _label_on->setText("On");
    _label_off->move(31, 4);
    _label_on->move(15, 4);
    setFixedSize(QSize(60, 24));

    if (style == Style::YESNO) {
        _label_off->setText("No");
        _label_on->setText("Yes");
        _label_off->move(33, 4);
        _label_on->move(12, 4);
        setFixedSize(QSize(60, 24));
    }
    else if (style == Style::BOOL) {
        _label_off->setText("False");
        _label_on->setText("True");
        _label_off->move(37, 5);
        _label_on->move(12, 5);
        setFixedSize(QSize(75, 24));
    }
    else if (style == Style::CUSTOM) { // TODO: Need custom sizing and spacing prams (could be based on string length)
        _label_off->setText(custom_text.last());
        _label_on->setText(custom_text.first());
        _label_off->move(27, 4);
        _label_on->move(15, 4);
        setFixedSize(QSize(102, 24));
    }
    else if (style == Style::EMPTY) {
        _label_off->setText("");
        _label_on->setText("");
        _label_off->move(31, 4);
        _label_on->move(12, 4);
        setFixedSize(QSize(45, 24));
    }

    _label_off->setStyleSheet("color: rgb(120, 120, 120); font-weight: bold;");
    _label_on->setStyleSheet("color: rgb(255, 255, 255); font-weight: bold;");
    _label_on->hide();

    _background->resize(20, 20);

    _background->move(2, 2);
    _circle->move(2, 2);
}

void SwitchButton::paintEvent(QPaintEvent*)
{
    QPainter* painter = new QPainter;
    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen pen(Qt::NoPen);
    painter->setPen(pen);

    painter->setBrush(_pen_color);
    painter->drawRoundedRect(0, 0
        , width(), height()
        , 12, 12);

    painter->setBrush(_lg);
    painter->drawRoundedRect(1, 1
        , width() - 2, height() - 2
        , 10, 10);

    painter->setBrush(QColor(210, 210, 210));
    painter->drawRoundedRect(2, 2
        , width() - 4, height() - 4
        , 10, 10);

    if (_enabled) {
        painter->setBrush(_off_color_lg);
        painter->drawRoundedRect(3, 3
            , width() - 6, height() - 6
            , 7, 7);
    }
    else {
        painter->setBrush(_lg_disabled);
        painter->drawRoundedRect(3, 3
            , width() - 6, height() - 6
            , 7, 7);
    }
    painter->end();
}

void SwitchButton::mousePressEvent(QMouseEvent*)
{
    if (!_enabled)
        return;

    __btn_move->stop();
    __back_move->stop();

    __btn_move->setDuration(_duration);
    __back_move->setDuration(_duration);

    int hback = 20;
    QSize initial_size(hback, hback);
    QSize final_size(width() - 4, hback);

    int xi = 2;
    int y = 2;
    int xf = width() - 22;

    if (_value) {
        final_size = QSize(hback, hback);
        initial_size = QSize(width() - 4, hback);
        xi = xf;
        xf = 2;
        _label_on->hide();
    }
    else {
        _label_on->show();
    }

    __btn_move->setStartValue(QPoint(xi, y));
    __btn_move->setEndValue(QPoint(xf, y));

    __back_move->setStartValue(initial_size);
    __back_move->setEndValue(final_size);

    __btn_move->start();
    __back_move->start();

    // Assigning new current value
    _value = !_value;
    emit valueChanged(_value);
}

void SwitchButton::setEnabled(bool flag)
{
    _enabled = flag;
    _circle->setEnabled(flag);
    _background->setEnabled(flag);
    if (flag)
        _label_on->show();
    else {
        if (value())
            _label_on->show();
        else
            _label_on->hide();
    }
    QWidget::setEnabled(flag);
}

void SwitchButton::setDuration(int time)
{
    _duration = time;
}

void SwitchButton::setValue(bool flag)
{
    if (flag == value())
        return;
    else {
        mousePressEvent(nullptr); // Fix to make sure the toggle visually happens when programmatically calling setValue.
        _value = flag;
        _update();
        //setEnabled(_enabled);
    }
}

bool SwitchButton::value() const
{
    return _value;
}

void SwitchButton::_update()
{
    int hback = 20;
    QSize final_size(width() - 4, hback);

    int y = 2;
    int xf = width() - 22;

    if (_value) {
        final_size = QSize(hback, hback);
        xf = 2;
    }

    _circle->move(QPoint(xf, y));
    _background->resize(final_size);
}


SwitchButton::SwitchBackground::SwitchBackground(bool rect, QWidget* parent)
    : QWidget(parent), _color(QColor(255, 255, 255)), _rect(rect), _border_radius(12), _pen_color(QColor(170, 170, 170))
{
    _initialBuild(true);
}
SwitchButton::SwitchBackground::SwitchBackground(QColor on_outline_color, QLinearGradient on_color_gradient, QLinearGradient disabled_color_gradient, bool rect, QWidget* parent)
    : QWidget(parent), _color(on_outline_color), _lg(on_color_gradient), _color_disabled(QColor(150, 150, 150)), _lg_disabled(disabled_color_gradient), _rect(rect), _border_radius(12), _pen_color(QColor(170, 170, 170))
{
    _initialBuild(false);
}

SwitchButton::SwitchBackground::~SwitchBackground() {}

void SwitchButton::SwitchBackground::_initialBuild(bool use_defaults)
{
    setFixedHeight(20);

    if (use_defaults) {

        _color = QColor(154, 190, 50);
        _color_disabled = QColor(150, 150, 150);

        // Green
        _lg = QLinearGradient(0, 25, 70, 0);
        _lg.setColorAt(0, QColor(154, 194, 50));
        _lg.setColorAt(0.25, QColor(154, 210, 50));
        _lg.setColorAt(0.95, QColor(154, 194, 50));

        _lg_disabled = QLinearGradient(0, 25, 70, 0);
        _lg_disabled.setColorAt(0, QColor(190, 190, 190));
        _lg_disabled.setColorAt(0.25, QColor(230, 230, 230));
        _lg_disabled.setColorAt(0.95, QColor(190, 190, 190));
    }

    if (_rect)
        _border_radius = 0;

    _enabled = true;
}

void SwitchButton::SwitchBackground::paintEvent(QPaintEvent*)
{
    QPainter* painter = new QPainter;
    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen pen(Qt::NoPen);
    painter->setPen(pen);
    if (_enabled) {
        painter->setBrush(_color);
        painter->drawRoundedRect(0, 0
            , width(), height()
            , 10, 10);

        painter->setBrush(_lg);
        painter->drawRoundedRect(1, 1, width() - 2, height() - 2, 8, 8);
    }
    else {
        painter->setBrush(_color_disabled);
        painter->drawRoundedRect(0, 0
            , width(), height()
            , 10, 10);

        painter->setBrush(_lg_disabled);
        painter->drawRoundedRect(1, 1, width() - 2, height() - 2, 8, 8);
    }
    painter->end();
}

void SwitchButton::SwitchBackground::setEnabled(bool flag)
{
    _enabled = flag;
}


SwitchButton::SwitchCircle::SwitchCircle(QWidget* parent, QColor color, bool rect)
    : QWidget(parent), _rect(rect), _border_radius(12), _color(color), _pen_color(QColor(120, 120, 120))
{
    setFixedSize(20, 20);

    _rg = QRadialGradient(static_cast<int>(width() / 2), static_cast<int>(height() / 2), 12);
    _rg.setColorAt(0, QColor(255, 255, 255));
    _rg.setColorAt(0.6, QColor(255, 255, 255));
    _rg.setColorAt(1, QColor(205, 205, 205));

    _lg = QLinearGradient(3, 18, 20, 4);
    _lg.setColorAt(0, QColor(255, 255, 255));
    _lg.setColorAt(0.55, QColor(230, 230, 230));
    _lg.setColorAt(0.72, QColor(255, 255, 255));
    _lg.setColorAt(1, QColor(255, 255, 255));

    _lg_disabled = QLinearGradient(3, 18, 20, 4);
    _lg_disabled.setColorAt(0, QColor(230, 230, 230));
    _lg_disabled.setColorAt(0.55, QColor(210, 210, 210));
    _lg_disabled.setColorAt(0.72, QColor(230, 230, 230));
    _lg_disabled.setColorAt(1, QColor(230, 230, 230));

    _enabled = true;
}

SwitchButton::SwitchCircle::~SwitchCircle() {}

void SwitchButton::SwitchCircle::paintEvent(QPaintEvent*)
{
    QPainter* painter = new QPainter;
    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen pen(Qt::NoPen);
    painter->setPen(pen);
    painter->setBrush(_pen_color);

    painter->drawEllipse(0, 0, 20, 20);
    painter->setBrush(_rg);
    painter->drawEllipse(1, 1, 18, 18);

    painter->setBrush(QColor(210, 210, 210));
    painter->drawEllipse(2, 2, 16, 16);

    if (_enabled) {
        painter->setBrush(_lg);
        painter->drawEllipse(3, 3, 14, 14);
    }
    else {
        painter->setBrush(_lg_disabled);
        painter->drawEllipse(3, 3, 14, 14);
    }

    painter->end();
}

void SwitchButton::SwitchCircle::setEnabled(bool flag)
{
    _enabled = flag;
}