/**
 * @file inspector_extended_types_demo.cpp
 * @brief Demonstration of extended inspector property types (Vector2, Vector3, Curve)
 *
 * This example shows how the inspector panel now supports:
 * - Vector2: Two spinboxes for X and Y components
 * - Vector3: Three spinboxes for X, Y, and Z components
 * - Curve: Button that opens curve editor dialog
 *
 * All implementations include:
 * - Proper debouncing to prevent UI spam
 * - Type-safe value binding through propertyValueChanged signal
 * - Consistent styling matching the existing editor theme
 */

#include "NovelMind/core/property_system.hpp"
#include <iostream>

using namespace NovelMind;

/**
 * @brief Example class demonstrating the use of Vector2, Vector3, and Curve properties
 */
class DemoObject {
public:
  DemoObject() = default;

  // Vector2 property - useful for 2D positions, sizes, etc.
  Vector2 m_position{100.0f, 200.0f};
  Vector2 getPosition() const { return m_position; }
  void setPosition(const Vector2 &pos) {
    m_position = pos;
    std::cout << "Position changed to: (" << pos.x << ", " << pos.y << ")\n";
  }

  // Vector3 property - useful for 3D positions, RGB colors, etc.
  Vector3 m_velocity{1.0f, 0.0f, 0.0f};
  Vector3 getVelocity() const { return m_velocity; }
  void setVelocity(const Vector3 &vel) {
    m_velocity = vel;
    std::cout << "Velocity changed to: (" << vel.x << ", " << vel.y << ", "
              << vel.z << ")\n";
  }

  // Curve property - useful for animation curves, easing functions, etc.
  CurveRef m_animationCurve{"default_ease_in_out"};
  CurveRef getAnimationCurve() const { return m_animationCurve; }
  void setAnimationCurve(const CurveRef &curve) {
    m_animationCurve = curve;
    std::cout << "Animation curve changed to: " << curve.curveId << "\n";
  }
};

/**
 * @brief Register DemoObject properties with the property system
 */
void registerDemoObjectProperties() {
  TypeInfoBuilder<DemoObject>("DemoObject")
      .property<Vector2>(
          PropertyMeta{"position", "Position", PropertyType::Vector2},
          [](const DemoObject &obj) { return obj.getPosition(); },
          [](DemoObject &obj, const Vector2 &val) { obj.setPosition(val); })
      .property<Vector3>(
          PropertyMeta{"velocity", "Velocity", PropertyType::Vector3},
          [](const DemoObject &obj) { return obj.getVelocity(); },
          [](DemoObject &obj, const Vector3 &val) { obj.setVelocity(val); })
      .property<CurveRef>(
          PropertyMeta{"animationCurve", "Animation Curve",
                       PropertyType::CurveRef},
          [](const DemoObject &obj) { return obj.getAnimationCurve(); },
          [](DemoObject &obj, const CurveRef &val) {
            obj.setAnimationCurve(val);
          })
      .build();
}

/**
 * @brief Demonstrate property system usage
 */
int main() {
  std::cout << "=== Inspector Extended Types Demo ===\n\n";

  // Register the demo object's properties
  registerDemoObjectProperties();

  // Create an instance
  DemoObject obj;

  // Get type information
  const TypeInfo *typeInfo =
      PropertyRegistry::instance().getTypeInfo<DemoObject>();
  if (!typeInfo) {
    std::cerr << "Failed to get type info\n";
    return 1;
  }

  std::cout << "Registered properties for " << typeInfo->getTypeName()
            << ":\n\n";

  // Display all properties
  for (const auto &prop : typeInfo->getProperties()) {
    const auto &meta = prop->getMeta();
    std::cout << "  - " << meta.displayName << " (" << meta.name
              << "): " << PropertyUtils::getTypeName(meta.type) << "\n";

    // Get current value
    PropertyValue currentValue = prop->getValue(&obj);
    std::cout << "    Current: " << PropertyUtils::toString(currentValue)
              << "\n\n";
  }

  // Demonstrate property updates
  std::cout << "=== Testing Property Updates ===\n\n";

  // Update Vector2
  std::cout << "1. Updating Vector2 property:\n";
  if (const auto *posAccessor = typeInfo->findProperty("position")) {
    Vector2 newPos{250.0f, 350.0f};
    posAccessor->setValue(&obj, PropertyValue{newPos});
  }

  // Update Vector3
  std::cout << "\n2. Updating Vector3 property:\n";
  if (const auto *velAccessor = typeInfo->findProperty("velocity")) {
    Vector3 newVel{2.5f, -1.0f, 0.5f};
    velAccessor->setValue(&obj, PropertyValue{newVel});
  }

  // Update Curve
  std::cout << "\n3. Updating Curve property:\n";
  if (const auto *curveAccessor = typeInfo->findProperty("animationCurve")) {
    CurveRef newCurve{"custom_bounce_curve"};
    curveAccessor->setValue(&obj, PropertyValue{newCurve});
  }

  std::cout << "\n=== Demo Complete ===\n";
  std::cout << "\nIn the Inspector Panel UI:\n";
  std::cout << "  - Vector2 shows: [X: spinbox] [Y: spinbox]\n";
  std::cout << "  - Vector3 shows: [X: spinbox] [Y: spinbox] [Z: spinbox]\n";
  std::cout << "  - Curve shows: [Edit Curve... button]\n";
  std::cout << "  - All changes are debounced (150ms) to prevent spam\n";
  std::cout << "  - Values are validated and type-checked before application\n";

  return 0;
}
