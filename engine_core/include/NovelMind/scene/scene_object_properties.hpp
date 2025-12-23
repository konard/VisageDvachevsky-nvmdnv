#pragma once

/**
 * @file scene_object_properties.hpp
 * @brief Property registration for scene objects
 *
 * Provides runtime property introspection for scene objects, enabling:
 * - Inspector panel to display and edit properties
 * - Type-safe property access without raw pointers
 * - RAII-compliant property binding
 *
 * This integrates the PropertySystem with SceneGraph objects to fulfill
 * the requirements of Issue #15: Inspector → real property editing.
 */

#include "NovelMind/core/property_system.hpp"
#include "NovelMind/scene/scene_graph.hpp"

namespace NovelMind::scene {

/**
 * @brief Register all scene object types with the property system
 *
 * This should be called once during engine initialization.
 * Registers property accessors for:
 * - SceneObjectBase (common properties)
 * - BackgroundObject
 * - CharacterObject
 * - DialogueUIObject
 * - EffectOverlayObject
 * - And other scene object types
 */
void registerSceneObjectProperties();

/**
 * @brief Register properties for SceneObjectBase
 */
void registerSceneObjectBaseProperties();

/**
 * @brief Register properties for BackgroundObject
 */
void registerBackgroundObjectProperties();

/**
 * @brief Register properties for CharacterObject
 */
void registerCharacterObjectProperties();

/**
 * @brief Register properties for DialogueUIObject
 */
void registerDialogueUIObjectProperties();

/**
 * @brief Register properties for EffectOverlayObject
 */
void registerEffectOverlayObjectProperties();

} // namespace NovelMind::scene
