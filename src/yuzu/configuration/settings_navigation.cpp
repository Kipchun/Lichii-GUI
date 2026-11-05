// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QTabBar>
#include <QTabWidget>

#include "yuzu/configuration/settings_navigation.h"

SettingsNavigation::SettingsNavigation(QDialog* dialog_, QListWidget* categories_, QTabWidget* tabs_)
    : QObject{dialog_}, dialog{dialog_}, categories{categories_}, tabs{tabs_} {
    // Application-wide so rows are handled before combo boxes, or the dialog's own Enter and
    // Escape handling, see the key.
    qApp->installEventFilter(this);
}

SettingsNavigation::~SettingsNavigation() {
    qApp->removeEventFilter(this);
}

void SettingsNavigation::FocusCategories() {
    categories->setFocus(Qt::OtherFocusReason);
}

bool SettingsNavigation::eventFilter(QObject* object, QEvent* event) {
    if (event->type() != QEvent::KeyPress || !dialog->isVisible()) {
        return false;
    }
    // Only the focus widget's own delivery; a key it ignores then travels up its parents.
    auto* focus = qobject_cast<QWidget*>(object);
    if (!focus || focus != QApplication::focusWidget() ||
        (focus != dialog && !dialog->isAncestorOf(focus))) {
        return false;
    }
    const auto* key_event = static_cast<QKeyEvent*>(event);
    if (key_event->modifiers() & ~Qt::KeyboardModifiers(Qt::KeypadModifier)) {
        return false;
    }
    return HandleKey(focus, key_event->key());
}

void SettingsNavigation::FocusRow(int direction, QWidget* from) {
    QWidget* page = tabs->currentWidget();
    if (!page) {
        return;
    }
    const auto is_row = [page](QWidget* widget) {
        return page->isAncestorOf(widget) && widget->isVisible() && widget->isEnabled() &&
               (widget->focusPolicy() & Qt::TabFocus) && !widget->focusProxy() &&
               !qobject_cast<QTabBar*>(widget) && !qobject_cast<QScrollBar*>(widget);
    };
    // Rows follow the page's tab order.
    QList<QWidget*> rows;
    QWidget* widget = page;
    for (int guard = 0; guard < 4096; ++guard) {
        widget = widget->nextInFocusChain();
        if (widget == page) {
            break;
        }
        if (is_row(widget)) {
            rows.append(widget);
        }
    }
    if (rows.isEmpty()) {
        return;
    }
    const qsizetype index = from ? rows.indexOf(from) : -1;
    QWidget* next = index < 0 ? rows.first()
                              : rows[std::clamp<qsizetype>(index + direction, 0, rows.size() - 1)];
    next->setFocus(Qt::TabFocusReason);
    for (QWidget* parent = next->parentWidget(); parent; parent = parent->parentWidget()) {
        if (auto* area = qobject_cast<QScrollArea*>(parent)) {
            area->ensureWidgetVisible(next);
            break;
        }
    }
}

bool SettingsNavigation::HandleKey(QWidget* focus, int key) {
    QWidget* page = tabs->currentWidget();
    const bool in_list = focus == categories;
    const bool in_page = page && page->isAncestorOf(focus);
    // Spin boxes hand focus to their inner line edit.
    auto* spin = qobject_cast<QAbstractSpinBox*>(focus);
    if (!spin && focus->parentWidget()) {
        spin = qobject_cast<QAbstractSpinBox*>(focus->parentWidget());
    }
    auto* line_edit = spin ? nullptr : qobject_cast<QLineEdit*>(focus);
    auto* view = in_list ? nullptr : qobject_cast<QAbstractItemView*>(focus);

    switch (key) {
    case Qt::Key_PageUp:
    case Qt::Key_PageDown: {
        const int index = tabs->currentIndex() + (key == Qt::Key_PageUp ? -1 : 1);
        if (index >= 0 && index < tabs->count()) {
            tabs->setCurrentIndex(index);
            if (in_page) {
                FocusRow(1, nullptr);
            }
        }
        return true;
    }
    case Qt::Key_Escape:
        if (in_list) {
            dialog->accept();
        } else {
            FocusCategories();
        }
        return true;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (in_list) {
            FocusRow(1, nullptr);
        } else if (auto* button = qobject_cast<QAbstractButton*>(focus)) {
            // Deferred: the click may open a dialog with its own event loop.
            QMetaObject::invokeMethod(button, &QAbstractButton::click, Qt::QueuedConnection);
        } else if (auto* combo = qobject_cast<QComboBox*>(focus)) {
            combo->showPopup();
        }
        // Anything else would fall through to the dialog's default button.
        return true;
    case Qt::Key_Up:
    case Qt::Key_Down: {
        if (in_list) {
            return false;
        }
        const int direction = key == Qt::Key_Up ? -1 : 1;
        if (view && view->model()) {
            // Lists move through their own rows and hand off only at either end.
            const int row = view->currentIndex().row();
            const int last = view->model()->rowCount(view->currentIndex().parent()) - 1;
            if (row >= 0 && ((direction < 0 && row > 0) || (direction > 0 && row < last))) {
                return false;
            }
        }
        if (in_page) {
            FocusRow(direction, focus);
        } else {
            FocusCategories();
        }
        return true;
    }
    case Qt::Key_Left:
    case Qt::Key_Right: {
        const bool right = key == Qt::Key_Right;
        if (in_list) {
            if (right) {
                FocusRow(1, nullptr);
            }
            return true;
        }
        if (qobject_cast<QSlider*>(focus) || line_edit) {
            return false;
        }
        if (spin) {
            spin->stepBy(right ? 1 : -1);
            return true;
        }
        if (!right) {
            FocusCategories();
        }
        return true;
    }
    default:
        return false;
    }
}
