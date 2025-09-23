#include "PSRamAudioFS.h"

#include <Arduino.h>
#include <FSImpl.h>
#include <esp32-hal-psram.h>
#include <esp_heap_caps.h>
#include <algorithm>
#include <memory>
#include <cstring>

namespace {

struct PSRamAudioData {
  uint8_t* buffer = nullptr;
  size_t size = 0;
  String path;

  ~PSRamAudioData() {
    if (buffer) {
      heap_caps_free(buffer);
      buffer = nullptr;
    }
  }
};

class PSRamFileImpl : public fs::FileImpl {
public:
  explicit PSRamFileImpl(std::shared_ptr<PSRamAudioData> data) : data_(std::move(data)) {}

  size_t write(const uint8_t* buf, size_t size) override {
    (void)buf;
    (void)size;
    return 0;  // read-only view
  }

  size_t read(uint8_t* buf, size_t size) override {
    if (!data_ || !data_->buffer) {
      return 0;
    }
    size_t remaining = data_->size > position_ ? data_->size - position_ : 0;
    size_t toCopy = std::min(size, remaining);
    if (toCopy) {
      memcpy(buf, data_->buffer + position_, toCopy);
      position_ += toCopy;
    }
    return toCopy;
  }

  void flush() override {}

  bool seek(uint32_t pos, fs::SeekMode mode) override {
    if (!data_) {
      return false;
    }
    size_t newPos = position_;
    switch (mode) {
      case fs::SeekSet:
        newPos = pos;
        break;
      case fs::SeekCur:
        newPos = position_ + pos;
        break;
      case fs::SeekEnd:
        newPos = data_->size + pos;
        break;
      default:
        return false;
    }
    if (newPos > data_->size) {
      return false;
    }
    position_ = newPos;
    return true;
  }

  size_t position() const override {
    return position_;
  }

  size_t size() const override {
    return data_ ? data_->size : 0;
  }

  bool setBufferSize(size_t) override {
    return true;
  }

  void close() override {
    position_ = 0;
  }

  time_t getLastWrite() override {
    return 0;
  }

  const char* path() const override {
    return data_ ? data_->path.c_str() : "";
  }

  const char* name() const override {
    return path();
  }

  boolean isDirectory(void) override {
    return false;
  }

  fs::FileImplPtr openNextFile(const char*) override {
    return fs::FileImplPtr();
  }

  boolean seekDir(long) override {
    return false;
  }

  String getNextFileName(void) override {
    return String();
  }

  String getNextFileName(bool*) override {
    return String();
  }

  void rewindDirectory(void) override {}

  operator bool() override {
    return data_ && data_->buffer;
  }

private:
  std::shared_ptr<PSRamAudioData> data_;
  size_t position_ = 0;
};

class PSRamAudioFSImpl : public fs::FSImpl {
public:
  bool load(const uint8_t* data, size_t size, const char* path) {
    if (!data || size == 0 || !path) {
      return false;
    }
    if (!psramFound()) {
      return false;
    }
    clear();
    std::shared_ptr<PSRamAudioData> holder(new PSRamAudioData());
    holder->buffer = static_cast<uint8_t*>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!holder->buffer) {
      return false;
    }
    memcpy(holder->buffer, data, size);
    holder->size = size;
    holder->path = path;
    data_ = holder;
    return true;
  }

  void clear() {
    data_.reset();
  }

  fs::FileImplPtr open(const char* path, const char* mode, const bool) override {
    if (!data_) {
      return fs::FileImplPtr();
    }
    if (mode && strchr(mode, 'w')) {
      return fs::FileImplPtr();
    }
    if (path && data_->path.length() && strcmp(path, data_->path.c_str()) != 0) {
      return fs::FileImplPtr();
    }
    return fs::FileImplPtr(new PSRamFileImpl(data_));
  }

  bool exists(const char* path) override {
    if (!data_ || !data_->buffer) {
      return false;
    }
    if (!path) {
      return false;
    }
    return strcmp(path, data_->path.c_str()) == 0;
  }

  bool rename(const char* from, const char* to) override {
    if (!exists(from) || !to) {
      return false;
    }
    data_->path = to;
    return true;
  }

  bool remove(const char* path) override {
    if (!exists(path)) {
      return false;
    }
    clear();
    return true;
  }

  bool mkdir(const char*) override { return false; }
  bool mkdir(const String&) override { return false; }
  bool rmdir(const char*) override { return false; }
  bool rmdir(const String&) override { return false; }

private:
  std::shared_ptr<PSRamAudioData> data_;
};

}  // namespace

PSRamAudioFS::PSRamAudioFS() : fs::FS(std::shared_ptr<fs::FSImpl>(new PSRamAudioFSImpl())) {}

bool PSRamAudioFS::load(const uint8_t* data, size_t size, const char* path) {
  auto impl = getImpl();
  if (!impl) {
    return false;
  }
  return impl->load(data, size, path);
}

void PSRamAudioFS::clear() {
  auto impl = getImpl();
  if (impl) {
    impl->clear();
  }
}

std::shared_ptr<PSRamAudioFSImpl> PSRamAudioFS::getImpl() const {
  return std::static_pointer_cast<PSRamAudioFSImpl>(_impl);
}

