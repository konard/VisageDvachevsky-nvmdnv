#include "NovelMind/editor/qt/panels/nm_timeline_panel.hpp"
#include "NovelMind/editor/qt/panels/nm_keyframe_item.hpp"
#include "NovelMind/editor/qt/nm_icon_manager.hpp"

#include <QBrush>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QPen>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QToolBar>
#include <QVBoxLayout>

namespace NovelMind::editor::qt {

// =============================================================================
// TimelineTrack Implementation
// =============================================================================

void TimelineTrack::addKeyframe(int frame, const QVariant &value,
                                EasingType easing) {
  // Check if keyframe already exists
  for (auto &kf : keyframes) {
    if (kf.frame == frame) {
      kf.value = value;
      kf.easing = easing;
      return;
    }
  }

  // Add new keyframe
  Keyframe newKeyframe;
  newKeyframe.frame = frame;
  newKeyframe.value = value;
  newKeyframe.easing = easing;
  keyframes.append(newKeyframe);

  // Sort keyframes by frame
  std::sort(
      keyframes.begin(), keyframes.end(),
      [](const Keyframe &a, const Keyframe &b) { return a.frame < b.frame; });
}

void TimelineTrack::removeKeyframe(int frame) {
  keyframes.erase(
      std::remove_if(keyframes.begin(), keyframes.end(),
                     [frame](const Keyframe &kf) { return kf.frame == frame; }),
      keyframes.end());
}

void TimelineTrack::moveKeyframe(int fromFrame, int toFrame) {
  Keyframe *kf = getKeyframe(fromFrame);
  if (kf && fromFrame != toFrame) {
    // Store keyframe data
    QVariant value = kf->value;
    EasingType easing = kf->easing;
    bool selected = kf->isSelected;

    // Remove old keyframe
    removeKeyframe(fromFrame);

    // Add at new position
    addKeyframe(toFrame, value, easing);

    // Restore selection state
    Keyframe *newKf = getKeyframe(toFrame);
    if (newKf) {
      newKf->isSelected = selected;
    }
  }
}

Keyframe *TimelineTrack::getKeyframe(int frame) {
  for (auto &kf : keyframes) {
    if (kf.frame == frame)
      return &kf;
  }
  return nullptr;
}

Keyframe TimelineTrack::interpolate(int frame) const {
  // Simple implementation - return nearest keyframe
  // Full interpolation would consider easing functions
  if (keyframes.isEmpty()) {
    return Keyframe();
  }

  for (const auto &kf : keyframes) {
    if (kf.frame == frame)
      return kf;
  }

  return keyframes.first();
}

QList<Keyframe *> TimelineTrack::selectedKeyframes() {
  QList<Keyframe *> selected;
  for (auto &kf : keyframes) {
    if (kf.isSelected) {
      selected.append(&kf);
    }
  }
  return selected;
}

void TimelineTrack::selectKeyframesInRange(int startFrame, int endFrame) {
  for (auto &kf : keyframes) {
    if (kf.frame >= startFrame && kf.frame <= endFrame) {
      kf.isSelected = true;
    }
  }
}

void TimelineTrack::clearSelection() {
  for (auto &kf : keyframes) {
    kf.isSelected = false;
  }
}

// =============================================================================
// NMTimelinePanel Implementation
// =============================================================================

NMTimelinePanel::NMTimelinePanel(QWidget *parent)
    : NMDockPanel("Timeline", parent) {}

NMTimelinePanel::~NMTimelinePanel() = default;

void NMTimelinePanel::onInitialize() {
  setupUI();

  // Create default tracks
  addTrack(TimelineTrackType::Audio, "Background Music");
  addTrack(TimelineTrackType::Animation, "Character Animation");
  addTrack(TimelineTrackType::Event, "Story Events");
}

void NMTimelinePanel::onShutdown() {
  // Clean up tracks
  qDeleteAll(m_tracks);
  m_tracks.clear();
}

void NMTimelinePanel::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget());
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  setupToolbar();
  mainLayout->addWidget(m_toolbar);

  setupPlaybackControls();

  setupTrackView();
  mainLayout->addWidget(m_timelineView, 1);
}

void NMTimelinePanel::setupToolbar() {
  m_toolbar = new QToolBar(contentWidget());
  m_toolbar->setObjectName("TimelineToolbar");

  auto &iconMgr = NMIconManager::instance();

  // Playback controls
  m_btnPlay = new QPushButton(m_toolbar);
  m_btnPlay->setIcon(iconMgr.getIcon("play", 16));
  m_btnPlay->setCheckable(true);
  m_btnPlay->setToolTip("Play/Pause (Space)");
  m_btnPlay->setFlat(true);
  connect(m_btnPlay, &QPushButton::clicked, this,
          &NMTimelinePanel::togglePlayback);

  m_btnStop = new QPushButton(m_toolbar);
  m_btnStop->setIcon(iconMgr.getIcon("stop", 16));
  m_btnStop->setToolTip("Stop");
  m_btnStop->setFlat(true);
  connect(m_btnStop, &QPushButton::clicked, this,
          &NMTimelinePanel::stopPlayback);

  m_btnStepBack = new QPushButton(m_toolbar);
  m_btnStepBack->setIcon(iconMgr.getIcon("step-backward", 16));
  m_btnStepBack->setToolTip("Step Backward");
  m_btnStepBack->setFlat(true);
  connect(m_btnStepBack, &QPushButton::clicked, this,
          &NMTimelinePanel::stepBackward);

  m_btnStepForward = new QPushButton(m_toolbar);
  m_btnStepForward->setIcon(iconMgr.getIcon("step-forward", 16));
  m_btnStepForward->setToolTip("Step Forward");
  m_btnStepForward->setFlat(true);
  connect(m_btnStepForward, &QPushButton::clicked, this,
          &NMTimelinePanel::stepForward);

  // Frame display
  m_frameSpinBox = new QSpinBox(m_toolbar);
  m_frameSpinBox->setMinimum(0);
  m_frameSpinBox->setMaximum(m_totalFrames);
  m_frameSpinBox->setValue(m_currentFrame);
  m_frameSpinBox->setToolTip("Current Frame");
  connect(m_frameSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &NMTimelinePanel::setCurrentFrame);

  m_timeLabel = new QLabel("00:00.00", m_toolbar);
  m_timeLabel->setMinimumWidth(60);

  // Zoom controls
  m_btnZoomIn = new QPushButton(m_toolbar);
  m_btnZoomIn->setIcon(iconMgr.getIcon("zoom-in", 16));
  m_btnZoomIn->setToolTip("Zoom In");
  m_btnZoomIn->setFlat(true);
  connect(m_btnZoomIn, &QPushButton::clicked, this, &NMTimelinePanel::zoomIn);

  m_btnZoomOut = new QPushButton(m_toolbar);
  m_btnZoomOut->setIcon(iconMgr.getIcon("zoom-out", 16));
  m_btnZoomOut->setToolTip("Zoom Out");
  m_btnZoomOut->setFlat(true);
  connect(m_btnZoomOut, &QPushButton::clicked, this, &NMTimelinePanel::zoomOut);

  m_btnZoomFit = new QPushButton(m_toolbar);
  m_btnZoomFit->setIcon(iconMgr.getIcon("zoom-fit", 16));
  m_btnZoomFit->setToolTip("Zoom to Fit");
  m_btnZoomFit->setFlat(true);
  connect(m_btnZoomFit, &QPushButton::clicked, this,
          &NMTimelinePanel::zoomToFit);

  // Add widgets to toolbar
  m_toolbar->addWidget(m_btnPlay);
  m_toolbar->addWidget(m_btnStop);
  m_toolbar->addSeparator();
  m_toolbar->addWidget(m_btnStepBack);
  m_toolbar->addWidget(m_btnStepForward);
  m_toolbar->addSeparator();
  m_toolbar->addWidget(new QLabel("Frame:", m_toolbar));
  m_toolbar->addWidget(m_frameSpinBox);
  m_toolbar->addWidget(m_timeLabel);
  m_toolbar->addSeparator();
  m_toolbar->addWidget(m_btnZoomIn);
  m_toolbar->addWidget(m_btnZoomOut);
  m_toolbar->addWidget(m_btnZoomFit);
}

void NMTimelinePanel::setupPlaybackControls() {
  // Additional playback controls can be added here
}

void NMTimelinePanel::setupTrackView() {
  m_timelineScene = new QGraphicsScene(this);
  m_timelineView = new QGraphicsView(m_timelineScene, contentWidget());
  m_timelineView->setObjectName("TimelineView");
  m_timelineView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  m_timelineView->setDragMode(QGraphicsView::ScrollHandDrag);
  m_timelineView->setFocusPolicy(Qt::StrongFocus);

  // Install event filter for keyboard handling
  m_timelineView->installEventFilter(this);

  // Create playhead
  m_playheadItem =
      m_timelineScene->addLine(TRACK_HEADER_WIDTH, 0, TRACK_HEADER_WIDTH, 1000,
                               QPen(QColor("#ff0000"), 2));
  m_playheadItem->setZValue(100); // Always on top

  renderTracks();
}

void NMTimelinePanel::onUpdate(double deltaTime) {
  if (!m_playing)
    return;

  m_playbackTime += deltaTime;
  int newFrame = static_cast<int>(m_playbackTime * m_fps);

  if (newFrame != m_currentFrame) {
    if (newFrame >= m_playbackEndFrame) {
      if (m_loop) {
        m_playbackTime = static_cast<double>(m_playbackStartFrame) / m_fps;
        newFrame = m_playbackStartFrame;
      } else {
        stopPlayback();
        return;
      }
    }

    setCurrentFrame(newFrame);
  }
}

void NMTimelinePanel::setCurrentFrame(int frame) {
  if (frame < 0)
    frame = 0;
  if (frame > m_totalFrames)
    frame = m_totalFrames;

  m_currentFrame = frame;
  m_frameSpinBox->blockSignals(true);
  m_frameSpinBox->setValue(frame);
  m_frameSpinBox->blockSignals(false);

  updatePlayhead();
  updateFrameDisplay();

  emit frameChanged(frame);
}

void NMTimelinePanel::togglePlayback() {
  m_playing = !m_playing;

  if (m_playing) {
    m_playbackTime = static_cast<double>(m_currentFrame) / m_fps;
    m_btnPlay->setText("\u23F8"); // Pause symbol
  } else {
    m_btnPlay->setText("\u25B6"); // Play symbol
  }

  emit playbackStateChanged(m_playing);
}

void NMTimelinePanel::stopPlayback() {
  m_playing = false;
  m_btnPlay->setChecked(false);
  m_btnPlay->setText("\u25B6");
  setCurrentFrame(m_playbackStartFrame);
  emit playbackStateChanged(false);
}

void NMTimelinePanel::stepForward() { setCurrentFrame(m_currentFrame + 1); }

void NMTimelinePanel::stepBackward() { setCurrentFrame(m_currentFrame - 1); }

void NMTimelinePanel::addTrack(TimelineTrackType type, const QString &name) {
  if (m_tracks.contains(name))
    return;

  TimelineTrack *track = new TimelineTrack();
  track->name = name;
  track->type = type;

  // Assign color based on type
  switch (type) {
  case TimelineTrackType::Audio:
    track->color = QColor("#4CAF50");
    break;
  case TimelineTrackType::Animation:
    track->color = QColor("#2196F3");
    break;
  case TimelineTrackType::Event:
    track->color = QColor("#FF9800");
    break;
  case TimelineTrackType::Camera:
    track->color = QColor("#9C27B0");
    break;
  case TimelineTrackType::Character:
    track->color = QColor("#F44336");
    break;
  case TimelineTrackType::Effect:
    track->color = QColor("#00BCD4");
    break;
  }

  m_tracks[name] = track;
  renderTracks();
}

void NMTimelinePanel::removeTrack(const QString &name) {
  if (!m_tracks.contains(name))
    return;

  delete m_tracks[name];
  m_tracks.remove(name);
  renderTracks();
}

void NMTimelinePanel::addKeyframeAtCurrent(const QString &trackName,
                                           const QVariant &value) {
  if (!m_tracks.contains(trackName))
    return;

  m_tracks[trackName]->addKeyframe(m_currentFrame, value);
  renderTracks();

  emit keyframeModified(trackName, m_currentFrame);
}

void NMTimelinePanel::zoomIn() {
  m_zoom *= 1.2f;
  m_pixelsPerFrame = static_cast<int>(4 * m_zoom);
  renderTracks();
}

void NMTimelinePanel::zoomOut() {
  m_zoom /= 1.2f;
  if (m_zoom < 0.1f)
    m_zoom = 0.1f;
  m_pixelsPerFrame = static_cast<int>(4 * m_zoom);
  renderTracks();
}

void NMTimelinePanel::zoomToFit() {
  m_zoom = 1.0f;
  m_pixelsPerFrame = 4;
  renderTracks();
}

void NMTimelinePanel::updatePlayhead() {
  int x = frameToX(m_currentFrame);
  m_playheadItem->setLine(
      x, 0, x,
      static_cast<qreal>(m_tracks.size() * TRACK_HEIGHT + TIMELINE_MARGIN * 2));
}

void NMTimelinePanel::updateFrameDisplay() {
  int totalSeconds = m_currentFrame / m_fps;
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;
  int frames = m_currentFrame % m_fps;

  m_timeLabel->setText(QString("%1:%2.%3")
                           .arg(minutes, 2, 10, QChar('0'))
                           .arg(seconds, 2, 10, QChar('0'))
                           .arg(frames, 2, 10, QChar('0')));
}

void NMTimelinePanel::renderTracks() {
  // Clear existing track visualization (except playhead)
  QList<QGraphicsItem *> items = m_timelineScene->items();
  for (QGraphicsItem *item : items) {
    if (item != m_playheadItem) {
      m_timelineScene->removeItem(item);
      delete item;
    }
  }

  // Clear keyframe item map
  m_keyframeItems.clear();

  int y = TIMELINE_MARGIN;
  int trackIndex = 0;

  // Draw frame ruler
  for (int frame = 0; frame <= m_totalFrames; frame += 10) {
    int x = frameToX(frame);
    m_timelineScene->addLine(x, 0, x, 10, QPen(QColor("#606060")));

    if (frame % 30 == 0) // Every second
    {
      QGraphicsTextItem *label =
          m_timelineScene->addText(QString::number(frame));
      label->setPos(x - 10, -20);
      label->setDefaultTextColor(QColor("#a0a0a0"));
    }
  }

  // Draw tracks
  for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it, ++trackIndex) {
    TimelineTrack *track = it.value();

    // Track background
    m_timelineScene->addRect(
        TRACK_HEADER_WIDTH, y, frameToX(m_totalFrames) - TRACK_HEADER_WIDTH,
        TRACK_HEIGHT, QPen(Qt::NoPen), QBrush(QColor("#2d2d2d")));

    // Track header
    m_timelineScene->addRect(0, y, TRACK_HEADER_WIDTH, TRACK_HEIGHT,
                             QPen(Qt::NoPen), QBrush(track->color.darker(150)));

    QGraphicsTextItem *nameLabel = m_timelineScene->addText(track->name);
    nameLabel->setPos(8, y + 8);
    nameLabel->setDefaultTextColor(QColor("#e0e0e0"));

    // Draw keyframes using custom items
    for (const Keyframe &kf : track->keyframes) {
      int kfX = frameToX(kf.frame);

      // Create custom keyframe item
      NMKeyframeItem *kfItem =
          new NMKeyframeItem(trackIndex, kf.frame, track->color);
      kfItem->setPos(kfX, y + TRACK_HEIGHT / 2);
      kfItem->setSnapToGrid(m_snapToGrid);
      kfItem->setGridSize(m_gridSize);

      // Set coordinate conversion functions
      kfItem->setFrameConverter([this](int x) { return this->xToFrame(x); },
                                 [this](int f) { return this->frameToX(f); });

      // Connect signals
      connect(kfItem, &NMKeyframeItem::clicked, this,
              &NMTimelinePanel::onKeyframeClicked);
      connect(kfItem, &NMKeyframeItem::moved, this,
              &NMTimelinePanel::onKeyframeMoved);
      connect(kfItem, &NMKeyframeItem::doubleClicked, this,
              &NMTimelinePanel::onKeyframeDoubleClicked);

      // Add to scene
      m_timelineScene->addItem(kfItem);

      // Store in map
      KeyframeId id;
      id.trackIndex = trackIndex;
      id.frame = kf.frame;
      m_keyframeItems[id] = kfItem;

      // Restore selection state
      if (m_selectedKeyframes.contains(id)) {
        kfItem->setSelected(true);
      }
    }

    y += TRACK_HEIGHT;
  }

  // Update scene rect
  m_timelineScene->setSceneRect(0, -30, frameToX(m_totalFrames) + 100,
                                y + TIMELINE_MARGIN);

  updatePlayhead();
}

int NMTimelinePanel::frameToX(int frame) const {
  return TRACK_HEADER_WIDTH + frame * m_pixelsPerFrame;
}

int NMTimelinePanel::xToFrame(int x) const {
  return (x - TRACK_HEADER_WIDTH) / m_pixelsPerFrame;
}

// =============================================================================
// Selection Management
// =============================================================================

void NMTimelinePanel::selectKeyframe(const KeyframeId &id, bool additive) {
  if (!additive) {
    // Clear previous selection
    m_selectedKeyframes.clear();
  }

  if (m_selectedKeyframes.contains(id)) {
    // Toggle if already selected in additive mode
    if (additive) {
      m_selectedKeyframes.remove(id);
    }
  } else {
    m_selectedKeyframes.insert(id);
  }

  updateSelectionVisuals();
}

void NMTimelinePanel::clearSelection() {
  m_selectedKeyframes.clear();
  updateSelectionVisuals();
}

void NMTimelinePanel::updateSelectionVisuals() {
  for (auto it = m_keyframeItems.begin(); it != m_keyframeItems.end(); ++it) {
    bool selected = m_selectedKeyframes.contains(it.key());
    it.value()->setSelected(selected);
  }

  // Update data model selection state
  int trackIndex = 0;
  for (auto trackIt = m_tracks.begin(); trackIt != m_tracks.end();
       ++trackIt, ++trackIndex) {
    TimelineTrack *track = trackIt.value();
    for (auto &kf : track->keyframes) {
      KeyframeId id;
      id.trackIndex = trackIndex;
      id.frame = kf.frame;
      kf.isSelected = m_selectedKeyframes.contains(id);
    }
  }
}

// =============================================================================
// Keyframe Event Handlers
// =============================================================================

void NMTimelinePanel::onKeyframeClicked(bool additiveSelection,
                                        const KeyframeId &id) {
  selectKeyframe(id, additiveSelection);
}

void NMTimelinePanel::onKeyframeMoved(int oldFrame, int newFrame,
                                      int trackIndex) {
  // Find the track
  int currentTrackIndex = 0;
  TimelineTrack *targetTrack = nullptr;

  for (auto it = m_tracks.begin(); it != m_tracks.end();
       ++it, ++currentTrackIndex) {
    if (currentTrackIndex == trackIndex) {
      targetTrack = it.value();
      break;
    }
  }

  if (!targetTrack)
    return;

  // Move keyframe in data model
  targetTrack->moveKeyframe(oldFrame, newFrame);

  // Update selection to new position
  KeyframeId oldId;
  oldId.trackIndex = trackIndex;
  oldId.frame = oldFrame;

  KeyframeId newId;
  newId.trackIndex = trackIndex;
  newId.frame = newFrame;

  if (m_selectedKeyframes.contains(oldId)) {
    m_selectedKeyframes.remove(oldId);
    m_selectedKeyframes.insert(newId);
  }

  // Re-render to update positions
  renderTracks();

  emit keyframeMoved(QString::number(trackIndex), oldFrame, newFrame);
}

void NMTimelinePanel::onKeyframeDoubleClicked(int trackIndex, int frame) {
  showEasingDialog(trackIndex, frame);
}

// =============================================================================
// Easing Dialog
// =============================================================================

void NMTimelinePanel::showEasingDialog(int trackIndex, int frame) {
  // Find the track and keyframe
  int currentTrackIndex = 0;
  TimelineTrack *targetTrack = nullptr;
  Keyframe *targetKeyframe = nullptr;

  for (auto it = m_tracks.begin(); it != m_tracks.end();
       ++it, ++currentTrackIndex) {
    if (currentTrackIndex == trackIndex) {
      targetTrack = it.value();
      targetKeyframe = targetTrack->getKeyframe(frame);
      break;
    }
  }

  if (!targetTrack || !targetKeyframe)
    return;

  // Create easing selection dialog
  QDialog dialog(this);
  dialog.setWindowTitle("Select Easing Type");
  QVBoxLayout *layout = new QVBoxLayout(&dialog);

  QListWidget *easingList = new QListWidget(&dialog);
  easingList->addItem("Linear");
  easingList->addItem("Ease In");
  easingList->addItem("Ease Out");
  easingList->addItem("Ease In Out");
  easingList->addItem("Ease In Quad");
  easingList->addItem("Ease Out Quad");
  easingList->addItem("Ease In Out Quad");
  easingList->addItem("Ease In Cubic");
  easingList->addItem("Ease Out Cubic");
  easingList->addItem("Ease In Out Cubic");
  easingList->addItem("Ease In Elastic");
  easingList->addItem("Ease Out Elastic");
  easingList->addItem("Ease In Bounce");
  easingList->addItem("Ease Out Bounce");
  easingList->addItem("Step");
  easingList->addItem("Custom");

  // Select current easing
  easingList->setCurrentRow(static_cast<int>(targetKeyframe->easing));

  layout->addWidget(easingList);

  QDialogButtonBox *buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  layout->addWidget(buttonBox);

  if (dialog.exec() == QDialog::Accepted) {
    int selectedIndex = easingList->currentRow();
    if (selectedIndex >= 0 &&
        selectedIndex < static_cast<int>(EasingType::Custom) + 1) {
      targetKeyframe->easing = static_cast<EasingType>(selectedIndex);
      emit keyframeEasingChanged(targetTrack->name, frame,
                                  targetKeyframe->easing);
    }
  }
}

// =============================================================================
// Delete Selected Keyframes
// =============================================================================

void NMTimelinePanel::deleteSelectedKeyframes() {
  if (m_selectedKeyframes.isEmpty())
    return;

  // Delete keyframes from data model
  int trackIndex = 0;
  for (auto trackIt = m_tracks.begin(); trackIt != m_tracks.end();
       ++trackIt, ++trackIndex) {
    TimelineTrack *track = trackIt.value();

    // Collect frames to delete for this track
    QVector<int> framesToDelete;
    for (const KeyframeId &id : m_selectedKeyframes) {
      if (id.trackIndex == trackIndex) {
        framesToDelete.append(id.frame);
      }
    }

    // Delete the keyframes
    for (int frame : framesToDelete) {
      track->removeKeyframe(frame);
      emit keyframeDeleted(track->name, frame);
    }
  }

  // Clear selection
  m_selectedKeyframes.clear();

  // Re-render
  renderTracks();
}

// =============================================================================
// Event Filter for Keyboard
// =============================================================================

bool NMTimelinePanel::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

    if (keyEvent->key() == Qt::Key_Delete ||
        keyEvent->key() == Qt::Key_Backspace) {
      deleteSelectedKeyframes();
      return true;
    }
  }

  return NMDockPanel::eventFilter(obj, event);
}

// =============================================================================
// Track Access Methods
// =============================================================================

TimelineTrack* NMTimelinePanel::getTrack(const QString& name) const {
  auto it = m_tracks.find(name);
  if (it != m_tracks.end()) {
    return it.value();
  }
  return nullptr;
}

} // namespace NovelMind::editor::qt
