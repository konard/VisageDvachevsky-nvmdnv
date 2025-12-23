/**
 * @file animation_preview_example.cpp
 * @brief Example demonstrating Timeline → engine_core animation integration
 *
 * This example shows how to:
 * 1. Create animation tracks in Timeline
 * 2. Convert them to engine_core animations
 * 3. Preview animations in SceneView
 * 4. Synchronize playback
 */

#include "NovelMind/scene/animation.hpp"
#include "NovelMind/scene/scene_manager.hpp"
#include "NovelMind/scene/scene_object.hpp"
#include <iostream>
#include <memory>

using namespace NovelMind;
using namespace NovelMind::scene;

/**
 * @brief Simple character sprite for demonstration
 */
class DemoCharacter : public SceneObject {
public:
  explicit DemoCharacter(const std::string& id) : SceneObject(id) {}

  void render(renderer::IRenderer& renderer) override {
    // In real implementation, would render sprite at transform position
    (void)renderer;
  }

  void printState() const {
    std::cout << "Character '" << m_id << "' state:\n";
    std::cout << "  Position: (" << m_transform.position.x << ", "
              << m_transform.position.y << ")\n";
    std::cout << "  Scale: (" << m_transform.scale.x << ", "
              << m_transform.scale.y << ")\n";
    std::cout << "  Rotation: " << m_transform.rotation << " degrees\n";
    std::cout << "  Alpha: " << m_alpha << "\n";
  }
};

/**
 * @brief Simulates a Timeline track with keyframes
 */
struct SimulatedTimelineTrack {
  std::string name;
  std::string targetObjectId;
  std::string targetProperty;

  struct Keyframe {
    int frame;      // Frame number (at 30 FPS)
    f32 value;      // Value at this frame
    EaseType easing;
  };

  std::vector<Keyframe> keyframes;
};

/**
 * @brief Convert simulated timeline track to engine_core animation
 */
std::unique_ptr<AnimationTimeline>
convertTrackToAnimation(const SimulatedTimelineTrack& track, f32* targetPtr) {
  if (track.keyframes.size() < 2) {
    return nullptr;
  }

  const int FPS = 30;
  auto timeline = std::make_unique<AnimationTimeline>();

  // Build tween for each keyframe segment
  for (size_t i = 0; i < track.keyframes.size() - 1; ++i) {
    const auto& kf1 = track.keyframes[i];
    const auto& kf2 = track.keyframes[i + 1];

    f32 duration = static_cast<f32>(kf2.frame - kf1.frame) / FPS;

    auto tween = std::make_unique<FloatTween>(
        targetPtr,
        kf1.value,
        kf2.value,
        duration,
        kf1.easing
    );

    timeline->append(std::move(tween));
  }

  return timeline;
}

int main() {
  std::cout << "=== Animation Integration Example ===\n\n";

  // =========================================================================
  // 1. Create Scene and Objects
  // =========================================================================

  SceneManager sceneManager;

  auto hero = std::make_unique<DemoCharacter>("hero");
  hero->setPosition(0.0f, 300.0f);  // Start at left side of screen
  hero->setAlpha(0.0f);             // Start invisible

  DemoCharacter* heroPtr = hero.get();
  sceneManager.addToLayer(LayerType::Characters, std::move(hero));

  std::cout << "Initial state:\n";
  heroPtr->printState();
  std::cout << "\n";

  // =========================================================================
  // 2. Define Animation Tracks (simulating Timeline editor)
  // =========================================================================

  // Track 1: Fade in character (alpha 0 → 1 over 1 second)
  SimulatedTimelineTrack fadeInTrack;
  fadeInTrack.name = "hero_fade_in";
  fadeInTrack.targetObjectId = "hero";
  fadeInTrack.targetProperty = "alpha";
  fadeInTrack.keyframes = {
      {0, 0.0f, EaseType::Linear},
      {30, 1.0f, EaseType::EaseInQuad}  // Frame 30 = 1 second at 30 FPS
  };

  // Track 2: Move character across screen (X: 0 → 640 over 2 seconds)
  SimulatedTimelineTrack moveTrack;
  moveTrack.name = "hero_move";
  moveTrack.targetObjectId = "hero";
  moveTrack.targetProperty = "positionX";
  moveTrack.keyframes = {
      {30, 0.0f, EaseType::Linear},     // Start at frame 30 (after fade in)
      {90, 640.0f, EaseType::EaseOutQuad}  // Frame 90 = 3 seconds total
  };

  // Track 3: Scale character (bounce effect)
  SimulatedTimelineTrack scaleTrack;
  scaleTrack.name = "hero_scale";
  scaleTrack.targetObjectId = "hero";
  scaleTrack.targetProperty = "scaleX";
  scaleTrack.keyframes = {
      {0, 1.0f, EaseType::Linear},
      {15, 1.2f, EaseType::EaseInOutBounce},
      {30, 1.0f, EaseType::EaseInOutBounce}
  };

  std::cout << "Defined animation tracks:\n";
  std::cout << "  1. " << fadeInTrack.name << ": alpha 0→1 (frames 0-30)\n";
  std::cout << "  2. " << moveTrack.name << ": positionX 0→640 (frames 30-90)\n";
  std::cout << "  3. " << scaleTrack.name << ": scaleX 1→1.2→1 (frames 0-30)\n";
  std::cout << "\n";

  // =========================================================================
  // 3. Convert Tracks to engine_core Animations
  // =========================================================================

  // Prepare property storage (in real adapter, this would be managed)
  f32 animatedAlpha = 0.0f;
  f32 animatedPosX = 0.0f;
  f32 animatedScaleX = 1.0f;

  auto alphaAnimation = convertTrackToAnimation(fadeInTrack, &animatedAlpha);
  auto posXAnimation = convertTrackToAnimation(moveTrack, &animatedPosX);
  auto scaleXAnimation = convertTrackToAnimation(scaleTrack, &animatedScaleX);

  std::cout << "Converted tracks to engine_core animations\n\n";

  // =========================================================================
  // 4. Preview Playback Simulation
  // =========================================================================

  std::cout << "=== Playback Preview ===\n\n";

  const f64 TIME_STEP = 0.5;  // Update every 0.5 seconds
  const f64 TOTAL_TIME = 3.0; // 3 seconds total (90 frames)

  // Start all animations
  if (alphaAnimation) alphaAnimation->start();
  if (posXAnimation) posXAnimation->start();
  if (scaleXAnimation) scaleXAnimation->start();

  // Simulate frame-by-frame preview
  for (f64 time = 0.0; time <= TOTAL_TIME; time += TIME_STEP) {
    // Update all animations
    bool alphaRunning = alphaAnimation && alphaAnimation->update(TIME_STEP);
    bool posXRunning = posXAnimation && posXAnimation->update(TIME_STEP);
    bool scaleXRunning = scaleXAnimation && scaleXAnimation->update(TIME_STEP);

    // Apply animated values to scene object
    heroPtr->setAlpha(animatedAlpha);
    heroPtr->setPosition(animatedPosX, heroPtr->getTransform().position.y);
    heroPtr->setScale(animatedScaleX, heroPtr->getTransform().scale.y);

    // Display current state (simulating SceneView rendering)
    int currentFrame = static_cast<int>(time * 30);
    std::cout << "Frame " << currentFrame << " (t=" << time << "s):\n";
    heroPtr->printState();
    std::cout << "  Running: alpha=" << (alphaRunning ? "yes" : "no")
              << ", posX=" << (posXRunning ? "yes" : "no")
              << ", scaleX=" << (scaleXRunning ? "yes" : "no") << "\n\n";
  }

  // =========================================================================
  // 5. Using AnimationManager for Multiple Objects
  // =========================================================================

  std::cout << "=== Animation Manager Example ===\n\n";

  AnimationManager animManager;

  // Reset character
  heroPtr->setPosition(0.0f, 300.0f);
  heroPtr->setAlpha(0.0f);
  heroPtr->setScale(1.0f, 1.0f);

  // Create fresh animations
  f32 alpha2 = 0.0f;
  f32 posX2 = 0.0f;

  animManager.add("hero_fade",
                  std::make_unique<FloatTween>(&alpha2, 0.0f, 1.0f, 1.0f, EaseType::Linear));
  animManager.add("hero_move",
                  std::make_unique<FloatTween>(&posX2, 0.0f, 640.0f, 2.0f, EaseType::EaseOutQuad));

  std::cout << "Created " << animManager.count() << " managed animations\n";

  // Update manager (would be called in update loop)
  for (int i = 0; i < 10; ++i) {
    animManager.update(0.25);  // 0.25s per step

    // Apply values
    heroPtr->setAlpha(alpha2);
    heroPtr->setPosition(posX2, heroPtr->getTransform().position.y);

    std::cout << "Step " << i << ": "
              << "alpha=" << alpha2 << ", "
              << "posX=" << posX2 << ", "
              << "active animations=" << animManager.count() << "\n";
  }

  std::cout << "\n=== Example Complete ===\n";

  return 0;
}
