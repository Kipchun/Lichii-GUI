// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPolygonF>

#include "yuzu/game/home_theme.h"

namespace {
// Style sheets can only draw arrows from image files, so they are rendered once per colour.
QString ArrowImage(const QColor& color, bool up) {
    const QString dir = QDir::temp().filePath(QStringLiteral("lichii-home-theme"));
    const QString path = QStringLiteral("%1/arrow-%2-%3.png")
                             .arg(dir, up ? QStringLiteral("up") : QStringLiteral("down"),
                                  color.name().mid(1));
    if (!QFile::exists(path)) {
        QDir().mkpath(dir);
        QPixmap pixmap(20, 12);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(color, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (up) {
            p.drawPolyline(QPolygonF{QPointF(4, 9), QPointF(10, 3), QPointF(16, 9)});
        } else {
            p.drawPolyline(QPolygonF{QPointF(4, 3), QPointF(10, 9), QPointF(16, 3)});
        }
        p.end();
        pixmap.save(path);
    }
    return path;
}
} // namespace

HomeTheme HomeTheme::BasicBlack() {
    return {QColor("#2d2d2d"), QColor("#ffffff"), QColor("#6e6e6e"), QColor("#4b4b4b"),
            QColor("#1ec4e8"), QColor("#2df2ff"), QColor("#0789a8"), {}};
}

HomeTheme HomeTheme::BasicWhite() {
    return {QColor("#ebebeb"), QColor("#2d2d2d"), QColor("#a9a9a9"), QColor("#ffffff"),
            QColor("#0aa9d3"), QColor("#18d6ff"), QColor("#0478a0"), {}};
}

HomeTheme HomeTheme::FromId(const QString& id) {
    if (id == QStringLiteral("basic_white")) {
        return BasicWhite();
    }
    if (id == QStringLiteral("basic_black")) {
        return BasicBlack();
    }
    const bool dark = QApplication::palette().color(QPalette::Window).lightness() < 128;
    return dark ? BasicBlack() : BasicWhite();
}

QString HomeTheme::SettingsStyleSheet() const {
    const bool dark = background.lightness() < 128;
    const QColor secondary = MixColors(text, background, 0.38);
    const QColor field = dark ? MixColors(button, background, 0.45) : button;
    const QColor border = MixColors(text, background, 0.72);
    const QColor hover = MixColors(background, text, 0.08);
    const QColor selected_row = MixColors(background, accent, 0.12);

    // Every rule is scoped to the dialog so nothing leaks into the rest of the application.
    QString sheet = QStringLiteral(R"(
        QDialog#ConfigureDialog { background: @bg; }
        #ConfigureDialog QWidget { color: @text; background: transparent; }
        #ConfigureDialog QLabel, #ConfigureDialog QCheckBox, #ConfigureDialog QRadioButton,
        #ConfigureDialog QGroupBox { background: transparent; }
        #ConfigureDialog QLabel:disabled, #ConfigureDialog QCheckBox:disabled,
        #ConfigureDialog QRadioButton:disabled { color: @secondary; }
        #ConfigureDialog QLabel#SettingsTitle { font-size: 24px; }
        #ConfigureDialog QLabel#SettingsNotice { color: @secondary; }
        #ConfigureDialog QFrame#SettingsRule { background: @muted; border: none;
                                               min-height: 1px; max-height: 1px; }

        #ConfigureDialog QListWidget#selectorList { background: transparent; border: none;
            border-right: 1px solid @muted; outline: 0; font-size: 17px; padding-top: 6px; }
        #ConfigureDialog QListWidget#selectorList::item { padding: 11px 12px; color: @text;
            border-left: 4px solid transparent; }
        #ConfigureDialog QListWidget#selectorList::item:hover:!selected { background: @hover; }
        #ConfigureDialog QListWidget#selectorList::item:selected { color: @accent;
            background: @selrow; border-left: 4px solid @accent; }

        #ConfigureDialog QTabWidget::pane { border: none; background: @bg; }
        #ConfigureDialog QTabWidget > QWidget, #ConfigureDialog QStackedWidget,
        #ConfigureDialog QScrollArea, #ConfigureDialog QScrollArea > QWidget > QWidget {
            background: @bg; border: none; }
        #ConfigureDialog QTabBar { background: transparent; }
        #ConfigureDialog QTabBar::tab { background: transparent; color: @secondary; border: none;
            border-bottom: 3px solid transparent; padding: 8px 14px; font-size: 14px; }
        #ConfigureDialog QTabBar::tab:hover { color: @text; }
        #ConfigureDialog QTabBar::tab:selected { color: @text; border-bottom: 3px solid @accent; }

        #ConfigureDialog QGroupBox { border: 1px solid @border; border-radius: 8px;
            margin-top: 18px; padding: 10px 6px 6px 6px; }
        #ConfigureDialog QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 12px;
            padding: 0 4px; color: @secondary; }

        #ConfigureDialog QComboBox, #ConfigureDialog QSpinBox, #ConfigureDialog QDoubleSpinBox,
        #ConfigureDialog QLineEdit, #ConfigureDialog QTimeEdit, #ConfigureDialog QDateTimeEdit {
            background: @field; color: @text; border: 1px solid @border; border-radius: 6px;
            padding: 4px 8px; min-height: 22px; selection-background-color: @accent; }
        #ConfigureDialog QComboBox:focus, #ConfigureDialog QSpinBox:focus,
        #ConfigureDialog QDoubleSpinBox:focus, #ConfigureDialog QLineEdit:focus,
        #ConfigureDialog QTimeEdit:focus, #ConfigureDialog QDateTimeEdit:focus {
            border: 2px solid @accent; }
        #ConfigureDialog QComboBox:disabled, #ConfigureDialog QSpinBox:disabled,
        #ConfigureDialog QDoubleSpinBox:disabled, #ConfigureDialog QLineEdit:disabled {
            color: @secondary; }
        #ConfigureDialog QComboBox::drop-down { border: none; width: 26px; }
        #ConfigureDialog QComboBox::down-arrow { image: url(@arrowdown); width: 10px; height: 6px; }
        #ConfigureDialog QAbstractSpinBox::up-button, #ConfigureDialog QAbstractSpinBox::down-button {
            border: none; background: transparent; width: 20px; subcontrol-origin: border; }
        #ConfigureDialog QAbstractSpinBox::up-button { subcontrol-position: top right; }
        #ConfigureDialog QAbstractSpinBox::down-button { subcontrol-position: bottom right; }
        #ConfigureDialog QAbstractSpinBox::up-arrow { image: url(@arrowup); width: 10px; height: 6px; }
        #ConfigureDialog QAbstractSpinBox::down-arrow { image: url(@arrowdown); width: 10px; height: 6px; }
        #ConfigureDialog QComboBox QAbstractItemView { background: @field; color: @text;
            border: 1px solid @border; outline: 0; selection-background-color: @accent;
            selection-color: #ffffff; }

        #ConfigureDialog QPushButton, #ConfigureDialog QToolButton { background: @field; color: @text;
            border: 1px solid @border; border-radius: 6px; padding: 5px 14px; }
        #ConfigureDialog QPushButton:hover, #ConfigureDialog QToolButton:hover { border-color: @accent; }
        #ConfigureDialog QPushButton:focus, #ConfigureDialog QToolButton:focus { border: 2px solid @accent; }
        #ConfigureDialog QPushButton:pressed, #ConfigureDialog QToolButton:pressed { background: @hover; }
        #ConfigureDialog QPushButton:disabled, #ConfigureDialog QToolButton:disabled { color: @secondary; }
        #ConfigureDialog QDialogButtonBox QPushButton { background: transparent; border: none;
            border-radius: 6px; font-size: 16px; padding: 4px 10px; }
        #ConfigureDialog QDialogButtonBox QPushButton:hover { background: @hover; }
        #ConfigureDialog QDialogButtonBox QPushButton:focus { border: 2px solid @accent; }

        #ConfigureDialog QCheckBox:focus, #ConfigureDialog QRadioButton:focus,
        #ConfigureDialog QSlider:focus { background: @selrow; border-radius: 4px; }
        #ConfigureDialog QTreeView:focus, #ConfigureDialog QTableView:focus,
        #ConfigureDialog QListView:focus { border: 2px solid @accent; }
        #ConfigureDialog QCheckBox::indicator { width: 16px; height: 16px; border-radius: 4px;
            border: 2px solid @secondary; background: transparent; }
        #ConfigureDialog QRadioButton::indicator { width: 16px; height: 16px; border-radius: 10px;
            border: 2px solid @secondary; background: transparent; }
        #ConfigureDialog QCheckBox::indicator:checked, #ConfigureDialog QRadioButton::indicator:checked {
            background: @accent; border-color: @accent; }
        #ConfigureDialog QCheckBox::indicator:indeterminate { background: @secondary; }
        #ConfigureDialog QCheckBox::indicator:disabled, #ConfigureDialog QRadioButton::indicator:disabled {
            border-color: @border; }

        #ConfigureDialog QSlider::groove:horizontal { height: 4px; background: @border; border-radius: 2px; }
        #ConfigureDialog QSlider::sub-page:horizontal { background: @accent; border-radius: 2px; }
        #ConfigureDialog QSlider::handle:horizontal { background: @text; width: 16px; height: 16px;
            margin: -6px 0; border-radius: 8px; }

        #ConfigureDialog QTreeView, #ConfigureDialog QTableView, #ConfigureDialog QListView,
        #ConfigureDialog QTreeWidget, #ConfigureDialog QListWidget, #ConfigureDialog QTableWidget {
            background: @field; alternate-background-color: @hover; border: 1px solid @border;
            border-radius: 6px; outline: 0; selection-background-color: @accent; selection-color: #ffffff; }
        #ConfigureDialog QHeaderView::section { background: @bg; color: @secondary; border: none;
            border-bottom: 1px solid @border; padding: 5px 8px; }

        #ConfigureDialog QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
        #ConfigureDialog QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
        #ConfigureDialog QScrollBar::handle { background: @border; border-radius: 3px;
            min-height: 24px; min-width: 24px; }
        #ConfigureDialog QScrollBar::add-line, #ConfigureDialog QScrollBar::sub-line { width: 0; height: 0; }
        #ConfigureDialog QScrollBar::add-page, #ConfigureDialog QScrollBar::sub-page { background: transparent; }
    )");
    // Longer names first so @selrow is not caught by a shorter token.
    const std::pair<const char*, QColor> tokens[] = {
        {"@secondary", secondary}, {"@selrow", selected_row}, {"@accent", accent},
        {"@border", border},       {"@field", field},         {"@hover", hover},
        {"@muted", muted},         {"@text", text},           {"@bg", background},
    };
    sheet.replace(QStringLiteral("@arrowdown"), ArrowImage(text, false));
    sheet.replace(QStringLiteral("@arrowup"), ArrowImage(text, true));
    for (const auto& [token, color] : tokens) {
        sheet.replace(QLatin1String(token), color.name());
    }
    return sheet;
}

QColor MixColors(const QColor& from, const QColor& to, qreal amount) {
    return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * amount,
                            from.greenF() + (to.greenF() - from.greenF()) * amount,
                            from.blueF() + (to.blueF() - from.blueF()) * amount);
}

QPixmap DrawHomeSymbol(HomeSymbol symbol, const QColor& color, int size) {
    QPixmap pixmap(size * 2, size * 2);
    pixmap.setDevicePixelRatio(2);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(size / 48.0, size / 48.0);
    p.setPen(QPen(color, 3.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    switch (symbol) {
    case HomeSymbol::Gamepad: {
        QPainterPath path;
        path.moveTo(13, 12);
        path.cubicTo(5, 12, 5, 24, 3, 35);
        path.cubicTo(3, 41, 10, 40, 15, 31);
        path.lineTo(33, 31);
        path.cubicTo(38, 40, 45, 41, 45, 35);
        path.cubicTo(43, 24, 43, 12, 35, 12);
        path.closeSubpath();
        p.drawPath(path);
        p.drawLine(11, 21, 19, 21);
        p.drawLine(15, 17, 15, 25);
        p.setBrush(color);
        p.drawEllipse(QPointF(33, 18), 1.4, 1.4);
        p.drawEllipse(QPointF(37, 23), 1.4, 1.4);
        break;
    }
    case HomeSymbol::Settings:
        p.drawEllipse(QPointF(24, 24), 6.5, 6.5);
        p.drawEllipse(QPointF(24, 24), 14, 14);
        for (int i = 0; i < 8; ++i) {
            p.save();
            p.translate(24, 24);
            p.rotate(i * 45);
            p.setPen(QPen(color, 5, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(0, -17, 0, -20);
            p.restore();
        }
        break;
    case HomeSymbol::Handbook:
        p.drawRoundedRect(QRectF(6, 8, 36, 27), 3, 3);
        p.drawLine(13, 17, 35, 17);
        p.drawLine(13, 25, 29, 25);
        p.drawLine(17, 35, 23, 42);
        p.drawLine(23, 42, 29, 35);
        break;
    case HomeSymbol::Bag:
        p.drawPolygon(QPolygonF{QPointF(9, 17), QPointF(39, 17), QPointF(42, 42), QPointF(6, 42)});
        p.drawArc(QRectF(16, 6, 16, 20), 0, 180 * 16);
        break;
    case HomeSymbol::Album:
        p.drawRoundedRect(QRectF(5, 9, 38, 31), 3, 3);
        p.drawPolyline(QPolygonF{QPointF(6, 36), QPointF(19, 22), QPointF(27, 30), QPointF(33, 25),
                                 QPointF(42, 33)});
        break;
    case HomeSymbol::Power:
        p.drawArc(QRectF(8, 9, 32, 32), 125 * 16, 290 * 16);
        p.drawLine(24, 5, 24, 23);
        break;
    }
    return pixmap;
}

QIcon HomeButtonGlyph(const QString& glyph, const QColor& circle, const QColor& mark) {
    constexpr int Size = 64;
    QPixmap pixmap(Size, Size);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(circle);
    p.drawEllipse(QRectF(2, 2, Size - 4, Size - 4));
    p.setPen(mark);
    QFont font = p.font();
    font.setPixelSize(40);
    font.setBold(true);
    p.setFont(font);
    p.drawText(QRectF(0, -1, Size, Size), Qt::AlignCenter, glyph);
    return QIcon(pixmap);
}
