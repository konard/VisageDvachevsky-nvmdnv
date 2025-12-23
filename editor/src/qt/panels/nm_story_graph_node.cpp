#include "NovelMind/editor/qt/panels/nm_story_graph_panel.hpp"
#include "NovelMind/editor/qt/nm_icon_manager.hpp"
#include "NovelMind/editor/qt/nm_play_mode_controller.hpp"
#include "NovelMind/editor/qt/nm_style_manager.hpp"
#include "NovelMind/editor/qt/nm_undo_manager.hpp"
#include "NovelMind/editor/project_manager.hpp"

#include <QAction>
#include <QFrame>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineF>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QPushButton>
#include <QScrollBar>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>
#include <QSet>
#include <filesystem>


namespace NovelMind::editor::qt {

// ============================================================================
// NMGraphNodeItem
// ============================================================================

NMGraphNodeItem::NMGraphNodeItem(const QString &title, const QString &nodeType)
    : m_title(title), m_nodeType(nodeType) {
  setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
}

void NMGraphNodeItem::setTitle(const QString &title) {
  m_title = title;
  update();
}

void NMGraphNodeItem::setNodeType(const QString &type) {
  m_nodeType = type;
  update();
}

void NMGraphNodeItem::setSelected(bool selected) {
  m_isSelected = selected;
  QGraphicsItem::setSelected(selected);
  update();
}

void NMGraphNodeItem::setBreakpoint(bool hasBreakpoint) {
  m_hasBreakpoint = hasBreakpoint;
  // Only update if we're in a valid scene with views
  // The signal connection is queued, so this is safe
  if (scene() && !scene()->views().isEmpty()) {
    update();
  }
}

void NMGraphNodeItem::setCurrentlyExecuting(bool isExecuting) {
  m_isCurrentlyExecuting = isExecuting;
  // Only update if we're in a valid scene with views
  // The signal connection is queued, so this is safe
  if (scene() && !scene()->views().isEmpty()) {
    update();
  }
}

void NMGraphNodeItem::setEntry(bool isEntry) {
  m_isEntry = isEntry;
  if (scene() && !scene()->views().isEmpty()) {
    update();
  }
}

QPointF NMGraphNodeItem::inputPortPosition() const {
  return mapToScene(QPointF(0, NODE_HEIGHT / 2));
}

QPointF NMGraphNodeItem::outputPortPosition() const {
  return mapToScene(QPointF(NODE_WIDTH, NODE_HEIGHT / 2));
}

bool NMGraphNodeItem::hitTestInputPort(const QPointF &scenePos) const {
  const QPointF portPos = inputPortPosition();
  const qreal hitRadius = PORT_RADIUS + 6;
  if (QLineF(portPos, scenePos).length() <= hitRadius) {
    return true;
  }

  const QPointF localPos = mapFromScene(scenePos);
  const qreal zoneWidth = 16.0;
  const QRectF inputZone(0.0, 0.0, zoneWidth, NODE_HEIGHT);
  return inputZone.contains(localPos);
}

bool NMGraphNodeItem::hitTestOutputPort(const QPointF &scenePos) const {
  const QPointF portPos = outputPortPosition();
  const qreal hitRadius = PORT_RADIUS + 6;
  if (QLineF(portPos, scenePos).length() <= hitRadius) {
    return true;
  }

  const QPointF localPos = mapFromScene(scenePos);
  const qreal zoneWidth = 16.0;
  const QRectF outputZone(NODE_WIDTH - zoneWidth, 0.0, zoneWidth, NODE_HEIGHT);
  return outputZone.contains(localPos);
}

QRectF NMGraphNodeItem::boundingRect() const {
  return QRectF(0, 0, NODE_WIDTH, NODE_HEIGHT);
}

void NMGraphNodeItem::paint(QPainter *painter,
                            const QStyleOptionGraphicsItem * /*option*/,
                            QWidget * /*widget*/) {
  const auto &palette = NMStyleManager::instance().palette();

  painter->setRenderHint(QPainter::Antialiasing);

  // Node background
  QColor bgColor = m_isSelected ? palette.nodeSelected : palette.nodeDefault;
  painter->setBrush(bgColor);
  painter->setPen(QPen(palette.borderLight, 1));
  painter->drawRoundedRect(boundingRect(), CORNER_RADIUS, CORNER_RADIUS);

  // Header bar with icon
  QRectF headerRect(0, 0, NODE_WIDTH, 28);
  painter->setBrush(palette.bgDark);
  painter->setPen(Qt::NoPen);
  QPainterPath headerPath;
  headerPath.addRoundedRect(headerRect, CORNER_RADIUS, CORNER_RADIUS);
  // Clip to top corners only
  QPainterPath clipPath;
  clipPath.addRect(QRectF(0, CORNER_RADIUS, NODE_WIDTH, 28 - CORNER_RADIUS));
  headerPath = headerPath.united(clipPath);
  painter->drawPath(headerPath);

  // Node type icon + text (header)
  QString iconName = "node-dialogue"; // default
  QColor iconColor = palette.textSecondary;

  // Map node types to icons and colors
  if (m_nodeType.contains("Dialogue", Qt::CaseInsensitive)) {
    iconName = "node-dialogue";
    iconColor = QColor(100, 180, 255); // Blue
  } else if (m_nodeType.contains("Choice", Qt::CaseInsensitive)) {
    iconName = "node-choice";
    iconColor = QColor(255, 180, 100); // Orange
  } else if (m_nodeType.contains("Event", Qt::CaseInsensitive)) {
    iconName = "node-event";
    iconColor = QColor(255, 220, 100); // Yellow
  } else if (m_nodeType.contains("Condition", Qt::CaseInsensitive)) {
    iconName = "node-condition";
    iconColor = QColor(200, 100, 255); // Purple
  } else if (m_nodeType.contains("Random", Qt::CaseInsensitive)) {
    iconName = "node-random";
    iconColor = QColor(100, 255, 180); // Green
  } else if (m_nodeType.contains("Start", Qt::CaseInsensitive)) {
    iconName = "node-start";
    iconColor = QColor(100, 255, 100); // Bright Green
  } else if (m_nodeType.contains("End", Qt::CaseInsensitive)) {
    iconName = "node-end";
    iconColor = QColor(255, 100, 100); // Red
  } else if (m_nodeType.contains("Jump", Qt::CaseInsensitive)) {
    iconName = "node-jump";
    iconColor = QColor(180, 180, 255); // Light Blue
  } else if (m_nodeType.contains("Variable", Qt::CaseInsensitive)) {
    iconName = "node-variable";
    iconColor = QColor(255, 180, 255); // Pink
  }

  // Draw icon (with null check to prevent segfault if icon fails to load)
  QPixmap iconPixmap = NMIconManager::instance().getPixmap(iconName, 18, iconColor);
  if (!iconPixmap.isNull()) {
    painter->drawPixmap(6, static_cast<int>(headerRect.center().y()) - 9, iconPixmap);
  }

  // Draw node type text
  painter->setPen(palette.textSecondary);
  painter->setFont(NMStyleManager::instance().defaultFont());
  painter->drawText(headerRect.adjusted(28, 0, -8, 0),
                    Qt::AlignVCenter | Qt::AlignLeft, m_nodeType);

  if (m_isEntry) {
    QPolygonF marker;
    marker << QPointF(NODE_WIDTH - 18, 6)
           << QPointF(NODE_WIDTH - 6, 14)
           << QPointF(NODE_WIDTH - 18, 22);
    painter->setBrush(QColor(80, 200, 120));
    painter->setPen(Qt::NoPen);
    painter->drawPolygon(marker);
  }

  // Node title (body)
  QRectF titleRect(8, 34, NODE_WIDTH - 16, NODE_HEIGHT - 42);
  painter->setPen(palette.textPrimary);
  QFont boldFont = NMStyleManager::instance().defaultFont();
  boldFont.setBold(true);
  painter->setFont(boldFont);
  painter->drawText(titleRect, Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap,
                    m_title);

  // Input/output ports
  const QPointF inputPort(0, NODE_HEIGHT / 2);
  const QPointF outputPort(NODE_WIDTH, NODE_HEIGHT / 2);
  painter->setBrush(palette.bgDark);
  painter->setPen(QPen(palette.borderLight, 1));
  painter->drawEllipse(inputPort, PORT_RADIUS, PORT_RADIUS);
  painter->setBrush(palette.accentPrimary);
  painter->drawEllipse(outputPort, PORT_RADIUS, PORT_RADIUS);

  // Selection highlight
  if (m_isSelected) {
    painter->setPen(QPen(palette.accentPrimary, 2));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(boundingRect().adjusted(1, 1, -1, -1),
                             CORNER_RADIUS, CORNER_RADIUS);
  }

  // Breakpoint indicator (red circle in top-left corner)
  if (m_hasBreakpoint) {
    const qreal radius = 8.0;
    const QPointF center(radius + 4, radius + 4);

    painter->setBrush(QColor(220, 60, 60)); // Red
    painter->setPen(QPen(QColor(180, 40, 40), 2));
    painter->drawEllipse(center, radius, radius);

    // Inner highlight for 3D effect
    painter->setBrush(QColor(255, 100, 100, 80));
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(center - QPointF(2, 2), radius * 0.4, radius * 0.4);
  }

  // Currently executing indicator (pulsing green border + glow)
  if (m_isCurrentlyExecuting) {
    // Outer glow effect
    for (int i = 3; i >= 0; --i) {
      int alpha = 40 - (i * 10);
      QColor glowColor(60, 220, 120, alpha);
      painter->setPen(QPen(glowColor, 3 + i * 2));
      painter->setBrush(Qt::NoBrush);
      painter->drawRoundedRect(boundingRect().adjusted(-i, -i, i, i),
                               CORNER_RADIUS + i, CORNER_RADIUS + i);
    }

    // Solid green border
    painter->setPen(QPen(QColor(60, 220, 120), 3));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(boundingRect().adjusted(1, 1, -1, -1),
                             CORNER_RADIUS, CORNER_RADIUS);

    // Execution arrow indicator in top-right corner
    const qreal arrowSize = 16.0;
    const QPointF arrowCenter(NODE_WIDTH - arrowSize - 4, arrowSize / 2 + 4);

    QPainterPath arrowPath;
    arrowPath.moveTo(arrowCenter + QPointF(-arrowSize / 2, -arrowSize / 3));
    arrowPath.lineTo(arrowCenter + QPointF(arrowSize / 2, 0));
    arrowPath.lineTo(arrowCenter + QPointF(-arrowSize / 2, arrowSize / 3));
    arrowPath.closeSubpath();

    painter->setBrush(QColor(60, 220, 120));
    painter->setPen(QPen(QColor(40, 180, 90), 2));
    painter->drawPath(arrowPath);
  }
}

QVariant NMGraphNodeItem::itemChange(GraphicsItemChange change,
                                     const QVariant &value) {
  if (change == ItemPositionHasChanged && scene()) {
    // Update all connections attached to this node
    auto *graphScene = qobject_cast<NMStoryGraphScene *>(scene());
    if (graphScene) {
      const auto connections = graphScene->findConnectionsForNode(this);
      for (auto *conn : connections) {
        if (conn) {
          conn->updatePath();
        }
      }
    }
  } else if (change == ItemSelectedHasChanged) {
    m_isSelected = value.toBool();
  }
  return QGraphicsItem::itemChange(change, value);
}

void NMGraphNodeItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
  QMenu menu;
  auto &iconMgr = NMIconManager::instance();

  // Toggle Breakpoint action
  QAction *breakpointAction =
      menu.addAction(m_hasBreakpoint ? "Remove Breakpoint" : "Add Breakpoint");
  breakpointAction->setIcon(
      iconMgr.getIcon(m_hasBreakpoint ? "remove" : "breakpoint", 16));

  menu.addSeparator();

  // Edit Node action
  QAction *editAction = menu.addAction("Edit Node Properties");
  editAction->setIcon(iconMgr.getIcon("panel-inspector", 16));

  // Set as Entry action
  QAction *entryAction = menu.addAction("Set as Entry");
  entryAction->setIcon(iconMgr.getIcon("node-start", 16));
  if (m_isEntry) {
    entryAction->setEnabled(false);
  }

  // Delete Node action
  QAction *deleteAction = menu.addAction("Delete Node");
  deleteAction->setIcon(iconMgr.getIcon("edit-delete", 16));

  // Show menu and handle action
  QAction *selectedAction = menu.exec(event->screenPos());

  if (selectedAction == breakpointAction) {
    // Toggle breakpoint via Play Mode Controller
    // Only toggle if we have a valid node ID string
    if (!m_nodeIdString.isEmpty()) {
      NMPlayModeController::instance().toggleBreakpoint(m_nodeIdString);
      // Update visual state immediately
      setBreakpoint(NMPlayModeController::instance().hasBreakpoint(m_nodeIdString));
    }
  } else if (selectedAction == deleteAction) {
    if (auto *graphScene = qobject_cast<NMStoryGraphScene *>(scene())) {
      NMUndoManager::instance().pushCommand(
          new DeleteGraphNodeCommand(graphScene, nodeId()));
    }
  } else if (selectedAction == entryAction) {
    if (auto *graphScene = qobject_cast<NMStoryGraphScene *>(scene())) {
      graphScene->requestEntryNode(m_nodeIdString);
    }
  } else if (selectedAction == editAction) {
    if (scene() && !scene()->views().isEmpty()) {
      if (auto *view =
              qobject_cast<NMStoryGraphView *>(scene()->views().first())) {
        view->emitNodeClicked(nodeId());
      }
    }
  }

  event->accept();
}

// ============================================================================

} // namespace NovelMind::editor::qt
