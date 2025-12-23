#pragma once

/**
 * @file nm_undo_manager.hpp
 * @brief Centralized undo/redo management using Qt's QUndoStack
 *
 * This manager provides:
 * - Global undo/redo stack for all editor operations
 * - Command pattern implementation for reversible actions
 * - Integration with Qt's undo framework
 * - Undo history visualization
 */

#include <QObject>
#include <QPointF>
#include <QUndoCommand>
#include <QUndoStack>
#include <QVariant>
#include <QStringList>
#include <QPointer>
#include <memory>
#include <vector>
#include <cstdint>
#include <functional>
#include "NovelMind/core/property_system.hpp"
#include "NovelMind/editor/qt/panels/nm_story_graph_panel.hpp"

namespace NovelMind::editor::qt {

// Forward declarations to avoid heavy coupling in the header
class NMSceneViewPanel;
class NMStoryGraphScene;
enum class NMSceneObjectType : int;

struct SceneObjectSnapshot {
  QString id;
  QString name;
  NMSceneObjectType type;
  QPointF position;
  qreal rotation = 0.0;
  qreal scaleX = 1.0;
  qreal scaleY = 1.0;
  qreal opacity = 1.0;
  bool visible = true;
  qreal zValue = 0.0;
  QString assetPath;
};

struct GraphNodeSnapshot {
  uint64_t id = 0;
  QString idString;
  QString title;
  QString type;
  QPointF position;
  QString scriptPath;
  QString speaker;
  QString dialogueText;
  QStringList choices;
};

struct GraphConnectionSnapshot {
  uint64_t fromId = 0;
  uint64_t toId = 0;
};

/**
 * @brief Centralized undo/redo manager (singleton)
 *
 * Manages all undoable operations in the editor using Qt's QUndoStack.
 * All modifications to the scene, story graph, properties, etc. should
 * go through this system to ensure undo/redo support.
 */
class NMUndoManager : public QObject {
  Q_OBJECT

public:
  /**
   * @brief Get the singleton instance
   */
  static NMUndoManager &instance();

  /**
   * @brief Initialize the undo manager
   */
  void initialize();

  /**
   * @brief Shutdown the undo manager
   */
  void shutdown();

  /**
   * @brief Get the underlying QUndoStack
   */
  [[nodiscard]] QUndoStack *undoStack() const { return m_undoStack.get(); }

  /**
   * @brief Push a command onto the undo stack
   * @param command The command to push (takes ownership)
   */
  void pushCommand(QUndoCommand *command);

  /**
   * @brief Check if undo is available
   */
  [[nodiscard]] bool canUndo() const;

  /**
   * @brief Check if redo is available
   */
  [[nodiscard]] bool canRedo() const;

  /**
   * @brief Get the text for the next undo action
   */
  [[nodiscard]] QString undoText() const;

  /**
   * @brief Get the text for the next redo action
   */
  [[nodiscard]] QString redoText() const;

  /**
   * @brief Begin a macro (group of commands)
   * @param text Description of the macro
   */
  void beginMacro(const QString &text);

  /**
   * @brief End the current macro
   */
  void endMacro();

  /**
   * @brief Clear the undo stack
   */
  void clear();

  /**
   * @brief Set the clean state (typically after save)
   */
  void setClean();

  /**
   * @brief Check if the document is clean (no unsaved changes)
   */
  [[nodiscard]] bool isClean() const;

  /**
   * @brief Set the undo limit (0 = unlimited)
   */
  void setUndoLimit(int limit);

public slots:
  /**
   * @brief Perform undo
   */
  void undo();

  /**
   * @brief Perform redo
   */
  void redo();

signals:
  /**
   * @brief Emitted when undo/redo availability changes
   */
  void canUndoChanged(bool canUndo);
  void canRedoChanged(bool canRedo);

  /**
   * @brief Emitted when undo/redo text changes
   */
  void undoTextChanged(const QString &text);
  void redoTextChanged(const QString &text);

  /**
   * @brief Emitted when clean state changes
   */
  void cleanChanged(bool clean);

  /**
   * @brief Emitted when an index changes in the stack
   */
  void indexChanged(int index);

private:
  NMUndoManager();
  ~NMUndoManager() override;

  NMUndoManager(const NMUndoManager &) = delete;
  NMUndoManager &operator=(const NMUndoManager &) = delete;

  std::unique_ptr<QUndoStack> m_undoStack;
  bool m_initialized = false;
};

// =============================================================================
// Common Command Types
// =============================================================================

/**
 * @brief Command for changing a property value
 */
class PropertyChangeCommand : public QUndoCommand {
public:
  using ApplyFn = std::function<void(const PropertyValue &, bool isUndo)>;

  PropertyChangeCommand(const QString &objectName, const QString &propertyName,
                        const PropertyValue &oldValue,
                        const PropertyValue &newValue,
                        ApplyFn apply, QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  void applyValue(const PropertyValue &value, bool isUndo);

  ApplyFn m_apply;
  QString m_objectName;
  QString m_propertyName;
  PropertyValue m_oldValue;
  PropertyValue m_newValue;
};

/**
 * @brief Command for adding an object to the scene
 */
class AddObjectCommand : public QUndoCommand {
public:
  AddObjectCommand(NMSceneViewPanel *panel, SceneObjectSnapshot snapshot,
                   QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  QPointer<NMSceneViewPanel> m_panel;
  SceneObjectSnapshot m_snapshot;
  bool m_created = false;
};

/**
 * @brief Command for deleting an object from the scene
 */
class DeleteObjectCommand : public QUndoCommand {
public:
  DeleteObjectCommand(NMSceneViewPanel *panel, SceneObjectSnapshot snapshot,
                      QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  QPointer<NMSceneViewPanel> m_panel;
  SceneObjectSnapshot m_snapshot;
};

/**
 * @brief Command for moving/transforming an object
 */
class TransformObjectCommand : public QUndoCommand {
public:
  TransformObjectCommand(NMSceneViewPanel *panel, const QString &objectId,
                         const QPointF &oldPosition,
                         const QPointF &newPosition,
                         qreal oldRotation = 0.0, qreal newRotation = 0.0,
                         qreal oldScaleX = 1.0, qreal newScaleX = 1.0,
                         qreal oldScaleY = 1.0, qreal newScaleY = 1.0,
                         QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;
  bool mergeWith(const QUndoCommand *other) override;
  int id() const override { return 1; } // For command merging

private:
  QPointer<NMSceneViewPanel> m_panel;
  QString m_objectId;
  QPointF m_oldPosition;
  QPointF m_newPosition;
  qreal m_oldRotation = 0.0;
  qreal m_newRotation = 0.0;
  qreal m_oldScaleX = 1.0;
  qreal m_newScaleX = 1.0;
  qreal m_oldScaleY = 1.0;
  qreal m_newScaleY = 1.0;
};

/**
 * @brief Command for creating a graph node
 */
class CreateGraphNodeCommand : public QUndoCommand {
public:
  CreateGraphNodeCommand(NMStoryGraphScene *scene, const QString &nodeType,
                         const QPointF &position,
                         const QString &title = QString(),
                         QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  QPointer<NMStoryGraphScene> m_scene;
  GraphNodeSnapshot m_snapshot;
  bool m_created = false;
};

/**
 * @brief Command for deleting a graph node
 */
class DeleteGraphNodeCommand : public QUndoCommand {
public:
  DeleteGraphNodeCommand(NMStoryGraphScene *scene, uint64_t nodeId,
                         QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  QPointer<NMStoryGraphScene> m_scene;
  GraphNodeSnapshot m_snapshot;
  std::vector<GraphConnectionSnapshot> m_connections;
  bool m_removed = false;
};

/**
 * @brief Command for connecting two graph nodes
 */
class ConnectGraphNodesCommand : public QUndoCommand {
public:
  ConnectGraphNodesCommand(NMStoryGraphScene *scene, uint64_t sourceNodeId,
                           uint64_t targetNodeId, QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  QPointer<NMStoryGraphScene> m_scene;
  GraphConnectionSnapshot m_connection;
  bool m_connected = false;
};

/**
 * @brief Command for disconnecting two graph nodes
 */
class DisconnectGraphNodesCommand : public QUndoCommand {
public:
  DisconnectGraphNodesCommand(NMStoryGraphScene *scene, uint64_t sourceNodeId,
                              uint64_t targetNodeId,
                              QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  QPointer<NMStoryGraphScene> m_scene;
  GraphConnectionSnapshot m_connection;
  bool m_disconnected = false;
};

/**
 * @brief Command for moving one or more graph nodes
 */
class MoveGraphNodesCommand : public QUndoCommand {
public:
  MoveGraphNodesCommand(NMStoryGraphScene *scene,
                        const QVector<GraphNodeMove> &moves,
                        QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;
  bool mergeWith(const QUndoCommand *other) override;
  int id() const override { return 2; }

private:
  QPointer<NMStoryGraphScene> m_scene;
  QVector<GraphNodeMove> m_moves;
};

} // namespace NovelMind::editor::qt
