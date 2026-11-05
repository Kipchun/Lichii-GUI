// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QListView>
#include <QString>

class GameListModel;
class QStyledItemDelegate;
class QVariantAnimation;

/// The HOME Menu software row: square tiles, with a pulsing frame on the selection while focused.
class GameCarousel : public QListView {
    Q_OBJECT

public:
    explicit GameCarousel(QWidget* parent = nullptr);

    void SetModel(GameListModel* model);
    void ApplyFilter(const QString& edit_filter_text, GameListModel* model);
    void UpdateIconSize();
    void SelectFirst();
    void SelectFirstIfNone();
    /// Placeholder tile and selection frame colours, from the home screen theme.
    void SetColors(const QColor& tile, const QColor& text, const QColor& frame_dark,
                   const QColor& frame_light);

    QModelIndex indexAt(const QPoint& point) const override;

signals:
    void TopEdgeReached();
    void BottomEdgeReached();

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QModelIndex NextVisible(int from_row, int direction) const;

    QStyledItemDelegate* m_tile = nullptr;
    QVariantAnimation* m_pulse = nullptr;
    int m_tileSize = 256;
};
