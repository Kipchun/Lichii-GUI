// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>

#include <QAbstractButton>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QFileDialog>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTimer>
#include <QToolButton>
#include <QVariantAnimation>
#include <QVBoxLayout>

#include "yuzu/game/home_screen.h"

namespace {
// Geometry is laid out on the Switch's 1280x720 HOME Menu and scaled to the widget.
constexpr qreal DesignWidth = 1280.0;
constexpr qreal DesignHeight = 720.0;

QIcon ControllerIcon(int connected, const QColor& color) {
    constexpr int Width = 128;
    constexpr int Height = 104;
    QPixmap pixmap(Width, Height);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    for (int i = 0; i < 4; ++i) {
        QColor led = color;
        led.setAlphaF(i < connected ? 1.0 : 0.25);
        p.setBrush(i < connected ? QColor("#4cd964") : led);
        p.drawRect(QRectF(14 + i * 26, 0, 16, 12));
    }
    p.drawPixmap(QRectF(8, 14, 112, 90), DrawHomeSymbol(HomeSymbol::Gamepad, color, 56),
                 QRectF(0, 0, 112, 112));
    return QIcon(pixmap);
}
} // namespace

// Round HOME Menu button: a shortcut icon on a disc, or a profile picture.
class HomeScreen::CircleButton : public QAbstractButton {
public:
    CircleButton(const HomeScreen* screen_, QWidget* parent)
        : QAbstractButton(parent), screen{screen_} {
        setFocusPolicy(Qt::TabFocus);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);
    }

    void SetSymbol(HomeSymbol symbol_, const QColor& fixed_color_) {
        symbol = symbol_;
        fixed_color = fixed_color_;
    }

    void SetAvatar(const QPixmap& avatar_) {
        avatar = avatar_;
    }

protected:
    void paintEvent(QPaintEvent*) override {
        const HomeTheme& colors = screen->theme;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        // The outer band of the widget is reserved for the focus ring.
        const qreal ring = width() * 0.075;
        const QRectF disc = QRectF(rect()).adjusted(ring, ring, -ring, -ring);
        if (!isEnabled()) {
            p.setOpacity(0.35);
        }

        if (!avatar.isNull()) {
            QPainterPath clip;
            clip.addEllipse(disc);
            p.save();
            p.setClipPath(clip);
            p.drawPixmap(disc.toRect(), avatar);
            p.restore();
        } else {
            QColor fill = colors.button;
            if (underMouse() && isEnabled()) {
                fill = fill.lightness() > 128 ? fill.darker(106) : fill.lighter(122);
            }
            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            p.drawEllipse(disc);
            const int icon_size = qRound(disc.width() * 0.5);
            const QColor color = fixed_color.isValid() ? fixed_color : colors.text;
            if (icon.isNull() || icon_size != icon_cache_size || color != icon_cache_color) {
                icon = DrawHomeSymbol(symbol, color, icon_size);
                icon_cache_size = icon_size;
                icon_cache_color = color;
            }
            p.drawPixmap(QPointF(disc.center().x() - icon_size / 2.0,
                                 disc.center().y() - icon_size / 2.0),
                         icon);
        }

        if (hasFocus()) {
            p.setOpacity(1.0);
            const qreal value = screen->pulse->currentValue().toReal();
            p.setPen(QPen(MixColors(colors.focus_dark, colors.focus_light, value), ring * 0.55));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(disc.adjusted(-ring * 0.55, -ring * 0.55, ring * 0.55, ring * 0.55));
        }
    }

private:
    const HomeScreen* screen;
    HomeSymbol symbol = HomeSymbol::Gamepad;
    QColor fixed_color;
    QPixmap avatar;
    QPixmap icon;
    int icon_cache_size = 0;
    QColor icon_cache_color;
};

class HomeScreen::PanelKeyFilter : public QObject {
public:
    explicit PanelKeyFilter(HomeScreen* screen_) : QObject(screen_), screen{screen_} {}

    bool eventFilter(QObject* object, QEvent* event) override {
        if (event->type() != QEvent::KeyPress) {
            return false;
        }
        return screen->HandlePanelKey(static_cast<QAbstractButton*>(object),
                                      static_cast<QKeyEvent*>(event)->key());
    }

private:
    HomeScreen* screen;
};

HomeScreen::HomeScreen(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("HomeScreen"));

    pulse = new QVariantAnimation(this);
    pulse->setStartValue(0.0);
    pulse->setKeyValueAt(0.5, 1.0);
    pulse->setEndValue(0.0);
    pulse->setDuration(1600);
    pulse->setLoopCount(-1);
    pulse->setEasingCurve(QEasingCurve::InOutSine);
    connect(pulse, &QVariantAnimation::valueChanged, this, [this] {
        QWidget* focused = window()->focusWidget();
        if (IsNavigationTarget(focused)) {
            focused->update();
        }
    });

    content_host = new QWidget(this);
    content_host->setObjectName(QStringLiteral("HomeContent"));
    content_layout = new QVBoxLayout(content_host);
    content_layout->setContentsMargins(0, 0, 0, 0);
    content_layout->setSpacing(0);

    title = new QLabel(this);
    title->setObjectName(QStringLiteral("HomeTitle"));
    clock = new QLabel(this);
    clock->setObjectName(QStringLiteral("HomeClock"));
    clock->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    caption = new QLabel(this);
    caption->setObjectName(QStringLiteral("HomeCaption"));
    caption->setAlignment(Qt::AlignCenter);
    caption->hide();

    key_filter = new PanelKeyFilter(this);

    struct Shortcut {
        HomeSymbol symbol;
        const char* color;
    };
    static constexpr std::array shortcut_symbols{
        Shortcut{HomeSymbol::Handbook, "#ff4b4b"}, Shortcut{HomeSymbol::Bag, "#ffb400"},
        Shortcut{HomeSymbol::Album, "#2aa7ff"},    Shortcut{HomeSymbol::Gamepad, nullptr},
        Shortcut{HomeSymbol::Settings, nullptr},   Shortcut{HomeSymbol::Power, nullptr},
    };
    for (const Shortcut& shortcut : shortcut_symbols) {
        auto* button = new CircleButton(this, this);
        button->setObjectName(QStringLiteral("HomeShortcut"));
        button->SetSymbol(shortcut.symbol, shortcut.color ? QColor(shortcut.color) : QColor());
        button->installEventFilter(key_filter);
        shortcuts.append(button);
    }
    connect(shortcuts[3], &QAbstractButton::clicked, this, &HomeScreen::ControllersRequested);

    const auto make_hint = [this] {
        auto* button = new QToolButton(this);
        button->setObjectName(QStringLiteral("HomeHint"));
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setFocusPolicy(Qt::NoFocus);
        button->setCursor(Qt::PointingHandCursor);
        return button;
    };
    controller_button = make_hint();
    controller_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    options_hint = make_hint();
    open_hint = make_hint();
    connect(controller_button, &QToolButton::clicked, this, &HomeScreen::ControllersRequested);
    connect(options_hint, &QToolButton::clicked, this, &HomeScreen::OptionsRequested);
    connect(open_hint, &QToolButton::clicked, this, &HomeScreen::OpenRequested);

    clock_timer = new QTimer(this);
    clock_timer->setInterval(5000);
    connect(clock_timer, &QTimer::timeout, this, &HomeScreen::UpdateClock);

    connect(qApp, &QApplication::focusChanged, this, &HomeScreen::UpdateFocusDecoration);

    ApplyTheme();
    RetranslateHome();
}

void HomeScreen::RetranslateHome() {
    const std::array names{tr("Handbook"),    tr("Install Files"),   tr("Album"),
                           tr("Controllers"), tr("System Settings"), tr("Exit")};
    for (std::size_t i = 0; i < names.size(); ++i) {
        shortcuts[static_cast<int>(i)]->setText(names[i]);
        shortcuts[static_cast<int>(i)]->setToolTip(names[i]);
    }
    options_hint->setText(tr("Options"));
    open_hint->setText(tr("Start"));
    for (CircleButton* profile : std::as_const(profiles)) {
        profile->setToolTip(tr("Profiles"));
    }
    SetConnectedPlayers(connected_players);
    UpdateClock();
    UpdateFocusDecoration();
    Relayout();
}

void HomeScreen::BindActions(const Actions& actions) {
    const auto bind = [this](QAbstractButton* button, QAction* action) {
        connect(button, &QAbstractButton::clicked, action, &QAction::trigger);
        const auto sync = [button, action] { button->setEnabled(action->isEnabled()); };
        connect(action, &QAction::changed, this, sync);
        sync();
    };
    bind(shortcuts[0], actions.handbook);
    bind(shortcuts[1], actions.install);
    bind(shortcuts[2], actions.album);
    bind(shortcuts[4], actions.settings);
    bind(shortcuts[5], actions.exit);
}

void HomeScreen::SetPage(Page page_) {
    page = page_;
    const bool home = page == Page::Home;
    for (QWidget* widget : std::initializer_list<QWidget*>{clock, controller_button}) {
        widget->setVisible(home);
    }
    options_hint->setVisible(home && library_hints);
    open_hint->setVisible(home && library_hints);
    for (CircleButton* button : std::as_const(shortcuts)) {
        button->setVisible(home);
    }
    for (CircleButton* profile : std::as_const(profiles)) {
        profile->setVisible(home);
    }
    ApplyTheme();
    Relayout();
    UpdateFocusDecoration();
}

void HomeScreen::SetProfiles(const QList<QPixmap>& avatars) {
    const bool had_focus = IsNavigationTarget(window()->focusWidget()) &&
                           profiles.contains(window()->focusWidget());
    // Deferred: this can run from a slot connected to one of these buttons.
    for (CircleButton* profile : std::as_const(profiles)) {
        profile->hide();
        profile->deleteLater();
    }
    profiles.clear();

    constexpr qsizetype MaxShownProfiles = 8;
    for (qsizetype i = 0; i < std::min(avatars.size(), MaxShownProfiles); ++i) {
        auto* profile = new CircleButton(this, this);
        profile->setObjectName(QStringLiteral("HomeProfile"));
        profile->SetAvatar(avatars[i]);
        profile->setToolTip(tr("Profiles"));
        profile->installEventFilter(key_filter);
        profile->setVisible(page == Page::Home);
        connect(profile, &QAbstractButton::clicked, this, &HomeScreen::ProfilesRequested);
        profiles.append(profile);
    }
    Relayout();
    if (had_focus) {
        FocusProfiles();
    }
}

void HomeScreen::SetConnectedPlayers(int count) {
    connected_players = count;
    controller_button->setIcon(ControllerIcon(count, theme.text));
    controller_button->setToolTip(tr("%n controller(s) connected", "", count));
}

void HomeScreen::SetTheme(const QString& choice, bool dark) {
    theme_id = choice;
    dark_mode = dark;
    ApplyTheme();
}

void HomeScreen::SetWallpaper(const QString& path) {
    theme.background_image = path;
    ApplyTheme();
}

void HomeScreen::AddAppearanceActions(QMenu& menu) {
    auto* group = new QActionGroup(&menu);
    const std::array themes{
        std::pair{QString{}, tr("Match System")},
        std::pair{QStringLiteral("basic_white"), tr("Light")},
        std::pair{QStringLiteral("basic_black"), tr("Dark")},
        std::pair{QStringLiteral("classic"), tr("Classic Desktop Theme")},
    };
    for (const auto& [id, name] : themes) {
        QAction* action = menu.addAction(name, this, [this, id] { emit ThemeSelected(id); });
        action->setCheckable(true);
        action->setChecked(theme_id == id);
        group->addAction(action);
    }
    menu.addSeparator();
    menu.addAction(tr("Choose Background Image..."), this, [this] {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Choose Background Image"), {}, tr("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
        if (!path.isEmpty()) {
            emit WallpaperSelected(path);
        }
    });
    QAction* reset = menu.addAction(tr("Remove Background Image"), this,
                                    [this] { emit WallpaperSelected({}); });
    reset->setEnabled(!theme.background_image.isEmpty());
}

void HomeScreen::FocusProfiles() {
    if (!profiles.isEmpty() && page == Page::Home) {
        profiles.first()->setFocus(Qt::OtherFocusReason);
    }
}

void HomeScreen::FocusShortcuts() {
    if (page != Page::Home) {
        return;
    }
    for (CircleButton* button : std::as_const(shortcuts)) {
        if (button->isEnabled()) {
            button->setFocus(Qt::OtherFocusReason);
            return;
        }
    }
}

bool HomeScreen::IsNavigationTarget(const QWidget* widget) const {
    return widget && (std::find(shortcuts.begin(), shortcuts.end(), widget) != shortcuts.end() ||
                      std::find(profiles.begin(), profiles.end(), widget) != profiles.end());
}

const HomeTheme& HomeScreen::Theme() const {
    return theme;
}

QVBoxLayout* HomeScreen::ContentLayout() const {
    return content_layout;
}

void HomeScreen::SetContentFocusWidget(QWidget* widget) {
    content_focus = widget;
}

void HomeScreen::SetSelectedTitle(const QString& title_) {
    selected_title = title_;
    title->setText(title_);
    UpdateFocusDecoration();
}

void HomeScreen::SetLibraryHintsVisible(bool visible) {
    library_hints = visible;
    options_hint->setVisible(page == Page::Home && visible);
    open_hint->setVisible(page == Page::Home && visible);
}

bool HomeScreen::HandlePanelKey(QAbstractButton* button, int key) {
    // Skips buttons whose action is unavailable; returns false at the end of the row.
    const auto focus_next = [](const QList<CircleButton*>& row, qsizetype from, int direction) {
        for (qsizetype i = from + direction; i >= 0 && i < row.size(); i += direction) {
            if (row[i]->isEnabled()) {
                row[i]->setFocus(Qt::OtherFocusReason);
                return true;
            }
        }
        return false;
    };

    switch (key) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        // Deferred because the click can open a modal dialog or replace this button.
        QMetaObject::invokeMethod(button, &QAbstractButton::click, Qt::QueuedConnection);
        return true;
    case Qt::Key_Escape:
        FocusContent();
        return true;
    default:
        break;
    }

    const auto* circle = static_cast<CircleButton*>(button);
    if (const qsizetype index = profiles.indexOf(circle); index >= 0) {
        switch (key) {
        case Qt::Key_Left:
            focus_next(profiles, index, -1);
            return true;
        case Qt::Key_Right:
            focus_next(profiles, index, 1);
            return true;
        case Qt::Key_Down:
            if (content_focus) {
                FocusContent();
            } else {
                FocusShortcuts();
            }
            return true;
        case Qt::Key_Up:
            return true;
        default:
            return false;
        }
    }

    const qsizetype index = shortcuts.indexOf(circle);
    switch (key) {
    case Qt::Key_Left:
        focus_next(shortcuts, index, -1);
        return true;
    case Qt::Key_Right:
        focus_next(shortcuts, index, 1);
        return true;
    case Qt::Key_Up:
        if (content_focus) {
            FocusContent();
        } else {
            FocusProfiles();
        }
        return true;
    case Qt::Key_Down:
        return true;
    default:
        return false;
    }
}

void HomeScreen::FocusContent() {
    if (content_focus) {
        content_focus->setFocus(Qt::OtherFocusReason);
    }
}

void HomeScreen::ApplyTheme() {
    const QString image = theme.background_image;
    theme = dark_mode ? HomeTheme::BasicBlack() : HomeTheme::BasicWhite();
    theme.background_image = image;
    background = {};

    const QString hover = theme.text.lightness() > 128 ? QStringLiteral("rgba(255, 255, 255, 26)")
                                                       : QStringLiteral("rgba(0, 0, 0, 20)");
    setStyleSheet(QStringLiteral("#HomeScreen QLabel#HomeTitle { color: %1; background: transparent; }"
                                 "#HomeScreen QLabel#HomeClock, #HomeScreen QLabel#HomeCaption,"
                                 "#HomeScreen QLabel#EmptyLibrary { color: %2; background: transparent; }"
                                 "#HomeScreen QLabel#EmptyLibrary { font-size: 20px; }"
                                 "#HomeScreen QWidget#HomeContent { background: transparent; }"
                                 "#HomeScreen QToolButton#HomeHint { color: %2; background: transparent;"
                                 " border: none; border-radius: 6px; padding: 2px 6px; }"
                                 "#HomeScreen QToolButton#HomeHint:hover { background: %3; }")
                      .arg(theme.accent.name(), theme.text.name(), hover));

    options_hint->setIcon(HomeButtonGlyph(QStringLiteral("+"), theme.text, theme.background));
    open_hint->setIcon(HomeButtonGlyph(QStringLiteral("A"), theme.text, theme.background));
    SetConnectedPlayers(connected_players);
    update();
    for (CircleButton* button : std::as_const(shortcuts)) {
        button->update();
    }
    emit ThemeChanged(theme);
}

void HomeScreen::Relayout() {
    if (page != Page::Home) {
        content_layout->setContentsMargins(0, 0, 0, 0);
        content_host->setGeometry(rect());
        return;
    }

    const qreal s = std::min(width() / DesignWidth, height() / DesignHeight);
    const int top = qRound((height() - DesignHeight * s) / 2);
    const auto S = [s](qreal value) { return qRound(value * s); };
    const auto set_font = [](QWidget* widget, int pixels) {
        QFont font = widget->font();
        font.setPixelSize(std::max(pixels, 9));
        widget->setFont(font);
    };

    int x = S(48);
    for (CircleButton* profile : std::as_const(profiles)) {
        profile->setGeometry(x, top + S(26), S(70), S(70));
        x += S(64);
    }

    set_font(clock, S(26));
    clock->setGeometry(width() - S(60) - S(320), top + S(39), S(320), S(44));

    set_font(title, S(24));
    title->setGeometry(S(98), top + S(136), width() - S(98), S(42));

    // The row runs to the right edge. The carousel derives its 256px tiles from this height
    // and centres each in a cell 14px wider, hence the 7px pulled back from the 98px margin.
    content_host->setGeometry(0, top + S(182), width(), S(276));
    content_layout->setContentsMargins(S(91), 0, 0, 0);

    const int disc = S(94);
    const int step = S(106);
    const int row_width = step * (static_cast<int>(shortcuts.size()) - 1) + disc;
    x = (width() - row_width) / 2;
    for (CircleButton* button : std::as_const(shortcuts)) {
        button->setGeometry(x, top + S(498), disc, disc);
        x += step;
    }
    set_font(caption, S(18));

    footer_line_y = top + S(646);
    controller_button->setIconSize(QSize(S(52), S(42)));
    controller_button->setGeometry(S(54), top + S(656), S(68), S(54));
    int right = width() - S(52);
    for (QToolButton* hint : {open_hint, options_hint}) {
        set_font(hint, S(20));
        hint->setIconSize(QSize(S(26), S(26)));
        const int hint_width = hint->sizeHint().width();
        right -= hint_width;
        hint->setGeometry(right, top + S(660), hint_width, S(46));
        right -= S(24);
    }
    UpdateFocusDecoration();
    update();
}

void HomeScreen::UpdateFocusDecoration() {
    QWidget* focused = window()->focusWidget();
    const bool home = page == Page::Home;
    title->setVisible(home && focused && focused == content_focus && !selected_title.isEmpty());

    const auto it = std::find(shortcuts.begin(), shortcuts.end(), focused);
    if (!home || it == shortcuts.end()) {
        caption->hide();
        return;
    }
    const QAbstractButton* button = *it;
    const int padding = caption->fontMetrics().height();
    const int caption_width = caption->fontMetrics().horizontalAdvance(button->text()) + padding;
    caption->setText(button->text());
    caption->setGeometry(button->geometry().center().x() - caption_width / 2,
                         button->geometry().bottom() + 2, caption_width,
                         caption->fontMetrics().height() + 4);
    caption->show();
}

void HomeScreen::UpdateClock() {
    clock->setText(QLocale::system().toString(QTime::currentTime(), QLocale::ShortFormat));
}

void HomeScreen::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateHome();
    }
    QWidget::changeEvent(event);
}

void HomeScreen::contextMenuEvent(QContextMenuEvent* event) {
    if (page != Page::Home) {
        QWidget::contextMenuEvent(event);
        return;
    }
    QMenu menu;
    AddAppearanceActions(menu);
    menu.exec(event->globalPos());
}

void HomeScreen::showEvent(QShowEvent* event) {
    UpdateClock();
    clock_timer->start();
    pulse->start();
    QWidget::showEvent(event);
}

void HomeScreen::hideEvent(QHideEvent* event) {
    clock_timer->stop();
    pulse->stop();
    QWidget::hideEvent(event);
}

void HomeScreen::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    background = {};
    Relayout();
}

void HomeScreen::paintEvent(QPaintEvent*) {
    if (page != Page::Home) {
        return;
    }
    QPainter p(this);
    p.fillRect(rect(), theme.background);

    if (!theme.background_image.isEmpty()) {
        const qreal ratio = devicePixelRatioF();
        if (background.isNull()) {
            const QImage image(theme.background_image);
            if (!image.isNull()) {
                background = QPixmap::fromImage(image.scaled(size() * ratio,
                                                             Qt::KeepAspectRatioByExpanding,
                                                             Qt::SmoothTransformation));
                background.setDevicePixelRatio(ratio);
            }
        }
        if (!background.isNull()) {
            const QSizeF shown = background.deviceIndependentSize();
            p.drawPixmap(QPointF((width() - shown.width()) / 2, (height() - shown.height()) / 2),
                         background);
        }
    }

    const qreal s = std::min(width() / DesignWidth, height() / DesignHeight);
    p.setPen(QPen(theme.muted, 1));
    p.drawLine(qRound(30 * s), footer_line_y, width() - qRound(30 * s), footer_line_y);
}
