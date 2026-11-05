// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>

#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QVariantAnimation>

#include "qt_common/game_list/game_list_p.h"
#include "qt_common/game_list/model.h"
#include "yuzu/game/carousel.h"
#include "yuzu/game/common.h"
#include "yuzu/game/home_theme.h"

namespace {
// On the Switch a 276px row holds 256px tiles; the rest is room for the selection frame.
constexpr double TileShare = 256.0 / 276.0;
// Cells are this much wider than their tile. The frame must stay inside its own cell, since the
// view only repaints that rectangle when the selection moves.
constexpr double GapShare = 14.0 / 256.0;

QRect TileRect(const QRect& cell, int tile) {
    return QRect(cell.left() + (cell.width() - tile) / 2, cell.top() + (cell.height() - tile) / 2,
                 tile, tile);
}

// A tile without art shows its game's name; a dump with no usable name falls back to its folder.
QString TileTitle(const QModelIndex& index) {
    const QString title = index.data(Qt::DisplayRole).toString().section(QLatin1Char('\n'), 0, 0);
    if (!title.trimmed().isEmpty()) {
        return title;
    }
    const QFileInfo file(index.data(GameListItemPath::FullPathRole).toString());
    return file.fileName() == QStringLiteral("main") ? file.dir().dirName() : file.completeBaseName();
}

class SoftwareTile : public QStyledItemDelegate {
public:
    SoftwareTile(const QVariantAnimation* pulse_, QObject* parent)
        : QStyledItemDelegate{parent}, pulse{pulse_} {}

    void SetSizes(int tile, const QSize& cell) {
        tile_size = tile;
        cell_size = cell;
    }

    void SetColors(const QColor& tile, const QColor& text, const QColor& dark, const QColor& light) {
        tile_color = tile;
        text_color = text;
        frame_dark = dark;
        frame_light = light;
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        const QRect tile = TileRect(option.rect, tile_size);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setRenderHint(QPainter::SmoothPixmapTransform);

        const QPixmap art = index.data(GameListItemPath::ArtworkRole).value<QPixmap>();
        if (art.isNull()) {
            painter->fillRect(tile, tile_color);
            QFont font = option.font;
            font.setPixelSize(std::max(11, tile_size / 12));
            painter->setFont(font);
            painter->setPen(text_color);
            painter->drawText(tile.adjusted(tile_size / 10, 0, -tile_size / 10, 0),
                              Qt::AlignCenter | Qt::TextWordWrap, TileTitle(index));
        } else {
            const int side = std::min(art.width(), art.height());
            painter->drawPixmap(tile, art,
                                QRect((art.width() - side) / 2, (art.height() - side) / 2, side, side));
        }

        // As on the Switch, only the focused row shows its selection.
        const bool focused = option.widget && option.widget->window()->focusWidget() == option.widget;
        if ((option.state & QStyle::State_Selected) && focused) {
            // The view repaints only this cell when the selection moves, so anything drawn past it
            // stays behind on the neighbouring tile. Keep the whole stroke, including its
            // antialiased edge, a pixel inside the cell.
            const int margin = std::min(option.rect.width() - tile_size,
                                        option.rect.height() - tile_size) / 2;
            const qreal width = std::clamp(tile_size * 0.017, 2.0, std::max(2.0, margin * 0.45));
            const qreal offset =
                std::min(tile_size * 0.012 + width / 2, std::max(width / 2, margin - width / 2 - 1.0));
            const qreal value = pulse->currentValue().toReal();
            painter->setPen(QPen(MixColors(frame_dark, frame_light, value), width));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(QRectF(tile).adjusted(-offset, -offset, offset, offset), 3, 3);
        }
        painter->restore();
    }

    // The item fills its whole cell so the frame around the tile is inside the item's rectangle.
    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override {
        return cell_size;
    }

private:
    const QVariantAnimation* pulse;
    int tile_size = 256;
    QSize cell_size{270, 276};
    QColor tile_color{0x4b, 0x4b, 0x4b};
    QColor text_color{Qt::white};
    QColor frame_dark{0x07, 0x89, 0xa8};
    QColor frame_light{0x2d, 0xf2, 0xff};
};

SoftwareTile* AsTile(QStyledItemDelegate* delegate) {
    return static_cast<SoftwareTile*>(delegate);
}
} // namespace

GameCarousel::GameCarousel(QWidget* parent) : QListView{parent} {
    m_pulse = new QVariantAnimation(this);
    m_pulse->setStartValue(0.0);
    m_pulse->setKeyValueAt(0.5, 1.0);
    m_pulse->setEndValue(0.0);
    m_pulse->setDuration(1600);
    m_pulse->setLoopCount(-1);
    m_pulse->setEasingCurve(QEasingCurve::InOutSine);
    connect(m_pulse, &QVariantAnimation::valueChanged, this,
            [this] { viewport()->update(visualRect(currentIndex())); });

    m_tile = new SoftwareTile(m_pulse, this);
    setItemDelegate(m_tile);

    setViewMode(QListView::IconMode);
    setMovement(QListView::Static);
    setUniformItemSizes(true);
    setSelectionMode(QAbstractItemView::SingleSelection);

    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setContextMenuPolicy(Qt::CustomContextMenu);

    setFrameShape(QFrame::NoFrame);
    setFlow(QListView::LeftToRight);
    setWrapping(false);
    // The home screen paints the background, including any theme image.
    setObjectName(QStringLiteral("SoftwareRow"));
    setStyleSheet(QStringLiteral("QListView#SoftwareRow, QListView#SoftwareRow > QWidget"
                                 " { background: transparent; border: none; }"));
}

void GameCarousel::SetModel(GameListModel* model) {
    QListView::setModel(model);
    UpdateIconSize();
}

void GameCarousel::ApplyFilter(const QString& edit_filter_text, GameListModel* model) {
    int row_count = model->rowCount();

    for (int i = 0; i < row_count; ++i) {
        QStandardItem* item = model->item(i, 0);
        if (!item)
            continue;

        if (Yuzu::FilterMatches(edit_filter_text, item)) {
            setRowHidden(i, false);
        } else {
            setRowHidden(i, true);
        }
    }

    if (!currentIndex().isValid() || isRowHidden(currentIndex().row())) {
        setCurrentIndex(NextVisible(-1, 1));
    }
}

void GameCarousel::UpdateIconSize() {
    const int height = viewport()->height();
    const int tile = std::max(32, static_cast<int>(height * TileShare));
    const QSize grid_size(tile + std::max(8, static_cast<int>(tile * GapShare)), height);
    if (gridSize() != grid_size || m_tileSize != tile) {
        m_tileSize = tile;
        AsTile(m_tile)->SetSizes(tile, grid_size);
        setGridSize(grid_size);
    }
}

void GameCarousel::SetColors(const QColor& tile, const QColor& text, const QColor& frame_dark,
                             const QColor& frame_light) {
    AsTile(m_tile)->SetColors(tile, text, frame_dark, frame_light);
    viewport()->update();
}

void GameCarousel::SelectFirst() {
    const QModelIndex first = NextVisible(-1, 1);
    if (first.isValid()) {
        setCurrentIndex(first);
    }
}

void GameCarousel::SelectFirstIfNone() {
    if (!currentIndex().isValid()) {
        SelectFirst();
    }
}

QModelIndex GameCarousel::indexAt(const QPoint& point) const {
    QModelIndex index = QListView::indexAt(point);
    if (!index.isValid())
        return {};
    if (!TileRect(visualRect(index), m_tileSize).contains(point))
        return {};
    return index;
}

void GameCarousel::focusInEvent(QFocusEvent* event) {
    QListView::focusInEvent(event);
    m_pulse->start();
}

void GameCarousel::focusOutEvent(QFocusEvent* event) {
    QListView::focusOutEvent(event);
    m_pulse->stop();
    viewport()->update();
}

void GameCarousel::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Up:
        emit TopEdgeReached();
        return;
    case Qt::Key_Down:
        emit BottomEdgeReached();
        return;
    default:
        QListView::keyPressEvent(event);
    }
}

QModelIndex GameCarousel::NextVisible(int from_row, int direction) const {
    if (!model()) {
        return {};
    }
    for (int row = from_row + direction; row >= 0 && row < model()->rowCount(); row += direction) {
        if (!isRowHidden(row)) {
            return model()->index(row, 0);
        }
    }
    return {};
}
