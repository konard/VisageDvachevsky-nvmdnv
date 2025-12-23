# Animation System Integration

## Overview

This document describes the integration between the Timeline editor panel and the engine_core animation system.

## Architecture

### Components

1. **Timeline Panel** (`nm_timeline_panel.*`)
   - GUI component for keyframe-based animation editing
   - Manages tracks, keyframes, and playback controls
   - Emits signals when keyframes are modified

2. **Engine Core Animation** (`engine_core/include/NovelMind/scene/animation.hpp`)
   - Runtime animation system with tweens and easing functions
   - Manages animation playback and property interpolation
   - Provides `AnimationManager` for tracking active animations

3. **Animation Adapter** (`nm_animation_adapter.*`)
   - Bridge between Timeline GUI model and engine_core animation model
   - Converts Timeline tracks to engine_core `AnimationTimeline` objects
   - Manages synchronization between editor and runtime

4. **Scene View Panel** (`nm_scene_view_panel.*`)
   - Displays preview of scene with animations
   - Receives animation updates from Timeline
   - Renders animated objects in real-time

## Track → Property Mapping

### Animation Track Types

| Timeline Track Type | Target Property | engine_core Tween Type |
|-------------------|----------------|----------------------|
| `TimelineTrackType::Animation` | Position (X, Y) | `PositionTween` |
| `TimelineTrackType::Animation` | Scale (X, Y) | Custom `FloatTween` per axis |
| `TimelineTrackType::Animation` | Rotation | `FloatTween` |
| `TimelineTrackType::Animation` | Alpha/Opacity | `FloatTween` |
| `TimelineTrackType::Animation` | Color | `ColorTween` |
| `TimelineTrackType::Event` | Callback | `CallbackTween` |

### Track Naming Convention

Track names follow the pattern: `<ObjectType>.<ObjectID>.<PropertyName>`

Examples:
- `Character.hero.position`
- `Character.hero.alpha`
- `Background.scene1.color`
- `Effect.particles.scale`

### Property Bindings

Each keyframe in a Timeline track maps to a property of a scene object:

```cpp
// Example: Timeline keyframe at frame 30
Keyframe kf;
kf.frame = 30;
kf.value = QVariant::fromValue(QPointF(100.0f, 200.0f)); // Position
kf.easing = EasingType::EaseOutQuad;

// Maps to engine_core animation
auto tween = std::make_unique<PositionTween>(
    &obj->getTransform().position.x,
    &obj->getTransform().position.y,
    startX, startY,
    100.0f, 200.0f,
    duration,
    EaseType::EaseOutQuad
);
```

## Easing Function Mapping

Timeline `EasingType` maps to engine_core `EaseType`:

| Timeline EasingType | engine_core EaseType |
|-------------------|-------------------|
| `Linear` | `EaseType::Linear` |
| `EaseIn` | `EaseType::EaseInQuad` |
| `EaseOut` | `EaseType::EaseOutQuad` |
| `EaseInOut` | `EaseType::EaseInOutQuad` |
| `EaseInCubic` | `EaseType::EaseInCubic` |
| `EaseOutCubic` | `EaseType::EaseOutCubic` |
| `EaseInOutCubic` | `EaseType::EaseInOutCubic` |
| `EaseInElastic` | `EaseType::EaseInElastic` |
| `EaseOutElastic` | `EaseType::EaseOutElastic` |
| `EaseInBounce` | `EaseType::EaseInBounce` |
| `EaseOutBounce` | `EaseType::EaseOutBounce` |
| `Custom` | Bezier curve interpolation (future) |

## Animation Clip Creation

### From Timeline to Animation Clip

1. **Collect Keyframes**: Gather all keyframes from a Timeline track
2. **Sort by Frame**: Ensure keyframes are in chronological order
3. **Create Segments**: Build tween for each keyframe→keyframe segment
4. **Build Timeline**: Sequence tweens into an `AnimationTimeline`
5. **Register**: Add to `AnimationManager` for playback

### Example Code

```cpp
// Create animation timeline from track
auto createAnimationFromTrack(const TimelineTrack* track,
                              SceneObject* targetObject)
    -> std::unique_ptr<AnimationTimeline>
{
    auto timeline = std::make_unique<AnimationTimeline>();

    for (size_t i = 0; i < track->keyframes.size() - 1; ++i) {
        const auto& kf1 = track->keyframes[i];
        const auto& kf2 = track->keyframes[i + 1];

        f32 duration = (kf2.frame - kf1.frame) / 30.0f; // Convert frames to seconds
        auto easing = mapEasingType(kf1.easing);

        // Create appropriate tween based on property type
        auto tween = createTweenForProperty(
            track->type,
            targetObject,
            kf1.value,
            kf2.value,
            duration,
            easing
        );

        timeline->append(std::move(tween));
    }

    return timeline;
}
```

## Preview Playback

### Timeline Playback Flow

1. User presses Play in Timeline panel
2. Timeline emits `frameChanged(int frame)` signal
3. Animation Adapter calculates interpolated values for current frame
4. Scene View updates object properties based on interpolated values
5. Scene View renders updated scene

### Frame Update Synchronization

```cpp
// In NMTimelinePanel
void NMTimelinePanel::setCurrentFrame(int frame) {
    m_currentFrame = frame;
    emit frameChanged(frame);
}

// In Animation Adapter
void NMAnimationAdapter::onTimelineFrameChanged(int frame) {
    f64 time = static_cast<f64>(frame) / m_fps;

    // Update all active animations to this time
    for (auto& [trackId, animation] : m_activeAnimations) {
        animation->seekToTime(time);
        applyAnimationToScene(trackId, animation.get());
    }

    emit sceneUpdated();
}

// In Scene View
void NMSceneViewPanel::onAnimationSceneUpdated() {
    // Scene objects are already updated by adapter
    // Just trigger a redraw
    m_view->viewport()->update();
}
```

## RAII and Memory Management

### Ownership Model

- **Timeline Panel**: Owns `TimelineTrack*` objects
- **Animation Adapter**: Owns `std::unique_ptr<AnimationTimeline>` for preview
- **Engine Core**: Owns `std::unique_ptr<Tween>` within timelines
- **Scene Manager**: Owns `std::unique_ptr<SceneObject>` scene objects

### Lifecycle

1. Timeline tracks are created/destroyed by user edits
2. Adapter creates temporary animation objects for preview
3. When preview stops, adapter cleans up animation objects
4. Scene objects persist throughout editor session

### Resource Cleanup

```cpp
class NMAnimationAdapter {
public:
    ~NMAnimationAdapter() {
        stopAllAnimations(); // Ensures clean shutdown
    }

    void stopAllAnimations() {
        m_activeAnimations.clear(); // unique_ptr auto-deletes
        m_animationManager.stopAll();
    }

private:
    std::unordered_map<QString, std::unique_ptr<AnimationTimeline>> m_activeAnimations;
    scene::AnimationManager m_animationManager;
};
```

## Signal/Slot Architecture

### Timeline → Adapter

- `frameChanged(int)` - Playback position changed
- `keyframeModified(QString, int)` - Keyframe data changed
- `keyframeAdded(QString, int)` - New keyframe created
- `keyframeDeleted(QString, int)` - Keyframe removed
- `playbackStateChanged(bool)` - Play/pause toggled

### Adapter → Scene View

- `sceneUpdated()` - Animation state changed, redraw needed
- `animationStarted()` - Preview playback began
- `animationStopped()` - Preview playback ended

## Future Enhancements

1. **Curve Editor Integration**: Custom Bezier curves for easing
2. **Layer-based Animation**: Animate entire layers
3. **Animation Export**: Save animations to external files
4. **Animation Events**: Trigger script callbacks at keyframes
5. **Animation Blending**: Smooth transitions between animations
6. **IK Constraints**: Inverse kinematics for character animations
