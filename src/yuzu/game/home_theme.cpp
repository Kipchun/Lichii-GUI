// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QGuiApplication>
#include <QStyleHints>
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
    return SystemPrefersDark() ? BasicBlack() : BasicWhite();
}

namespace {
struct DerivedColors {
    QColor secondary;
    QColor field;
    QColor border;
    QColor hover;
    QColor selected_row;
};

DerivedColors Derive(const HomeTheme& theme) {
    const bool dark = theme.background.lightness() < 128;
    return {
        MixColors(theme.text, theme.background, 0.38),
        dark ? MixColors(theme.button, theme.background, 0.45) : theme.button,
        MixColors(theme.text, theme.background, 0.72),
        MixColors(theme.background, theme.text, 0.08),
        MixColors(theme.background, theme.accent, 0.12),
    };
}

// Controls shared by the whole application and the settings page. "@s" starts every selector so
// the same rules can be scoped to one dialog or applied everywhere.
constexpr const char* ControlRules = R"(
    @sQGroupBox { border: 1px solid @border; border-radius: 8px; margin-top: 18px;
        padding: 10px 6px 6px 6px; }
    @sQGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 12px;
        padding: 0 4px; color: @secondary; }

    @sQComboBox, @sQSpinBox, @sQDoubleSpinBox, @sQLineEdit, @sQTimeEdit, @sQDateTimeEdit,
    @sQDateEdit { background: @field; color: @text; border: 1px solid @border; border-radius: 6px;
        padding: 4px 8px; min-height: 22px; selection-background-color: @accent; }
    @sQComboBox:focus, @sQSpinBox:focus, @sQDoubleSpinBox:focus, @sQLineEdit:focus,
    @sQTimeEdit:focus, @sQDateTimeEdit:focus, @sQDateEdit:focus { border: 2px solid @accent; }
    @sQComboBox:disabled, @sQSpinBox:disabled, @sQDoubleSpinBox:disabled, @sQLineEdit:disabled {
        color: @secondary; }
    @sQComboBox::drop-down { border: none; width: 26px; }
    @sQComboBox::down-arrow { image: url(@arrowdown); width: 10px; height: 6px; }
    @sQComboBox QAbstractItemView { background: @field; color: @text; border: 1px solid @border;
        outline: 0; selection-background-color: @accent; selection-color: #ffffff; }
    @sQAbstractSpinBox::up-button, @sQAbstractSpinBox::down-button { border: none;
        background: transparent; width: 20px; subcontrol-origin: border; }
    @sQAbstractSpinBox::up-button { subcontrol-position: top right; }
    @sQAbstractSpinBox::down-button { subcontrol-position: bottom right; }
    @sQAbstractSpinBox::up-arrow { image: url(@arrowup); width: 10px; height: 6px; }
    @sQAbstractSpinBox::down-arrow { image: url(@arrowdown); width: 10px; height: 6px; }
    @sQPlainTextEdit, @sQTextEdit, @sQTextBrowser { background: @field; color: @text;
        border: 1px solid @border; border-radius: 6px; selection-background-color: @accent; }

    @sQPushButton { background: @field; color: @text; border: 1px solid @border;
        border-radius: 6px; padding: 5px 14px; }
    @sQPushButton:hover { border-color: @accent; }
    @sQPushButton:focus { border: 2px solid @accent; }
    @sQPushButton:pressed { background: @hover; }
    @sQPushButton:disabled { color: @secondary; }
    @sQPushButton:flat { background: transparent; border: none; }

    @sQCheckBox, @sQRadioButton { background: transparent; }
    @sQCheckBox:disabled, @sQRadioButton:disabled { color: @secondary; }
    @sQCheckBox:focus, @sQRadioButton:focus, @sQSlider:focus { background: @selrow;
        border-radius: 4px; }
    @sQCheckBox::indicator { width: 16px; height: 16px; border-radius: 4px;
        border: 2px solid @secondary; background: transparent; }
    @sQRadioButton::indicator { width: 16px; height: 16px; border-radius: 10px;
        border: 2px solid @secondary; background: transparent; }
    @sQCheckBox::indicator:checked, @sQRadioButton::indicator:checked { background: @accent;
        border-color: @accent; }
    @sQCheckBox::indicator:indeterminate { background: @secondary; }
    @sQCheckBox::indicator:disabled, @sQRadioButton::indicator:disabled { border-color: @border; }

    @sQSlider::groove:horizontal { height: 4px; background: @border; border-radius: 2px; }
    @sQSlider::sub-page:horizontal { background: @accent; border-radius: 2px; }
    @sQSlider::handle:horizontal { background: @text; width: 16px; height: 16px; margin: -6px 0;
        border-radius: 8px; }
    @sQProgressBar { background: @field; border: 1px solid @border; border-radius: 6px;
        text-align: center; color: @text; }
    @sQProgressBar::chunk { background: @accent; border-radius: 5px; }

    @sQTreeView, @sQTableView, @sQListView { background: @field; alternate-background-color: @hover;
        border: 1px solid @border; border-radius: 6px; outline: 0;
        selection-background-color: @accent; selection-color: #ffffff; }
    @sQTreeView:focus, @sQTableView:focus, @sQListView:focus { border: 2px solid @accent; }
    @sQHeaderView::section { background: @bg; color: @secondary; border: none;
        border-bottom: 1px solid @border; padding: 5px 8px; }

    @sQTabWidget::pane { border: none; background: @bg; }
    @sQTabBar { background: transparent; }
    @sQTabBar::tab { background: transparent; color: @secondary; border: none;
        border-bottom: 3px solid transparent; padding: 8px 14px; }
    @sQTabBar::tab:hover { color: @text; }
    @sQTabBar::tab:selected { color: @text; border-bottom: 3px solid @accent; }

    @sQScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
    @sQScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
    @sQScrollBar::handle { background: @border; border-radius: 3px; min-height: 24px;
        min-width: 24px; }
    @sQScrollBar::add-line, @sQScrollBar::sub-line { width: 0; height: 0; }
    @sQScrollBar::add-page, @sQScrollBar::sub-page { background: transparent; }
)";

// The settings page's own chrome; every rule is scoped to the dialog.
constexpr const char* SettingsRules = R"(
    QDialog#ConfigureDialog { background: @bg; }
    #ConfigureDialog QWidget { color: @text; background: transparent; }
    #ConfigureDialog QLabel, #ConfigureDialog QGroupBox { background: transparent; }
    #ConfigureDialog QLabel:disabled { color: @secondary; }
    #ConfigureDialog QLabel#SettingsTitle { font-size: 24px; }
    #ConfigureDialog QLabel#SettingsNotice { color: @secondary; }
    #ConfigureDialog QFrame#SettingsRule { background: @muted; border: none; min-height: 1px;
        max-height: 1px; }
    #ConfigureDialog QListWidget#selectorList { background: transparent; border: none;
        border-right: 1px solid @muted; border-radius: 0; outline: 0; font-size: 17px;
        padding-top: 6px; }
    #ConfigureDialog QListWidget#selectorList::item { padding: 11px 12px; color: @text;
        border-left: 4px solid transparent; }
    #ConfigureDialog QListWidget#selectorList::item:hover:!selected { background: @hover; }
    #ConfigureDialog QListWidget#selectorList::item:selected { color: @accent; background: @selrow;
        border-left: 4px solid @accent; }
    #ConfigureDialog QTabWidget > QWidget, #ConfigureDialog QStackedWidget,
    #ConfigureDialog QScrollArea, #ConfigureDialog QScrollArea > QWidget > QWidget {
        background: @bg; border: none; }
    #ConfigureDialog QToolButton { background: @field; color: @text; border: 1px solid @border;
        border-radius: 6px; padding: 5px 14px; }
    #ConfigureDialog QToolButton:hover { border-color: @accent; }
    #ConfigureDialog QToolButton:focus { border: 2px solid @accent; }
    #ConfigureDialog QDialogButtonBox QPushButton { background: transparent; border: none;
        border-radius: 6px; font-size: 16px; padding: 4px 10px; }
    #ConfigureDialog QDialogButtonBox QPushButton:hover { background: @hover; }
    #ConfigureDialog QDialogButtonBox QPushButton:focus { border: 2px solid @accent; }
)";

// The menu bar and status bar, set on those widgets alone; everything else takes its colours from
// the palette. An application-wide style sheet would put every widget on Qt's slower style sheet
// path, which made windows several times slower to build.
constexpr const char* ChromeRules = R"(
    QMenuBar { background: @bg; color: @text; border-bottom: 1px solid @muted; }
    QMenuBar::item { background: transparent; padding: 4px 10px; border-radius: 4px; }
    QMenuBar::item:selected { background: @hover; }
    QMenu { background: @field; color: @text; border: 1px solid @border; padding: 4px; }
    QMenu::item { padding: 5px 28px 5px 24px; border-radius: 4px; }
    QMenu::item:selected { background: @accent; color: #ffffff; }
    QMenu::item:disabled { color: @secondary; }
    QMenu::separator { height: 1px; background: @border; margin: 4px 8px; }
    QStatusBar { background: @bg; color: @secondary; border-top: 1px solid @muted; }
    QStatusBar::item { border: none; }
    QStatusBar QLabel { color: @secondary; }
    QStatusBar QPushButton, QStatusBar QToolButton { background: transparent; border: none;
        border-radius: 4px; padding: 0 6px; color: @text; }
    QStatusBar QPushButton:hover, QStatusBar QToolButton:hover { background: @hover; }
    QStatusBar QPushButton:checked { color: @accent; }
)";

QString Fill(QString sheet, const HomeTheme& theme, const QString& scope) {
    const DerivedColors derived = Derive(theme);
    sheet.replace(QStringLiteral("@arrowdown"), ArrowImage(theme.text, false));
    sheet.replace(QStringLiteral("@arrowup"), ArrowImage(theme.text, true));
    // Longer tokens first: "@secondary" and "@selrow" both start like the scope marker "@s".
    const std::pair<const char*, QColor> tokens[] = {
        {"@secondary", derived.secondary}, {"@selrow", derived.selected_row},
        {"@accent", theme.accent},         {"@border", derived.border},
        {"@field", derived.field},         {"@hover", derived.hover},
        {"@muted", theme.muted},           {"@text", theme.text},
        {"@bg", theme.background},
    };
    for (const auto& [token, color] : tokens) {
        sheet.replace(QLatin1String(token), color.name());
    }
    sheet.replace(QStringLiteral("@s"), scope);
    return sheet;
}
} // namespace

QString HomeTheme::SettingsStyleSheet() const {
    return Fill(QString::fromLatin1(SettingsRules) + QString::fromLatin1(ControlRules), *this,
                QStringLiteral("#ConfigureDialog "));
}

QString HomeTheme::ChromeStyleSheet() const {
    return Fill(QString::fromLatin1(ChromeRules), *this, {});
}

QPalette HomeTheme::ApplicationPalette() const {
    const DerivedColors derived = Derive(*this);
    QPalette palette;
    palette.setColor(QPalette::Window, background);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, derived.field);
    palette.setColor(QPalette::AlternateBase, derived.hover);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, derived.field);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Highlight, accent);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::ToolTipBase, derived.field);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::PlaceholderText, derived.secondary);
    palette.setColor(QPalette::Link, accent);
    palette.setColor(QPalette::LinkVisited, focus_dark);
    palette.setColor(QPalette::Light, MixColors(derived.field, Qt::white, 0.2));
    palette.setColor(QPalette::Midlight, MixColors(derived.field, derived.border, 0.5));
    palette.setColor(QPalette::Mid, derived.border);
    palette.setColor(QPalette::Dark, MixColors(derived.border, Qt::black, 0.3));
    palette.setColor(QPalette::Shadow, Qt::black);
    for (const auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        palette.setColor(QPalette::Disabled, role, derived.secondary);
    }
    return palette;
}

bool HomeTheme::SystemPrefersDark() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    return false;
#endif
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
