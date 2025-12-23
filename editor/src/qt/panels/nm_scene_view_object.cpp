#include "NovelMind/editor/qt/panels/nm_scene_view_panel.hpp"
#include "NovelMind/editor/qt/nm_icon_manager.hpp"
#include "NovelMind/editor/qt/nm_style_manager.hpp"

#include <QColor>
#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QStyleOptionGraphicsItem>
#include <QTransform>
#include <QtMath>
#include <cmath>

namespace NovelMind::editor::qt {

// ============================================================================
// NMSceneObject
// ============================================================================

NMSceneObject::NMSceneObject(const QString &id, NMSceneObjectType type,
                             QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent), m_id(id), m_name(id), m_objectType(type) {
  setFlag(ItemIsMovable, true);
  setFlag(ItemIsSelectable, true);
  setFlag(ItemSendsGeometryChanges, true);
  setAcceptHoverEvents(true);

  // Create a fallback pixmap based on type with an icon
  QPixmap pixmap(200, 300);
  pixmap.fill(Qt::transparent);

  if (pixmap.isNull()) {
    setPixmap(pixmap);
    setTransformOriginPoint(boundingRect().center());
    return;
  }

  QPainter painter(&pixmap);
  if (!painter.isActive()) {
    setPixmap(pixmap);
    setTransformOriginPoint(boundingRect().center());
    return;
  }
  painter.setRenderHint(QPainter::Antialiasing);

  const auto &palette = NMStyleManager::instance().palette();

  // Get icon based on type
  QString iconName;
  QColor objectColor;
  QString typeName;

  switch (type) {
  case NMSceneObjectType::Background:
    iconName = "object-background";
    objectColor = QColor(60, 90, 120, 200);
    typeName = "Background";
    painter.fillRect(pixmap.rect(), objectColor);
    break;

  case NMSceneObjectType::Character:
    iconName = "object-character";
    objectColor = QColor(100, 150, 200, 200);
    typeName = "Character";
    painter.setBrush(objectColor);
    painter.setPen(QPen(palette.textPrimary, 2));
    painter.drawEllipse(50, 30, 100, 120); // Head
    painter.drawRect(70, 150, 60, 100);    // Body
    break;

  case NMSceneObjectType::UI:
    iconName = "object-ui";
    objectColor = QColor(120, 120, 150, 200);
    typeName = "UI Element";
    painter.fillRect(0, 0, 200, 100, objectColor);
    break;

  case NMSceneObjectType::Effect:
    iconName = "object-effect";
    objectColor = QColor(200, 120, 100, 200);
    typeName = "Effect";
    painter.setBrush(objectColor);
    painter.setPen(QPen(palette.textPrimary, 2));
    painter.drawEllipse(50, 50, 100, 100);
    break;
  }

  // Draw icon in top-left corner
  QPixmap icon = NMIconManager::instance().getPixmap(iconName, 32, palette.textPrimary);
  painter.drawPixmap(8, 8, icon);

  // Draw type text
  painter.setPen(QPen(palette.textPrimary, 2));
  QFont boldFont;
  boldFont.setBold(true);
  boldFont.setPointSize(10);
  painter.setFont(boldFont);
  painter.drawText(pixmap.rect().adjusted(0, 0, 0, -10),
                   Qt::AlignCenter | Qt::AlignBottom, typeName);

  setPixmap(pixmap);
  setTransformOriginPoint(boundingRect().center());
}

void NMSceneObject::setScaleX(qreal scale) {
  setScaleXY(scale, m_scaleY);
}

void NMSceneObject::setScaleY(qreal scale) {
  setScaleXY(m_scaleX, scale);
}

void NMSceneObject::setScaleXY(qreal scaleX, qreal scaleY) {
  m_scaleX = scaleX;
  m_scaleY = scaleY;
  setTransform(QTransform::fromScale(m_scaleX, m_scaleY));
}

void NMSceneObject::setUniformScale(qreal scale) {
  setScaleXY(scale, scale);
}

void NMSceneObject::setSelected(bool selected) {
  m_selected = selected;
  QGraphicsPixmapItem::setSelected(selected);
  update();
}

void NMSceneObject::setLocked(bool locked) {
  m_locked = locked;
  setFlag(ItemIsMovable, !locked);
  setAcceptedMouseButtons(Qt::AllButtons);
  update();
}

void NMSceneObject::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem *option,
                          QWidget *widget) {
  // Draw the pixmap
  QGraphicsPixmapItem::paint(painter, option, widget);

  // Draw selection outline
  if (m_selected || isSelected()) {
    const auto &palette = NMStyleManager::instance().palette();
    QColor fill = palette.accentPrimary;
    fill.setAlpha(40);
    painter->fillRect(boundingRect().adjusted(2, 2, -2, -2), fill);

    painter->setPen(QPen(palette.accentPrimary, 3, Qt::SolidLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(boundingRect().adjusted(1, 1, -1, -1));

    // Draw corner handles
    QRectF bounds = boundingRect();
    int handleSize = 8;
    painter->setBrush(palette.accentPrimary);
    painter->drawRect(static_cast<int>(bounds.left()),
                      static_cast<int>(bounds.top()), handleSize, handleSize);
    painter->drawRect(static_cast<int>(bounds.right() - handleSize),
                      static_cast<int>(bounds.top()), handleSize, handleSize);
    painter->drawRect(static_cast<int>(bounds.left()),
                      static_cast<int>(bounds.bottom() - handleSize),
                      handleSize, handleSize);
    painter->drawRect(static_cast<int>(bounds.right() - handleSize),
                      static_cast<int>(bounds.bottom() - handleSize),
                      handleSize, handleSize);
  }
}

QVariant NMSceneObject::itemChange(GraphicsItemChange change,
                                   const QVariant &value) {
  if (change == ItemPositionChange) {
    if (auto *scenePtr = scene()) {
      if (auto *nmScene = qobject_cast<NMSceneGraphicsScene *>(scenePtr)) {
        if (nmScene->snapToGrid()) {
          const qreal grid = nmScene->gridSize();
          QPointF pos = value.toPointF();
          if (!qFuzzyIsNull(grid)) {
            pos.setX(std::round(pos.x() / grid) * grid);
            pos.setY(std::round(pos.y() / grid) * grid);
          }
          return pos;
        }
      }
    }
  }
  if (change == ItemPositionHasChanged) {
    if (auto *scenePtr = scene()) {
      if (auto *nmScene = qobject_cast<NMSceneGraphicsScene *>(scenePtr)) {
        nmScene->handleItemPositionChange(m_id, value.toPointF());
      }
    }
  }
  return QGraphicsPixmapItem::itemChange(change, value);
}

void NMSceneObject::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    event->accept();
  }
  QGraphicsPixmapItem::mousePressEvent(event);
}

void NMSceneObject::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  QGraphicsPixmapItem::mouseMoveEvent(event);
}

void NMSceneObject::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  QGraphicsPixmapItem::mouseReleaseEvent(event);
}


} // namespace NovelMind::editor::qt
