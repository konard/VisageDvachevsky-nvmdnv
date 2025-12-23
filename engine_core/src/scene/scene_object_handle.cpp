/**
 * @file scene_object_handle.cpp
 * @brief Implementation of RAII-safe scene object handle
 */

#include "NovelMind/scene/scene_object_handle.hpp"
#include "NovelMind/scene/scene_graph.hpp"

namespace NovelMind::scene {

SceneObjectHandle::SceneObjectHandle(SceneGraph *sceneGraph,
                                     const std::string &objectId)
    : m_sceneGraph(sceneGraph), m_objectId(objectId) {}

bool SceneObjectHandle::isValid() const {
  return m_sceneGraph != nullptr && get() != nullptr;
}

SceneObjectBase *SceneObjectHandle::get() const {
  if (!m_sceneGraph || m_objectId.empty()) {
    return nullptr;
  }
  return m_sceneGraph->findObject(m_objectId);
}

bool SceneObjectHandle::withObject(
    std::function<void(SceneObjectBase *)> fn) const {
  if (auto *obj = get()) {
    fn(obj);
    return true;
  }
  return false;
}

void SceneObjectHandle::reset() {
  m_sceneGraph = nullptr;
  m_objectId.clear();
}

} // namespace NovelMind::scene
