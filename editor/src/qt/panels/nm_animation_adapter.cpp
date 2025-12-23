#include "NovelMind/editor/qt/panels/nm_animation_adapter.hpp"
#include "NovelMind/editor/qt/panels/nm_scene_view_panel.hpp"
#include "NovelMind/scene/scene_object.hpp"
#include <QDebug>
#include <algorithm>

namespace NovelMind::editor::qt {

// =============================================================================
// NMAnimationAdapter Implementation
// =============================================================================

NMAnimationAdapter::NMAnimationAdapter(scene::SceneManager* sceneManager,
                                     QObject* parent)
    : QObject(parent), m_sceneManager(sceneManager) {
  if (!m_sceneManager) {
    qWarning() << "[AnimationAdapter] SceneManager is null!";
  }
}

NMAnimationAdapter::~NMAnimationAdapter() {
  cleanupAnimations();
}

void NMAnimationAdapter::connectTimeline(NMTimelinePanel* timeline) {
  if (m_timeline) {
    // Disconnect previous timeline
    disconnect(m_timeline, nullptr, this, nullptr);
  }

  m_timeline = timeline;

  if (m_timeline) {
    connect(m_timeline, &NMTimelinePanel::frameChanged,
            this, &NMAnimationAdapter::onTimelineFrameChanged);
    connect(m_timeline, &NMTimelinePanel::playbackStateChanged,
            this, &NMAnimationAdapter::onTimelinePlaybackStateChanged);
    connect(m_timeline, &NMTimelinePanel::keyframeModified,
            this, &NMAnimationAdapter::onKeyframeModified);

    qDebug() << "[AnimationAdapter] Connected to Timeline panel";
  }
}

void NMAnimationAdapter::connectSceneView(NMSceneViewPanel* sceneView) {
  m_sceneView = sceneView;

  if (m_sceneView) {
    // Connect scene update signal to scene view redraw
    connect(this, &NMAnimationAdapter::sceneUpdateRequired,
            m_sceneView, [this]() {
              if (m_sceneView->graphicsView()) {
                m_sceneView->graphicsView()->viewport()->update();
              }
            });

    qDebug() << "[AnimationAdapter] Connected to SceneView panel";
  }
}

bool NMAnimationAdapter::createBinding(const QString& trackId,
                                      const QString& objectId,
                                      AnimatedProperty property) {
  AnimationBinding binding;
  binding.trackId = trackId;
  binding.objectId = objectId;
  binding.property = property;

  if (!binding.isValid()) {
    qWarning() << "[AnimationAdapter] Invalid binding: trackId=" << trackId
               << "objectId=" << objectId;
    return false;
  }

  m_bindings[trackId] = binding;

  // Initialize property storage for this object if not exists
  if (m_propertyStorage.find(objectId) == m_propertyStorage.end()) {
    PropertyStorage storage;

    // Initialize from scene object if available
    if (m_sceneManager) {
      scene::SceneObject* obj = m_sceneManager->findObject(objectId.toStdString());
      if (obj) {
        const auto& transform = obj->getTransform();
        storage.positionX = transform.position.x;
        storage.positionY = transform.position.y;
        storage.scaleX = transform.scale.x;
        storage.scaleY = transform.scale.y;
        storage.rotation = transform.rotation;
        storage.alpha = obj->getAlpha();
      }
    }

    m_propertyStorage[objectId] = storage;
  }

  qDebug() << "[AnimationAdapter] Created binding:" << trackId << "->" << objectId;

  return true;
}

void NMAnimationAdapter::removeBinding(const QString& trackId) {
  auto it = m_bindings.find(trackId);
  if (it != m_bindings.end()) {
    m_bindings.erase(it);

    // Also remove animation state
    auto stateIt = m_animationStates.find(trackId);
    if (stateIt != m_animationStates.end()) {
      m_animationStates.erase(stateIt);
    }

    qDebug() << "[AnimationAdapter] Removed binding:" << trackId;
  }
}

QList<AnimationBinding> NMAnimationAdapter::getBindings() const {
  QList<AnimationBinding> result;
  for (const auto& [trackId, binding] : m_bindings) {
    result.append(binding);
  }
  return result;
}

void NMAnimationAdapter::startPreview() {
  if (m_isPreviewActive) {
    qDebug() << "[AnimationAdapter] Preview already active";
    return;
  }

  m_isPreviewActive = true;
  rebuildAnimations();

  qDebug() << "[AnimationAdapter] Preview started";
  emit previewStarted();
}

void NMAnimationAdapter::stopPreview() {
  if (!m_isPreviewActive) {
    return;
  }

  m_isPreviewActive = false;
  cleanupAnimations();

  qDebug() << "[AnimationAdapter] Preview stopped";
  emit previewStopped();
}

scene::EaseType NMAnimationAdapter::mapEasingType(EasingType timelineEasing) {
  switch (timelineEasing) {
    case EasingType::Linear:
      return scene::EaseType::Linear;
    case EasingType::EaseIn:
    case EasingType::EaseInQuad:
      return scene::EaseType::EaseInQuad;
    case EasingType::EaseOut:
    case EasingType::EaseOutQuad:
      return scene::EaseType::EaseOutQuad;
    case EasingType::EaseInOut:
    case EasingType::EaseInOutQuad:
      return scene::EaseType::EaseInOutQuad;
    case EasingType::EaseInCubic:
      return scene::EaseType::EaseInCubic;
    case EasingType::EaseOutCubic:
      return scene::EaseType::EaseOutCubic;
    case EasingType::EaseInOutCubic:
      return scene::EaseType::EaseInOutCubic;
    case EasingType::EaseInElastic:
      return scene::EaseType::EaseInElastic;
    case EasingType::EaseOutElastic:
      return scene::EaseType::EaseOutElastic;
    case EasingType::EaseInBounce:
      return scene::EaseType::EaseInBounce;
    case EasingType::EaseOutBounce:
      return scene::EaseType::EaseOutBounce;
    case EasingType::Step:
    case EasingType::Custom:
    default:
      return scene::EaseType::Linear;
  }
}

// =============================================================================
// Slots
// =============================================================================

void NMAnimationAdapter::onTimelineFrameChanged(int frame) {
  if (!m_isPreviewActive) {
    return;
  }

  // Convert frame to time
  f64 time = static_cast<f64>(frame) / static_cast<f64>(m_fps);
  m_currentTime = time;

  // Seek all animations to this time
  seekToTime(time);

  // Request scene redraw
  emit sceneUpdateRequired();
}

void NMAnimationAdapter::onTimelinePlaybackStateChanged(bool playing) {
  if (playing && !m_isPreviewActive) {
    startPreview();
  } else if (!playing && m_isPreviewActive) {
    // Keep preview active but paused
    // User can still scrub through frames
  }
}

void NMAnimationAdapter::onKeyframeModified(const QString& trackName, int frame) {
  Q_UNUSED(frame);

  // Rebuild animation for modified track
  auto bindingIt = m_bindings.find(trackName);
  if (bindingIt != m_bindings.end() && m_timeline) {
    // Trigger a full rebuild when any keyframe changes
    rebuildAnimations();
  }
}

void NMAnimationAdapter::rebuildAnimations() {
  if (!m_timeline) {
    qWarning() << "[AnimationAdapter] No timeline connected, cannot rebuild animations";
    return;
  }

  // Clear existing animations
  m_animationStates.clear();

  // Build animations for each binding
  for (const auto& [trackId, binding] : m_bindings) {
    // Get track from timeline
    TimelineTrack* track = m_timeline->getTrack(trackId);
    if (!track) {
      qWarning() << "[AnimationAdapter] Track not found:" << trackId;
      continue;
    }

    // Create animation state
    AnimationPlaybackState state;
    state.binding = binding;
    state.timeline = buildAnimationFromTrack(track, binding);

    if (state.timeline) {
      // Calculate total duration
      if (!track->keyframes.isEmpty()) {
        int lastFrame = track->keyframes.last().frame;
        state.duration = static_cast<f64>(lastFrame) / static_cast<f64>(m_fps);
      }
    }

    m_animationStates[trackId] = std::move(state);
  }

  qDebug() << "[AnimationAdapter] Rebuilt" << m_animationStates.size() << "animations";
}

// =============================================================================
// Private Methods
// =============================================================================

std::unique_ptr<scene::AnimationTimeline>
NMAnimationAdapter::buildAnimationFromTrack(TimelineTrack* track,
                                           const AnimationBinding& binding) {
  if (!track || track->keyframes.isEmpty()) {
    return nullptr;
  }

  auto timeline = std::make_unique<scene::AnimationTimeline>();

  // Build tweens for each keyframe segment
  for (int i = 0; i < track->keyframes.size() - 1; ++i) {
    const Keyframe& kf1 = track->keyframes[i];
    const Keyframe& kf2 = track->keyframes[i + 1];

    // Calculate duration in seconds
    f32 duration = static_cast<f32>(kf2.frame - kf1.frame) / static_cast<f32>(m_fps);

    if (duration <= 0.0f) {
      continue; // Skip zero-duration segments
    }

    // Create tween for this segment
    auto tween = createTweenForProperty(binding, kf1, kf2, duration);
    if (tween) {
      timeline->append(std::move(tween));
    }
  }

  return timeline;
}

std::unique_ptr<scene::Tween>
NMAnimationAdapter::createTweenForProperty(const AnimationBinding& binding,
                                          const Keyframe& kf1,
                                          const Keyframe& kf2,
                                          f32 duration) {
  scene::EaseType easing = mapEasingType(kf1.easing);
  const QString& objectId = binding.objectId;

  // Get property storage for this object
  auto storageIt = m_propertyStorage.find(objectId);
  if (storageIt == m_propertyStorage.end()) {
    qWarning() << "[AnimationAdapter] No property storage for object:" << objectId;
    return nullptr;
  }

  PropertyStorage& storage = storageIt->second;

  switch (binding.property) {
    case AnimatedProperty::PositionX: {
      f32 startVal = kf1.value.toFloat();
      f32 endVal = kf2.value.toFloat();
      return std::make_unique<scene::FloatTween>(
          &storage.positionX, startVal, endVal, duration, easing);
    }

    case AnimatedProperty::PositionY: {
      f32 startVal = kf1.value.toFloat();
      f32 endVal = kf2.value.toFloat();
      return std::make_unique<scene::FloatTween>(
          &storage.positionY, startVal, endVal, duration, easing);
    }

    case AnimatedProperty::Position: {
      QPointF start = kf1.value.toPointF();
      QPointF end = kf2.value.toPointF();
      return std::make_unique<scene::PositionTween>(
          &storage.positionX, &storage.positionY,
          static_cast<f32>(start.x()), static_cast<f32>(start.y()),
          static_cast<f32>(end.x()), static_cast<f32>(end.y()),
          duration, easing);
    }

    case AnimatedProperty::ScaleX: {
      f32 startVal = kf1.value.toFloat();
      f32 endVal = kf2.value.toFloat();
      return std::make_unique<scene::FloatTween>(
          &storage.scaleX, startVal, endVal, duration, easing);
    }

    case AnimatedProperty::ScaleY: {
      f32 startVal = kf1.value.toFloat();
      f32 endVal = kf2.value.toFloat();
      return std::make_unique<scene::FloatTween>(
          &storage.scaleY, startVal, endVal, duration, easing);
    }

    case AnimatedProperty::Rotation: {
      f32 startVal = kf1.value.toFloat();
      f32 endVal = kf2.value.toFloat();
      return std::make_unique<scene::FloatTween>(
          &storage.rotation, startVal, endVal, duration, easing);
    }

    case AnimatedProperty::Alpha: {
      f32 startVal = kf1.value.toFloat();
      f32 endVal = kf2.value.toFloat();
      return std::make_unique<scene::FloatTween>(
          &storage.alpha, startVal, endVal, duration, easing);
    }

    default:
      qWarning() << "[AnimationAdapter] Unsupported property type:"
                 << static_cast<int>(binding.property);
      return nullptr;
  }
}

void NMAnimationAdapter::applyAnimationToScene(const AnimationBinding& binding,
                                              f64 time) {
  Q_UNUSED(time);

  if (!m_sceneManager) {
    return;
  }

  // Get the scene object
  scene::SceneObject* obj = m_sceneManager->findObject(
      binding.objectId.toStdString());
  if (!obj) {
    return;
  }

  // Get property storage
  auto storageIt = m_propertyStorage.find(binding.objectId);
  if (storageIt == m_propertyStorage.end()) {
    return;
  }

  const PropertyStorage& storage = storageIt->second;

  // Apply property values to scene object
  switch (binding.property) {
    case AnimatedProperty::PositionX:
      obj->setPosition(storage.positionX, obj->getTransform().position.y);
      break;

    case AnimatedProperty::PositionY:
      obj->setPosition(obj->getTransform().position.x, storage.positionY);
      break;

    case AnimatedProperty::Position:
      obj->setPosition(storage.positionX, storage.positionY);
      break;

    case AnimatedProperty::ScaleX:
      obj->setScale(storage.scaleX, obj->getTransform().scale.y);
      break;

    case AnimatedProperty::ScaleY:
      obj->setScale(obj->getTransform().scale.x, storage.scaleY);
      break;

    case AnimatedProperty::Scale:
      obj->setScale(storage.scaleX, storage.scaleY);
      break;

    case AnimatedProperty::Rotation:
      obj->setRotation(storage.rotation);
      break;

    case AnimatedProperty::Alpha:
      obj->setAlpha(storage.alpha);
      break;

    default:
      break;
  }
}

QVariant NMAnimationAdapter::interpolateTrackValue(TimelineTrack* track, f64 time) {
  if (!track || track->keyframes.isEmpty()) {
    return QVariant();
  }

  // Convert time to frame
  int frame = static_cast<int>(time * m_fps);

  // Find surrounding keyframes
  const Keyframe* kf1 = nullptr;
  const Keyframe* kf2 = nullptr;

  for (int i = 0; i < track->keyframes.size(); ++i) {
    if (track->keyframes[i].frame <= frame) {
      kf1 = &track->keyframes[i];
    }
    if (track->keyframes[i].frame > frame && !kf2) {
      kf2 = &track->keyframes[i];
      break;
    }
  }

  // If no surrounding keyframes, return first or last
  if (!kf1 && !kf2) {
    return track->keyframes.first().value;
  }
  if (!kf2) {
    return kf1->value;
  }
  if (!kf1) {
    return kf2->value;
  }

  // Calculate interpolation factor
  f32 t = static_cast<f32>(frame - kf1->frame) /
          static_cast<f32>(kf2->frame - kf1->frame);

  // Apply easing
  scene::EaseType easing = mapEasingType(kf1->easing);
  f32 easedT = scene::ease(easing, t);

  // Interpolate based on value type
  if (kf1->value.type() == QVariant::PointF) {
    QPointF p1 = kf1->value.toPointF();
    QPointF p2 = kf2->value.toPointF();
    QPointF result = p1 + (p2 - p1) * easedT;
    return QVariant::fromValue(result);
  } else if (kf1->value.canConvert<qreal>()) {
    qreal v1 = kf1->value.toReal();
    qreal v2 = kf2->value.toReal();
    qreal result = v1 + (v2 - v1) * easedT;
    return result;
  }

  // Default: return start value
  return kf1->value;
}

void NMAnimationAdapter::seekToTime(f64 time) {
  // Update all animations to this time
  for (auto& [trackId, state] : m_animationStates) {
    state.currentTime = time;

    // If we have a timeline, update it
    if (state.timeline) {
      // Note: AnimationTimeline doesn't have seekToTime method
      // We need to implement manual interpolation instead
    }

    // Apply current values to scene
    applyAnimationToScene(state.binding, time);
  }
}

void NMAnimationAdapter::cleanupAnimations() {
  m_animationStates.clear();
  m_animationManager.stopAll();
  qDebug() << "[AnimationAdapter] Cleaned up animations";
}

} // namespace NovelMind::editor::qt
