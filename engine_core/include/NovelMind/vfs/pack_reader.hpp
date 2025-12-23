#pragma once

#include "NovelMind/vfs/virtual_fs.hpp"
#include <fstream>
#include <mutex>
#include <unordered_map>

namespace NovelMind::vfs {

constexpr u32 PACK_MAGIC = 0x53524D4E; // "NMRS" in little-endian
constexpr u16 PACK_VERSION_MAJOR = 1;
constexpr u16 PACK_VERSION_MINOR = 0;

struct PackHeader {
  u32 magic;
  u16 versionMajor;
  u16 versionMinor;
  u32 flags;
  u32 resourceCount;
  u64 resourceTableOffset;
  u64 stringTableOffset;
  u64 dataOffset;
  u64 totalSize;
  u8 contentHash[16];
};

struct PackResourceEntry {
  u32 idStringOffset;
  u32 type;
  u64 dataOffset;
  u64 compressedSize;
  u64 uncompressedSize;
  u32 flags;
  u32 checksum;
  u8 iv[8];
};

enum class PackFlags : u32 {
  None = 0,
  Encrypted = 1 << 0,
  Compressed = 1 << 1,
  Signed = 1 << 2
};

class PackReader : public IVirtualFileSystem {
public:
  PackReader() = default;
  ~PackReader() override;

  Result<void> mount(const std::string &packPath) override;
  void unmount(const std::string &packPath) override;
  void unmountAll() override;

  [[nodiscard]] Result<std::vector<u8>>
  readFile(const std::string &resourceId) const override;

  [[nodiscard]] bool exists(const std::string &resourceId) const override;

  [[nodiscard]] std::optional<ResourceInfo>
  getInfo(const std::string &resourceId) const override;

  [[nodiscard]] std::vector<std::string>
  listResources(ResourceType type = ResourceType::Unknown) const override;

private:
  struct MountedPack {
    std::string path;
    PackHeader header;
    std::unordered_map<std::string, PackResourceEntry> entries;
    std::vector<std::string> stringTable;
  };

  Result<void> readPackHeader(std::ifstream &file, PackHeader &header);
  Result<void> readResourceTable(std::ifstream &file, MountedPack &pack);
  Result<void> readStringTable(std::ifstream &file, MountedPack &pack);

  [[nodiscard]] Result<std::vector<u8>>
  readResourceData(const std::string &packPath,
                   const PackResourceEntry &entry) const;

  mutable std::mutex m_mutex;
  std::unordered_map<std::string, MountedPack> m_packs;
};

} // namespace NovelMind::vfs
