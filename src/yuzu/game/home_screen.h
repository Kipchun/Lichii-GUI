// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QPixmap>
#include <QString>
#include <QWidget>

#include "yuzu/game/home_theme.h"

class QAbstractButton;
class QAction;
class QLabel;
class QMenu;
class QTimer;
class QToolButton;
class QVariantAnimation;
class QVBoxLayout;

// Switch HOME Menu layout shared by the library and its empty state. The grid and list views
// keep the regular desktop look, so the chrome only shows on the home page.
class HomeScreen : public QWidget {
    Q_OBJECT

public:
    enum class Page { Home, Games, Software };

    struct Actions {
        QAction* handbook;
        QAction* install;
        QAction* album;
        QAction* settings;
        QAction* exit;
    };

    explicit HomeScreen(QWidget* parent = nullptr);

    void BindActions(const Actions& actions);
    void SetPage(Page page);
    /// The first avatar belongs to the active profile.
    void SetProfiles(const QList<QPixmap>& avatars);
    void SetConnectedPlayers(int count);
    /// choice is the appearance setting, shown checked in the context menu; dark is what it
    /// resolved to.
    void SetTheme(const QString& choice, bool dark);
    /// An empty or unreadable path uses the theme's plain background.
    void SetWallpaper(const QString& path);
    void AddAppearanceActions(QMenu& menu);

    void FocusProfiles();
    void FocusShortcuts();
    /// Whether controller input should go to this widget instead of the library view.
    bool IsNavigationTarget(const QWidget* widget) const;
    const HomeTheme& Theme() const;

signals:
    void ControllersRequested();
    void ProfilesRequested();
    void ThemeSelected(const QString& theme_id);
    void WallpaperSelected(const QString& path);
    void OpenRequested();
    void OptionsRequested();
    void ThemeChanged(const HomeTheme& theme);

protected:
    QVBoxLayout* ContentLayout() const;
    /// The widget that takes focus when navigation returns to the software row.
    void SetContentFocusWidget(QWidget* widget);
    void SetSelectedTitle(const QString& title);
    void SetLibraryHintsVisible(bool visible);

    void changeEvent(QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    class CircleButton;
    class PanelKeyFilter;

    bool HandlePanelKey(QAbstractButton* button, int key);
    void FocusContent();
    void ApplyTheme();
    void Relayout();
    void RetranslateHome();
    void UpdateClock();
    void UpdateFocusDecoration();

    QWidget* content_host = nullptr;
    QVBoxLayout* content_layout = nullptr;
    QWidget* content_focus = nullptr;
    PanelKeyFilter* key_filter = nullptr;

    QList<CircleButton*> profiles;
    QList<CircleButton*> shortcuts;
    QLabel* title = nullptr;
    QLabel* clock = nullptr;
    QLabel* caption = nullptr;
    QToolButton* controller_button = nullptr;
    QToolButton* options_hint = nullptr;
    QToolButton* open_hint = nullptr;
    QTimer* clock_timer = nullptr;
    QVariantAnimation* pulse = nullptr;

    Page page = Page::Home;
    QString theme_id;
    bool dark_mode = true;
    HomeTheme theme;
    QPixmap background;
    QString selected_title;
    int connected_players = 0;
    int footer_line_y = 0;
    bool library_hints = true;
};
