// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class QDialog;
class QListWidget;
class QTabWidget;
class QWidget;

/// Switch-style keyboard and controller navigation for a settings dialog: the category list on
/// the left, the current tab's rows on the right, L/R (Page Up/Down) for tabs and B (Escape) to
/// step back out, which leaves the dialog from the category list.
class SettingsNavigation : public QObject {
public:
    SettingsNavigation(QDialog* dialog, QListWidget* categories, QTabWidget* tabs);
    ~SettingsNavigation() override;

    void FocusCategories();

protected:
    bool eventFilter(QObject* object, QEvent* event) override;

private:
    bool HandleKey(QWidget* focus, int key);
    void FocusRow(int direction, QWidget* from);

    QDialog* dialog;
    QListWidget* categories;
    QTabWidget* tabs;
};
