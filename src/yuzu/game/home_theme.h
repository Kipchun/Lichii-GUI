// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QIcon>
#include <QPalette>
#include <QPixmap>
#include <QString>

/// Colours and background of the HOME Menu look; the unit a theme file would replace.
struct HomeTheme {
    QColor background;
    QColor text;
    QColor muted;
    QColor button;
    QColor accent;
    QColor focus_light;
    QColor focus_dark;
    QString background_image;

    static HomeTheme BasicBlack();
    static HomeTheme BasicWhite();
    /// Accepts "basic_black" or "basic_white"; anything else follows the system colour scheme.
    static HomeTheme FromId(const QString& id);
    static bool SystemPrefersDark();

    /// Restyles a settings dialog and its pages in the System Settings look.
    QString SettingsStyleSheet() const;
    /// For the main window's menu bar and status bar only; the rest of the application follows
    /// ApplicationPalette under the Fusion style.
    QString ChromeStyleSheet() const;
    QPalette ApplicationPalette() const;
};

enum class HomeSymbol { Gamepad, Settings, Handbook, Bag, Album, Power };

QColor MixColors(const QColor& from, const QColor& to, qreal amount);
QPixmap DrawHomeSymbol(HomeSymbol symbol, const QColor& color, int size);
/// A controller button glyph, such as the A in "A Start".
QIcon HomeButtonGlyph(const QString& glyph, const QColor& circle, const QColor& mark);
