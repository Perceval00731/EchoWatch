#pragma once

#include <FS.h>
#include <memory>

// Small in-memory file system stored in PSRAM used to expose MQTT audio payloads as fs::File.
class PSRamAudioFSImpl;

class PSRamAudioFS : public fs::FS {
public:
  PSRamAudioFS();

  // Copy data into PSRAM and publish it under the provided virtual path.
  bool load(const uint8_t* data, size_t size, const char* path);

  // Release the currently loaded buffer, if any.
  void clear();

private:
  std::shared_ptr<PSRamAudioFSImpl> getImpl() const;
};

