#include "NovelMind/editor/project_manager.hpp"
#include "NovelMind/editor/scene_document.hpp"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace NovelMind::editor {

// Static instance
std::unique_ptr<ProjectManager> ProjectManager::s_instance = nullptr;

ProjectManager::ProjectManager() {
  m_recentProjects.reserve(m_maxRecentProjects);
}

ProjectManager::~ProjectManager() {
  if (m_state == ProjectState::Open) {
    closeProject(true);
  }
}

ProjectManager &ProjectManager::instance() {
  if (!s_instance) {
    s_instance = std::make_unique<ProjectManager>();
  }
  return *s_instance;
}

// ============================================================================
// Project Lifecycle
// ============================================================================

Result<void> ProjectManager::createProject(const std::string &path,
                                           const std::string &name,
                                           const std::string &templateName) {
  if (m_state != ProjectState::Closed) {
    return Result<void>::error("A project is already open. Close it first.");
  }

  namespace fs = std::filesystem;

  // Check if path exists
  if (fs::exists(path)) {
    // Check if it's empty
    if (!fs::is_empty(path)) {
      return Result<void>::error("Directory is not empty: " + path);
    }
  } else {
    // Create directory
    std::error_code ec;
    if (!fs::create_directories(path, ec)) {
      return Result<void>::error("Failed to create directory: " + ec.message());
    }
  }

  m_projectPath = fs::absolute(path).string();
  m_metadata = ProjectMetadata();
  m_metadata.name = name;
  m_metadata.createdAt = static_cast<u64>(std::time(nullptr));
  m_metadata.modifiedAt = m_metadata.createdAt;
  m_metadata.lastOpenedAt = m_metadata.createdAt;
  m_metadata.engineVersion = "0.2.0";

  // Create folder structure
  auto result = createFolderStructure();
  if (result.isError()) {
    return result;
  }

  // Create from template if specified
  if (templateName != "empty" && !templateName.empty()) {
    result = createProjectFromTemplate(templateName);
    if (result.isError()) {
      // Clean up on failure
      fs::remove_all(m_projectPath);
      return result;
    }
  }

  // Save project file
  result = saveProjectFile();
  if (result.isError()) {
    return result;
  }

  m_state = ProjectState::Open;
  m_modified = false;

  m_assetDatabase.initialize(m_projectPath);

  addToRecentProjects(m_projectPath);
  notifyProjectCreated();

  return Result<void>::ok();
}

Result<void> ProjectManager::openProject(const std::string &path) {
  if (m_state != ProjectState::Closed) {
    auto closeResult = closeProject();
    if (closeResult.isError()) {
      return closeResult;
    }
  }

  m_state = ProjectState::Opening;

  namespace fs = std::filesystem;

  std::string projectFilePath = path;

  // If path is a directory, look for project.json
  if (fs::is_directory(path)) {
    projectFilePath = (fs::path(path) / "project.json").string();
  }

  if (!fs::exists(projectFilePath)) {
    m_state = ProjectState::Closed;
    return Result<void>::error("Project file not found: " + projectFilePath);
  }

  auto result = loadProjectFile(projectFilePath);
  if (result.isError()) {
    m_state = ProjectState::Closed;
    return result;
  }

  m_projectPath = fs::path(projectFilePath).parent_path().string();
  m_metadata.lastOpenedAt = static_cast<u64>(std::time(nullptr));

  // Verify folder structure
  if (!verifyFolderStructure()) {
    // Try to repair
    auto repairResult = createFolderStructure();
    if (repairResult.isError()) {
      m_state = ProjectState::Closed;
      return Result<void>::error(
          "Project folder structure is invalid and could not be "
          "repaired");
    }
  }

  m_state = ProjectState::Open;
  m_modified = false;
  m_timeSinceLastSave = 0.0;

  m_assetDatabase.initialize(m_projectPath);

  addToRecentProjects(m_projectPath);
  notifyProjectOpened();

  return Result<void>::ok();
}

Result<void> ProjectManager::saveProject() {
  if (m_state != ProjectState::Open) {
    return Result<void>::error("No project is open");
  }

  m_state = ProjectState::Saving;

  m_metadata.modifiedAt = static_cast<u64>(std::time(nullptr));

  auto result = saveProjectFile();
  if (result.isError()) {
    m_state = ProjectState::Open;
    return result;
  }

  m_state = ProjectState::Open;
  m_modified = false;
  m_timeSinceLastSave = 0.0;

  notifyProjectSaved();

  return Result<void>::ok();
}

Result<void> ProjectManager::saveProjectAs(const std::string &path) {
  if (m_state != ProjectState::Open) {
    return Result<void>::error("No project is open");
  }

  namespace fs = std::filesystem;

  // Copy current project to new location
  std::error_code ec;
  fs::copy(m_projectPath, path,
           fs::copy_options::recursive | fs::copy_options::overwrite_existing,
           ec);
  if (ec) {
    return Result<void>::error("Failed to copy project: " + ec.message());
  }

  m_projectPath = fs::absolute(path).string();

  return saveProject();
}

Result<void> ProjectManager::closeProject(bool force) {
  if (m_state == ProjectState::Closed) {
    return Result<void>::ok();
  }

  if (!force && m_modified) {
    if (m_onUnsavedChangesPrompt) {
      auto promptResult = m_onUnsavedChangesPrompt();
      if (!promptResult.has_value()) {
        // User cancelled
        return Result<void>::error("Operation cancelled by user");
      }
      if (promptResult.value()) {
        // User wants to save
        auto saveResult = saveProject();
        if (saveResult.isError()) {
          return saveResult;
        }
      }
    }
  }

  m_state = ProjectState::Closing;

  // Clear project state
  m_assetDatabase.close();
  m_projectPath.clear();
  m_metadata = ProjectMetadata();
  m_modified = false;
  m_timeSinceLastSave = 0.0;

  m_state = ProjectState::Closed;

  notifyProjectClosed();

  return Result<void>::ok();
}

bool ProjectManager::hasOpenProject() const {
  return m_state == ProjectState::Open;
}

ProjectState ProjectManager::getState() const { return m_state; }

bool ProjectManager::hasUnsavedChanges() const { return m_modified; }

void ProjectManager::markModified() {
  if (!m_modified) {
    m_modified = true;
    notifyProjectModified();
  }
}

void ProjectManager::markSaved() { m_modified = false; }

// ============================================================================
// Project Information
// ============================================================================

const ProjectMetadata &ProjectManager::getMetadata() const {
  return m_metadata;
}

void ProjectManager::setMetadata(const ProjectMetadata &metadata) {
  m_metadata = metadata;
  markModified();
}

std::string ProjectManager::getProjectPath() const { return m_projectPath; }

std::string ProjectManager::getProjectName() const { return m_metadata.name; }

std::string ProjectManager::getStartScene() const {
  return m_metadata.startScene;
}

void ProjectManager::setStartScene(const std::string &sceneId) {
  if (m_metadata.startScene == sceneId) {
    return;
  }
  m_metadata.startScene = sceneId;
  markModified();
}

std::string ProjectManager::getFolderPath(ProjectFolder folder) const {
  namespace fs = std::filesystem;

  if (m_projectPath.empty()) {
    return "";
  }

  fs::path base(m_projectPath);

  switch (folder) {
  case ProjectFolder::Root:
    return m_projectPath;
  case ProjectFolder::Assets:
    return (base / "Assets").string();
  case ProjectFolder::Images:
    return (base / "Assets" / "Images").string();
  case ProjectFolder::Audio:
    return (base / "Assets" / "Audio").string();
  case ProjectFolder::Fonts:
    return (base / "Assets" / "Fonts").string();
  case ProjectFolder::Scripts:
    return (base / "Scripts").string();
  case ProjectFolder::Scenes:
    return (base / "Scenes").string();
  case ProjectFolder::Localization:
    return (base / "Localization").string();
  case ProjectFolder::Build:
    return (base / "Build").string();
  case ProjectFolder::Temp:
    return (base / ".temp").string();
  case ProjectFolder::Backup:
    return (base / ".backup").string();
  }

  return m_projectPath;
}

std::vector<std::string>
ProjectManager::getProjectFiles(const std::string &extension) const {
  namespace fs = std::filesystem;

  std::vector<std::string> files;

  if (m_projectPath.empty()) {
    return files;
  }

  for (const auto &entry : fs::recursive_directory_iterator(m_projectPath)) {
    if (entry.is_regular_file() && entry.path().extension() == extension) {
      files.push_back(entry.path().string());
    }
  }

  return files;
}

// ============================================================================
// Folder Structure
// ============================================================================

Result<void> ProjectManager::createFolderStructure() {
  namespace fs = std::filesystem;

  if (m_projectPath.empty()) {
    return Result<void>::error("No project path set");
  }

  std::error_code ec;
  fs::path base(m_projectPath);

  // Create all standard folders
  std::vector<fs::path> folders = {base / "Assets",
                                   base / "Assets" / "Images",
                                   base / "Assets" / "Audio",
                                   base / "Assets" / "Fonts",
                                   base / "Scripts",
                                   base / "Scenes",
                                   base / "Localization",
                                   base / "Build",
                                   base / ".temp",
                                   base / ".backup"};

  for (const auto &folder : folders) {
    if (!fs::exists(folder)) {
      if (!fs::create_directories(folder, ec)) {
        return Result<void>::error("Failed to create folder: " +
                                   folder.string() + " - " + ec.message());
      }
    }
  }

  return Result<void>::ok();
}

bool ProjectManager::verifyFolderStructure() const {
  namespace fs = std::filesystem;

  if (m_projectPath.empty()) {
    return false;
  }

  fs::path base(m_projectPath);

  // Check required folders
  std::vector<fs::path> required = {base / "Assets", base / "Scripts",
                                    base / "Scenes"};

  for (const auto &folder : required) {
    if (!fs::exists(folder) || !fs::is_directory(folder)) {
      return false;
    }
  }

  return true;
}

Result<void> ProjectManager::createFolder(const std::string &relativePath) {
  namespace fs = std::filesystem;

  if (m_projectPath.empty()) {
    return Result<void>::error("No project is open");
  }

  fs::path fullPath = fs::path(m_projectPath) / relativePath;

  std::error_code ec;
  if (!fs::create_directories(fullPath, ec)) {
    return Result<void>::error("Failed to create folder: " + ec.message());
  }

  return Result<void>::ok();
}

bool ProjectManager::isPathInProject(const std::string &path) const {
  namespace fs = std::filesystem;

  if (m_projectPath.empty()) {
    return false;
  }

  fs::path projectPath = fs::canonical(m_projectPath);
  fs::path targetPath = fs::canonical(path);

  auto it = std::search(targetPath.begin(), targetPath.end(),
                        projectPath.begin(), projectPath.end());

  return it == targetPath.begin();
}

std::string
ProjectManager::toRelativePath(const std::string &absolutePath) const {
  namespace fs = std::filesystem;

  if (m_projectPath.empty()) {
    return absolutePath;
  }

  return fs::relative(absolutePath, m_projectPath).string();
}

std::string
ProjectManager::toAbsolutePath(const std::string &relativePath) const {
  namespace fs = std::filesystem;

  if (m_projectPath.empty()) {
    return relativePath;
  }

  return (fs::path(m_projectPath) / relativePath).string();
}

// ============================================================================
// Recent Projects
// ============================================================================

const std::vector<RecentProject> &ProjectManager::getRecentProjects() const {
  return m_recentProjects;
}

void ProjectManager::addToRecentProjects(const std::string &path) {
  namespace fs = std::filesystem;

  // Remove existing entry for this path
  removeFromRecentProjects(path);

  // Add new entry at front
  RecentProject recent;
  recent.path = path;
  recent.lastOpened = static_cast<u64>(std::time(nullptr));
  recent.exists = fs::exists(path);

  // Try to get project name from metadata
  fs::path projectFile = fs::path(path) / "project.json";
  if (fs::exists(projectFile)) {
    // Would parse project name from file
    recent.name = fs::path(path).filename().string();
  } else {
    recent.name = fs::path(path).filename().string();
  }

  m_recentProjects.insert(m_recentProjects.begin(), recent);

  // Trim to max size
  if (m_recentProjects.size() > m_maxRecentProjects) {
    m_recentProjects.resize(m_maxRecentProjects);
  }
}

void ProjectManager::removeFromRecentProjects(const std::string &path) {
  m_recentProjects.erase(std::remove_if(m_recentProjects.begin(),
                                        m_recentProjects.end(),
                                        [&path](const RecentProject &p) {
                                          return p.path == path;
                                        }),
                         m_recentProjects.end());
}

void ProjectManager::clearRecentProjects() { m_recentProjects.clear(); }

void ProjectManager::refreshRecentProjects() {
  namespace fs = std::filesystem;

  for (auto &project : m_recentProjects) {
    project.exists = fs::exists(project.path);
  }
}

void ProjectManager::setMaxRecentProjects(size_t count) {
  m_maxRecentProjects = count;
  if (m_recentProjects.size() > m_maxRecentProjects) {
    m_recentProjects.resize(m_maxRecentProjects);
  }
}

// ============================================================================
// Auto-Save
// ============================================================================

void ProjectManager::setAutoSaveEnabled(bool enabled) {
  m_autoSaveEnabled = enabled;
}

bool ProjectManager::isAutoSaveEnabled() const { return m_autoSaveEnabled; }

void ProjectManager::setAutoSaveInterval(u32 seconds) {
  m_autoSaveIntervalSeconds = seconds;
}

u32 ProjectManager::getAutoSaveInterval() const {
  return m_autoSaveIntervalSeconds;
}

void ProjectManager::updateAutoSave(f64 deltaTime) {
  if (!m_autoSaveEnabled || m_state != ProjectState::Open || !m_modified) {
    return;
  }

  m_timeSinceLastSave += deltaTime;

  if (m_timeSinceLastSave >= static_cast<f64>(m_autoSaveIntervalSeconds)) {
    triggerAutoSave();
  }
}

void ProjectManager::triggerAutoSave() {
  if (m_state != ProjectState::Open) {
    return;
  }

  // Create backup before auto-save
  createBackup();

  // Save project
  auto result = saveProject();
  if (result.isOk()) {
    for (auto *listener : m_listeners) {
      listener->onAutoSaveTriggered();
    }
  }
}

// ============================================================================
// Validation
// ============================================================================

ProjectValidation ProjectManager::validateProject() const {
  ProjectValidation validation;

  if (m_projectPath.empty()) {
    validation.valid = false;
    validation.errors.push_back("No project is open");
    return validation;
  }

  // Check folder structure
  if (!verifyFolderStructure()) {
    validation.warnings.push_back("Some project folders are missing");
  }

  // Check for missing assets (referenced but not found)
  // This would scan scripts and scenes for asset references

  // Check for missing scripts
  // This would scan scene files for script references

  return validation;
}

bool ProjectManager::isValidProjectPath(const std::string &path) {
  namespace fs = std::filesystem;

  fs::path projectFile = fs::path(path);

  // If it's a directory, look for project.json
  if (fs::is_directory(path)) {
    projectFile = projectFile / "project.json";
  }

  return fs::exists(projectFile) && fs::is_regular_file(projectFile);
}

std::vector<std::string> ProjectManager::getAvailableTemplates() {
  return {"empty", "kinetic_novel", "branching_story"};
}

// ============================================================================
// Backup
// ============================================================================

Result<std::string> ProjectManager::createBackup() {
  namespace fs = std::filesystem;

  if (m_projectPath.empty()) {
    return Result<std::string>::error("No project is open");
  }

  fs::path backupDir = fs::path(m_projectPath) / ".backup";
  if (!fs::exists(backupDir)) {
    std::error_code ec;
    fs::create_directories(backupDir, ec);
    if (ec) {
      return Result<std::string>::error("Failed to create backup directory: " +
                                        ec.message());
    }
  }

  // Create timestamped backup name
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << "backup_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
  std::string backupName = ss.str();

  fs::path backupPath = backupDir / backupName;

  std::error_code ec;
  fs::create_directory(backupPath, ec);
  if (ec) {
    return Result<std::string>::error("Failed to create backup: " +
                                      ec.message());
  }

  // Copy project files (excluding backup and build directories)
  for (const auto &entry : fs::directory_iterator(m_projectPath)) {
    if (entry.path().filename() == ".backup" ||
        entry.path().filename() == ".temp" ||
        entry.path().filename() == "Build") {
      continue;
    }

    fs::copy(entry.path(), backupPath / entry.path().filename(),
             fs::copy_options::recursive, ec);
    if (ec) {
      // Log warning but continue
      ec.clear();
    }
  }

  // Trim old backups
  auto backups = getAvailableBackups();
  if (backups.size() > m_maxBackups) {
    for (size_t i = m_maxBackups; i < backups.size(); ++i) {
      fs::remove_all(backups[i], ec);
    }
  }

  return Result<std::string>::ok(backupPath.string());
}

Result<void> ProjectManager::restoreFromBackup(const std::string &backupPath) {
  namespace fs = std::filesystem;

  if (!fs::exists(backupPath)) {
    return Result<void>::error("Backup not found: " + backupPath);
  }

  if (m_projectPath.empty()) {
    return Result<void>::error("No project is open");
  }

  std::error_code ec;
  fs::path backupRoot(backupPath);
  fs::path projectRoot(m_projectPath);

  for (const auto &entry : fs::directory_iterator(backupRoot)) {
    fs::path target = projectRoot / entry.path().filename();
    fs::copy(entry.path(), target,
             fs::copy_options::recursive | fs::copy_options::overwrite_existing,
             ec);
    if (ec) {
      return Result<void>::error("Failed to restore backup: " + ec.message());
    }
  }

  markModified();
  return Result<void>::ok();
}

std::vector<std::string> ProjectManager::getAvailableBackups() const {
  namespace fs = std::filesystem;

  std::vector<std::string> backups;

  if (m_projectPath.empty()) {
    return backups;
  }

  fs::path backupDir = fs::path(m_projectPath) / ".backup";
  if (!fs::exists(backupDir)) {
    return backups;
  }

  for (const auto &entry : fs::directory_iterator(backupDir)) {
    if (entry.is_directory()) {
      backups.push_back(entry.path().string());
    }
  }

  // Sort by name (newest first due to timestamp naming)
  std::sort(backups.begin(), backups.end(), std::greater<>());

  return backups;
}

void ProjectManager::setMaxBackups(size_t count) { m_maxBackups = count; }

// ============================================================================
// Listeners
// ============================================================================

void ProjectManager::addListener(IProjectListener *listener) {
  if (listener && std::find(m_listeners.begin(), m_listeners.end(), listener) ==
                      m_listeners.end()) {
    m_listeners.push_back(listener);
  }
}

void ProjectManager::removeListener(IProjectListener *listener) {
  m_listeners.erase(
      std::remove(m_listeners.begin(), m_listeners.end(), listener),
      m_listeners.end());
}

void ProjectManager::setOnUnsavedChangesPrompt(
    std::function<std::optional<bool>()> callback) {
  m_onUnsavedChangesPrompt = std::move(callback);
}

// ============================================================================
// Private Methods
// ============================================================================

Result<void> ProjectManager::loadProjectFile(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return Result<void>::error("Failed to open project file: " + path);
  }

  // Simple JSON parsing (in production, use a proper JSON library)
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  // Parse name
  auto namePos = content.find("\"name\"");
  if (namePos != std::string::npos) {
    auto colonPos = content.find(':', namePos);
    auto quoteStart = content.find('"', colonPos);
    auto quoteEnd = content.find('"', quoteStart + 1);
    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
      m_metadata.name =
          content.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
    }
  }

  // Parse version
  auto versionPos = content.find("\"version\"");
  if (versionPos != std::string::npos) {
    auto colonPos = content.find(':', versionPos);
    auto quoteStart = content.find('"', colonPos);
    auto quoteEnd = content.find('"', quoteStart + 1);
    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
      m_metadata.version =
          content.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
    }
  }

  // Parse startScene
  auto startScenePos = content.find("\"startScene\"");
  if (startScenePos != std::string::npos) {
    auto colonPos = content.find(':', startScenePos);
    auto quoteStart = content.find('"', colonPos);
    auto quoteEnd = content.find('"', quoteStart + 1);
    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
      m_metadata.startScene =
          content.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
    }
  }

  return Result<void>::ok();
}

Result<void> ProjectManager::saveProjectFile() {
  namespace fs = std::filesystem;

  fs::path projectFile = fs::path(m_projectPath) / "project.json";

  std::ofstream file(projectFile);
  if (!file.is_open()) {
    return Result<void>::error("Failed to open project file for writing");
  }

  // Write JSON (in production, use a proper JSON library)
  file << "{\n";
  file << "  \"name\": \"" << m_metadata.name << "\",\n";
  file << "  \"version\": \"" << m_metadata.version << "\",\n";
  file << "  \"author\": \"" << m_metadata.author << "\",\n";
  file << "  \"description\": \"" << m_metadata.description << "\",\n";
  file << "  \"engineVersion\": \"" << m_metadata.engineVersion << "\",\n";
  file << "  \"createdAt\": " << m_metadata.createdAt << ",\n";
  file << "  \"modifiedAt\": " << m_metadata.modifiedAt << ",\n";
  file << "  \"startScene\": \"" << m_metadata.startScene << "\",\n";
  file << "  \"defaultLocale\": \"" << m_metadata.defaultLocale << "\",\n";
  file << "  \"targetResolution\": \"" << m_metadata.targetResolution << "\"\n";
  file << "}\n";

  file.close();

  return Result<void>::ok();
}

Result<void>
ProjectManager::createProjectFromTemplate(const std::string &templateName) {
  // Template would be copied from editor/templates/<templateName>/
  // For now, just create a basic main.nms script

  namespace fs = std::filesystem;

  fs::path scriptsDir = fs::path(m_projectPath) / "Scripts";
  fs::path mainScript = scriptsDir / "main.nms";
  fs::path scenesDir = fs::path(m_projectPath) / "Scenes";
  fs::create_directories(scenesDir);

  std::ofstream file(mainScript);
  if (!file.is_open()) {
    return Result<void>::error("Failed to create main script");
  }

  if (templateName == "kinetic_novel") {
    file << "// NovelMind Script - Visual Novel (Linear) Template\n";
    file << "// Add images to Assets/Images and update paths below.\n\n";
    file << "character Hero(name=\"Alex\", color=\"#ffcc00\")\n";
    file << "character Narrator(name=\"Narrator\", color=\"#cccccc\")\n\n";
    file << "scene main {\n";
    file << "    show background \"title.png\"\n";
    file << "    say \"Welcome to your visual novel!\"\n";
    file << "    say \"Replace this script with your story.\"\n";
    file << "    Hero \"Let's begin.\"\n";
    file << "}\n";
    m_metadata.startScene = "main";
  } else if (templateName == "branching_story") {
    file << "// NovelMind Script - Branching Story Template\n";
    file << "// Add images to Assets/Images and update paths below.\n\n";
    file << "character Hero(name=\"Alex\", color=\"#ffcc00\")\n\n";
    file << "scene main {\n";
    file << "    show background \"crossroads.png\"\n";
    file << "    say \"Welcome to your interactive story!\"\n";
    file << "    Hero \"Which path should we take?\"\n";
    file << "    choice {\n";
    file << "        \"Go left\" -> goto left_path\n";
    file << "        \"Go right\" -> goto right_path\n";
    file << "    }\n";
    file << "}\n\n";
    file << "scene left_path {\n";
    file << "    show background \"forest_path.png\"\n";
    file << "    say \"You chose the left path.\"\n";
    file << "    goto ending\n";
    file << "}\n\n";
    file << "scene right_path {\n";
    file << "    show background \"city_path.png\"\n";
    file << "    say \"You chose the right path.\"\n";
    file << "    goto ending\n";
    file << "}\n\n";
    file << "scene ending {\n";
    file << "    say \"This is the end of the demo. Expand it with your own scenes!\"\n";
    file << "}\n";
    m_metadata.startScene = "main";
  } else {
    file << "// NovelMind Script\n\n";
    file << "scene main {\n";
    file << "    say \"Hello, World!\"\n";
    file << "}\n";
    m_metadata.startScene = "main";
  }

  file.close();

  auto createSceneDoc = [&](const std::string &sceneId,
                            const std::string &bgAsset,
                            bool includeHero) -> void {
    SceneDocument doc;
    doc.sceneId = sceneId;
    int z = 0;
    if (!bgAsset.empty()) {
      SceneDocumentObject bg;
      bg.id = "background_" + sceneId;
      bg.name = "Background";
      bg.type = "Background";
      bg.zOrder = z++;
      bg.properties["textureId"] = bgAsset;
      doc.objects.push_back(std::move(bg));
    }
    if (includeHero) {
      SceneDocumentObject hero;
      hero.id = "character_hero_" + sceneId;
      hero.name = "Hero";
      hero.type = "Character";
      hero.zOrder = z++;
      hero.x = 0.0f;
      hero.y = 0.0f;
      hero.properties["characterId"] = "Hero";
      hero.properties["textureId"] = "hero.png";
      doc.objects.push_back(std::move(hero));
    }

    const fs::path scenePath = scenesDir / (sceneId + ".nmscene");
    saveSceneDocument(doc, scenePath.string());
  };

  if (templateName == "branching_story") {
    createSceneDoc("main", "crossroads.png", true);
    createSceneDoc("left_path", "forest_path.png", true);
    createSceneDoc("right_path", "city_path.png", true);
    createSceneDoc("ending", "", false);
  } else if (templateName == "kinetic_novel") {
    createSceneDoc("main", "title.png", true);
  } else {
    createSceneDoc("main", "", false);
  }

  return Result<void>::ok();
}

void ProjectManager::notifyProjectCreated() {
  for (auto *listener : m_listeners) {
    listener->onProjectCreated(m_projectPath);
  }
}

void ProjectManager::notifyProjectOpened() {
  for (auto *listener : m_listeners) {
    listener->onProjectOpened(m_projectPath);
  }
}

void ProjectManager::notifyProjectClosed() {
  for (auto *listener : m_listeners) {
    listener->onProjectClosed();
  }
}

void ProjectManager::notifyProjectSaved() {
  for (auto *listener : m_listeners) {
    listener->onProjectSaved();
  }
}

void ProjectManager::notifyProjectModified() {
  for (auto *listener : m_listeners) {
    listener->onProjectModified();
  }
}

// ============================================================================
// ProjectScope Implementation
// ============================================================================

ProjectScope::ProjectScope(const std::string &projectPath) {
  auto result = ProjectManager::instance().openProject(projectPath);
  m_valid = result.isOk();
}

ProjectScope::~ProjectScope() {
  if (m_valid) {
    ProjectManager::instance().closeProject(true);
  }
}

} // namespace NovelMind::editor
