#include "NovelMind/editor/editor_runtime_host.hpp"
#include "NovelMind/editor/scene_document.hpp"
#include "editor_runtime_host_detail.hpp"

#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <variant>

namespace NovelMind::editor {

namespace fs = std::filesystem;

// ============================================================================
// Private Helpers
// ============================================================================

Result<void> EditorRuntimeHost::compileProject() {
  try {
    fs::path scriptsPath(m_project.scriptsPath);

    if (!fs::exists(scriptsPath)) {
      return Result<void>::error("Scripts path does not exist: " +
                                 m_project.scriptsPath);
    }

    // Collect all script files
    std::string allScripts;
    m_sceneNames.clear();

    for (const auto &entry : fs::recursive_directory_iterator(scriptsPath)) {
      if (entry.is_regular_file()) {
        std::string ext = entry.path().extension().string();
        if (ext == ".nms" || ext == ".nm") {
          std::ifstream file(entry.path());
          if (file) {
            std::string content;
            if (!detail::readFileToString(file, content)) {
              return Result<void>::error("Failed to read script file: " +
                                         entry.path().string());
            }
            allScripts += "\n// File: " + entry.path().string() + "\n";
            allScripts += content;

            // Track file timestamps for hot reload
            u64 modTime = static_cast<u64>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    entry.last_write_time().time_since_epoch())
                    .count());
            m_fileTimestamps[entry.path().string()] = modTime;
          }
        }
      }
    }

    if (allScripts.empty()) {
      return Result<void>::error("No script files found in: " +
                                 m_project.scriptsPath);
    }

    // Lexer
    scripting::Lexer lexer;
    auto tokensResult = lexer.tokenize(allScripts);

    if (!tokensResult.isOk()) {
      return Result<void>::error("Lexer error: " + tokensResult.error());
    }

    // Parser
    scripting::Parser parser;
    auto parseResult = parser.parse(tokensResult.value());

    if (!parseResult.isOk()) {
      return Result<void>::error("Parse error: " + parseResult.error());
    }

    m_program =
        std::make_unique<scripting::Program>(std::move(parseResult.value()));

    // Extract scene names
    for (const auto &scene : m_program->scenes) {
      m_sceneNames.push_back(scene.name);
    }

    // Validator
    scripting::Validator validator;
    auto validationResult = validator.validate(*m_program);

    if (validationResult.hasErrors()) {
      std::string errorMsg = "Validation errors:\n";
      for (const auto &err : validationResult.errors.errors()) {
        errorMsg += "  " + err.format() + "\n";
      }
      return Result<void>::error(errorMsg);
    }

    // Compiler
    scripting::Compiler compiler;
    auto compileResult = compiler.compile(*m_program);

    if (!compileResult.isOk()) {
      return Result<void>::error("Compilation error: " + compileResult.error());
    }

    m_compiledScript = std::make_unique<scripting::CompiledScript>(
        std::move(compileResult.value()));

    return Result<void>::ok();
  } catch (const std::exception &e) {
    return Result<void>::error(std::string("Exception during compilation: ") +
                               e.what());
  }
}

Result<void> EditorRuntimeHost::initializeRuntime() {
  // Create scene graph
  m_sceneGraph = std::make_unique<scene::SceneGraph>();

  // Create resource manager for project assets
  m_resourceManager = std::make_unique<resource::ResourceManager>();
  if (!m_project.assetsPath.empty()) {
    m_resourceManager->setBasePath(m_project.assetsPath);
  } else {
    m_resourceManager->setBasePath(m_project.path);
  }
  m_sceneGraph->setResourceManager(m_resourceManager.get());

  // Create animation manager
  m_animationManager = std::make_unique<scene::AnimationManager>();

  // Create audio manager (dev mode - unencrypted)
  m_audioManager = std::make_unique<audio::AudioManager>();
  m_audioManager->setDataProvider([this](const std::string &id) {
    if (!m_resourceManager) {
      return Result<std::vector<u8>>::error("Resource manager unavailable");
    }
    return m_resourceManager->readData(id);
  });
  m_audioManager->initialize();

  // Create save manager
  m_saveManager = std::make_unique<save::SaveManager>();
  fs::path savePath = fs::path(m_project.path) / "Saves";
  std::error_code ec;
  fs::create_directories(savePath, ec);
  m_saveManager->setSavePath(savePath.string());

  // Create script runtime
  m_scriptRuntime = std::make_unique<scripting::ScriptRuntime>();

  // Connect runtime to scene components
  // Note: In a full implementation, we would also connect:
  // - SceneManager
  // - DialogueBox
  // - ChoiceMenu
  // - AudioManager

  // Set up event callback
  m_scriptRuntime->setEventCallback(
      [this](const scripting::ScriptEvent &event) { onRuntimeEvent(event); });

  return Result<void>::ok();
}

void EditorRuntimeHost::resetRuntime() {
  if (m_sceneGraph) {
    m_sceneGraph->clear();
  }

  if (m_animationManager) {
    m_animationManager->stopAll();
  }

  m_singleStepping = false;
  m_targetInstructionPointer = 0;
}

bool EditorRuntimeHost::checkBreakpoint(
    const scripting::SourceLocation &location) {
  for (const auto &bp : m_breakpoints) {
    if (bp.enabled && bp.line == location.line) {
      // Check file match (simplified)
      if (bp.condition.empty()) {
        fireBreakpointHit(bp);
        return true;
      }
      // Would evaluate conditional breakpoint here
    }
  }
  return false;
}

void EditorRuntimeHost::fireStateChanged(EditorRuntimeState newState) {
  if (m_onStateChanged) {
    m_onStateChanged(newState);
  }
}

void EditorRuntimeHost::fireBreakpointHit(const Breakpoint &bp) {
  if (m_onBreakpointHit) {
    m_state = EditorRuntimeState::Paused;
    auto stack = getScriptCallStack();
    m_onBreakpointHit(bp, stack);
  }
}

void EditorRuntimeHost::onRuntimeEvent(const scripting::ScriptEvent &event) {
  switch (event.type) {
  case scripting::ScriptEventType::SceneChange:
    // Update scene graph with new scene ID
    if (m_sceneGraph) {
      m_sceneGraph->setSceneId(event.name);
    }
    applySceneDocument(event.name);
    if (m_onSceneChanged) {
      m_onSceneChanged(event.name);
    }
    break;

  case scripting::ScriptEventType::BackgroundChanged:
    if (m_sceneGraph) {
      m_sceneGraph->showBackground(event.name);
    }
    break;

  case scripting::ScriptEventType::CharacterShow: {
    if (m_sceneGraph) {
      // event.value may hold desired slot (int). Default to Center.
      scene::CharacterObject::Position pos =
          scene::CharacterObject::Position::Center;
      if (std::holds_alternative<i32>(event.value)) {
        i32 posCode = std::get<i32>(event.value);
        switch (posCode) {
        case 0:
          pos = scene::CharacterObject::Position::Left;
          break;
        case 1:
          pos = scene::CharacterObject::Position::Center;
          break;
        case 2:
          pos = scene::CharacterObject::Position::Right;
          break;
        default:
          pos = scene::CharacterObject::Position::Custom;
          break;
        }
      }
      m_sceneGraph->showCharacter(event.name, event.name, pos);
    }
    break;
  }

  case scripting::ScriptEventType::CharacterHide:
    if (m_sceneGraph) {
      m_sceneGraph->hideCharacter(event.name);
    }
    break;

  case scripting::ScriptEventType::DialogueStart: {
    const std::string speaker = event.name;
    const std::string text = scripting::asString(event.value);
    if (m_sceneGraph) {
      m_sceneGraph->showDialogue(speaker, text);
    }
    if (m_onDialogueChanged) {
      m_onDialogueChanged(speaker, text);
    }
    break;
  }

  case scripting::ScriptEventType::DialogueComplete:
    if (m_sceneGraph) {
      // Keep dialogue box visible but mark typewriter complete
      m_sceneGraph->hideDialogue();
    }
    if (m_onDialogueChanged) {
      m_onDialogueChanged("", "");
    }
    break;

  case scripting::ScriptEventType::ChoiceStart: {
    if (m_scriptRuntime && m_sceneGraph) {
      const auto &choices = m_scriptRuntime->getCurrentChoices();
      std::vector<scene::ChoiceUIObject::ChoiceOption> options;
      options.reserve(choices.size());
      for (size_t i = 0; i < choices.size(); ++i) {
        scene::ChoiceUIObject::ChoiceOption opt;
        opt.id = "choice_" + std::to_string(i);
        opt.text = choices[i];
        options.push_back(opt);
      }
      m_sceneGraph->showChoices(options);
      if (m_onChoicesChanged) {
        m_onChoicesChanged(choices);
      }
    }
    break;
  }

  case scripting::ScriptEventType::ChoiceSelected:
    if (m_sceneGraph) {
      m_sceneGraph->hideChoices();
    }
    if (m_onChoicesChanged) {
      m_onChoicesChanged({});
    }
    break;

  case scripting::ScriptEventType::VariableChanged:
    if (m_onVariableChanged) {
      m_onVariableChanged(event.name, event.value);
    }
    break;

  default:
    break;
  }
}

void EditorRuntimeHost::applySceneDocument(const std::string &sceneId) {
  if (!m_sceneGraph || sceneId.empty()) {
    return;
  }

  if (m_project.scenesPath.empty()) {
    return;
  }

  namespace fs = std::filesystem;
  const fs::path scenePath =
      fs::path(m_project.scenesPath) / (sceneId + ".nmscene");
  const auto result = loadSceneDocument(scenePath.string());
  if (!result.isOk()) {
    return;
  }

  m_sceneGraph->clear();
  m_sceneGraph->setSceneId(sceneId);

  const auto &doc = result.value();
  for (const auto &item : doc.objects) {
    scene::SceneObjectState state;
    state.id = item.id;
    state.x = item.x;
    state.y = item.y;
    state.rotation = item.rotation;
    state.scaleX = item.scaleX;
    state.scaleY = item.scaleY;
    state.alpha = item.alpha;
    state.visible = item.visible;
    state.zOrder = item.zOrder;
    state.properties = item.properties;

    const std::string &type = item.type;
    if (type == "Background") {
      state.type = scene::SceneObjectType::Background;
      auto obj = std::make_unique<scene::BackgroundObject>(state.id);
      obj->loadState(state);
      m_sceneGraph->addToLayer(scene::LayerType::Background, std::move(obj));
    } else if (type == "Character") {
      state.type = scene::SceneObjectType::Character;
      std::string characterId = state.id;
      auto it = state.properties.find("characterId");
      if (it != state.properties.end() && !it->second.empty()) {
        characterId = it->second;
      }
      auto obj = std::make_unique<scene::CharacterObject>(state.id, characterId);
      obj->loadState(state);
      m_sceneGraph->addToLayer(scene::LayerType::Characters, std::move(obj));
    } else if (type == "Effect") {
      state.type = scene::SceneObjectType::EffectOverlay;
      auto obj = std::make_unique<scene::EffectOverlayObject>(state.id);
      obj->loadState(state);
      m_sceneGraph->addToLayer(scene::LayerType::Effects, std::move(obj));
    } else {
      continue;
    }
  }
}


} // namespace NovelMind::editor
