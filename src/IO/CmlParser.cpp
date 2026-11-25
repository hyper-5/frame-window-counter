#include "CmlParser.hpp"
#include <Geode/cocos/support/zip_support/ZipUtils.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace CmlParser {

static bool decompressGzip(const uint8_t *compressedData, size_t compressedSize,
                           std::vector<uint8_t> &outBuffer,
                           size_t expectedSize = 0) {
  unsigned char *decompressedBuf = nullptr;
  auto len = cocos2d::ZipUtils::ccInflateMemory(
      const_cast<unsigned char *>(compressedData),
      static_cast<unsigned int>(compressedSize), &decompressedBuf);
  if (len <= 0 || !decompressedBuf) {
    if (decompressedBuf)
      free(decompressedBuf);
    return false;
  }
  outBuffer.assign(decompressedBuf, decompressedBuf + len);
  free(decompressedBuf);
  return true;
}

class CmlReader {
public:
  const uint8_t *data;
  size_t size;
  size_t pos;
  bool encoded_strings;

  CmlReader(const uint8_t *d, size_t s, bool enc, size_t p = 0)
      : data(d), size(s), pos(p), encoded_strings(enc) {}

  uint8_t read_u8() {
    if (pos >= size)
      throw std::runtime_error("Unexpected EOF reading CML byte");
    return data[pos++];
  }

  bool read_bool() { return read_u8() != 0; }

  float read_f32() {
    if (pos + 4 > size)
      throw std::runtime_error("Unexpected EOF reading CML f32");
    uint32_t val = static_cast<uint32_t>(data[pos]) |
                   (static_cast<uint32_t>(data[pos + 1]) << 8) |
                   (static_cast<uint32_t>(data[pos + 2]) << 16) |
                   (static_cast<uint32_t>(data[pos + 3]) << 24);
    pos += 4;
    float f;
    std::memcpy(&f, &val, sizeof(float));
    return f;
  }

  uint64_t read_var_u64() {
    uint64_t value = 0;
    uint32_t shift = 0;
    while (true) {
      if (shift > 63)
        throw std::runtime_error("CML varint is too long");
      uint8_t byte = read_u8();
      value |= static_cast<uint64_t>(byte & 0x7f) << shift;
      if ((byte & 0x80) == 0)
        return value;
      shift += 7;
    }
  }

  int64_t read_var_i64() {
    uint64_t value = read_var_u64();
    return static_cast<int64_t>((value >> 1) ^
                                (-(static_cast<int64_t>(value & 1))));
  }

  static uint8_t string_key(size_t index) {
    uint64_t i = static_cast<uint64_t>(index);
    return static_cast<uint8_t>((i * 0x3d + 0xa7 + ((i >> 1) * 0x11)) & 0xff);
  }

  std::string read_string() {
    size_t len = static_cast<size_t>(read_var_u64());
    if (pos + len > size)
      throw std::runtime_error("Unexpected EOF reading CML string");
    std::string s;
    s.resize(len);
    for (size_t i = 0; i < len; ++i) {
      uint8_t b = data[pos + i];
      if (encoded_strings) {
        b ^= string_key(i);
      }
      s[i] = static_cast<char>(b);
    }
    pos += len;
    return s;
  }
};

class CmlBitReader {
public:
  const uint8_t *data;
  size_t size;
  size_t pos;
  uint8_t buf;
  uint32_t bits_left;
  bool encoded_strings;

  CmlBitReader(const uint8_t *d, size_t s, bool enc)
      : data(d), size(s), pos(0), buf(0), bits_left(0), encoded_strings(enc) {}

  bool read_bit() {
    if (bits_left == 0) {
      if (pos >= size)
        throw std::runtime_error("Unexpected EOF reading CML v7 bit");
      buf = data[pos++];
      bits_left = 8;
    }
    bits_left--;
    return ((buf >> bits_left) & 1) == 1;
  }

  uint64_t read_bits(uint32_t count) {
    uint64_t value = 0;
    for (uint32_t i = 0; i < count; ++i) {
      value = (value << 1) | (read_bit() ? 1ULL : 0ULL);
    }
    return value;
  }

  uint8_t read_byte() { return static_cast<uint8_t>(read_bits(8)); }

  float read_f32() {
    uint32_t bits = static_cast<uint32_t>(read_bits(32));
    float f;
    std::memcpy(&f, &bits, sizeof(float));
    return f;
  }

  uint64_t read_var_u64() {
    uint64_t value = 0;
    uint32_t shift = 0;
    while (true) {
      bool cont = read_bit();
      uint64_t chunk = read_bits(7);
      value |= chunk << shift;
      shift += 7;
      if (!cont)
        return value;
      if (shift >= 64)
        throw std::runtime_error("CML v7 varint is too long");
    }
  }

  int64_t read_var_i64() {
    uint64_t value = read_var_u64();
    return static_cast<int64_t>((value >> 1) ^
                                (-(static_cast<int64_t>(value & 1))));
  }

  static uint8_t string_key(size_t index) {
    uint64_t i = static_cast<uint64_t>(index);
    return static_cast<uint8_t>((i * 0x3d + 0xa7 + ((i >> 1) * 0x11)) & 0xff);
  }

  std::string read_string() {
    size_t len = static_cast<size_t>(read_var_u64());
    std::string s;
    s.reserve(len);
    for (size_t i = 0; i < len; ++i) {
      uint8_t byte = read_byte();
      if (encoded_strings) {
        byte ^= string_key(i);
      }
      s.push_back(static_cast<char>(byte));
    }
    return s;
  }
};

static void parse_v1_to_v6(CmlReader &cml, uint64_t version,
                           CmlParseResult &res) {
  std::string author = cml.read_string();
  std::string description = cml.read_string();
  float accuracy = cml.read_f32();
  float duration = cml.read_f32();
  float speedhack = cml.read_f32();
  float fps = cml.read_f32();
  if (fps > 0.0f) {
    res.fps = static_cast<double>(fps);
    res.hasFps = true;
  }

  int64_t unknown_a = cml.read_var_i64();
  int64_t unknown_b = cml.read_var_i64();
  bool unknown_flag = cml.read_bool();
  int64_t last_frame = cml.read_var_i64();
  std::string bot_name = cml.read_string();
  std::string bot_version = cml.read_string();
  uint64_t seed = cml.read_var_u64();
  std::string macro_name = cml.read_string();

  size_t input_count = static_cast<size_t>(cml.read_var_u64());
  res.actions.reserve(res.actions.size() + input_count);

  int64_t frame = 0;
  for (size_t i = 0; i < input_count; ++i) {
      frame += cml.read_var_i64();
      if (frame < 0)
          throw std::runtime_error("CML input frame became negative");
      uint8_t flags = cml.read_u8();
      bool isHold = (flags & 0x01) != 0;
      bool isPlayer2 = (flags & 0x02) != 0;

      int32_t actual_frame = (version >= 5 && version <= 6)
          ? static_cast<int32_t>(frame / 1000000)
          : static_cast<int32_t>(frame);

      res.actions.push_back({ actual_frame, false, 1.0, isPlayer2, 1, isHold });
  }
}

static void parse_v7(const std::vector<uint8_t> &decompressed,
                     bool encoded_strings, CmlParseResult &res) {
  constexpr int64_t CML_V7_SUBTICK_SCALE = 1000000;

  CmlBitReader bits(decompressed.data(), decompressed.size(), encoded_strings);

  std::string author = bits.read_string();
  std::string description = bits.read_string();
  float unknown_f1 = bits.read_f32();
  float unknown_f2 = bits.read_f32();
  float unknown_f3 = bits.read_f32();
  float fps = bits.read_f32();
  if (fps > 0.0f) {
    res.fps = static_cast<double>(fps);
    res.hasFps = true;
  }

  int64_t unknown_a = bits.read_var_i64();
  int64_t unknown_b = bits.read_var_i64();
  bool unknown_flag = bits.read_bit();
  int64_t last_frame = bits.read_var_i64();
  std::string bot_name = bits.read_string();
  std::string bot_version = bits.read_string();
  uint64_t seed = bits.read_var_u64();
  std::string macro_name = bits.read_string();

  size_t input_count = static_cast<size_t>(bits.read_var_u64());
  res.actions.reserve(res.actions.size() + input_count);

  int64_t subtick = 0;
  for (size_t i = 0; i < input_count; ++i) {
    int64_t delta = 0;
    if (bits.read_bit()) {
      delta = CML_V7_SUBTICK_SCALE;
    } else {
      delta = bits.read_var_i64();
    }

    // wrapping add
    subtick = static_cast<int64_t>(static_cast<uint64_t>(subtick) +
                                   static_cast<uint64_t>(delta));
    int64_t s = std::max<int64_t>(0, subtick);
    int32_t frame = static_cast<int32_t>(s / CML_V7_SUBTICK_SCALE);

    uint8_t flags = bits.read_byte();
    bool isHold = (flags & 0x01) != 0;
    bool isPlayer2 = (flags & 0x02) != 0;

    res.actions.push_back({ frame, false, 1.0, isPlayer2, 1, isHold });
  }
}

CmlParseResult parse(const std::vector<uint8_t> &data) {
  if (data.size() < 4) {
    throw std::runtime_error("CML file is too short to contain a header");
  }

  bool encoded_strings = false;
  if (data[0] == 0xd7 && data[1] == 0x8a && data[2] == 0x3e &&
      data[3] == 0x91) {
    encoded_strings = true;
  } else if (data[0] == 'C' && data[1] == 'M' && data[2] == 'L' &&
             data[3] == '\0') {
    encoded_strings = false;
  } else {
    throw std::runtime_error("Invalid CML magic signature");
  }

  CmlReader reader(data.data(), data.size(), encoded_strings, 4);
  uint64_t version = reader.read_var_u64();
  if (!((version >= 1 && version <= 3) || (version >= 5 && version <= 7))) {
    throw std::runtime_error("Unsupported CML version: " +
                             std::to_string(version));
  }

  CmlParseResult res;

  if (version >= 5 && version <= 7) {
    uint64_t decompressed_size = reader.read_var_u64();
    if (reader.pos > data.size()) {
      throw std::runtime_error("Unexpected EOF before CML compressed payload");
    }
    const uint8_t *gzip_ptr = data.data() + reader.pos;
    size_t gzip_size = data.size() - reader.pos;

    std::vector<uint8_t> decompressed;
    if (!decompressGzip(gzip_ptr, gzip_size, decompressed,
                        static_cast<size_t>(decompressed_size))) {
      throw std::runtime_error("Failed to decompress CML gzip payload");
    }

    if (version == 7) {
      parse_v7(decompressed, encoded_strings, res);
    } else {
      CmlReader decompressedReader(decompressed.data(), decompressed.size(),
                                   encoded_strings, 0);
      parse_v1_to_v6(decompressedReader, version, res);
    }
  } else {
    parse_v1_to_v6(reader, version, res);
  }

  std::stable_sort(res.actions.begin(), res.actions.end(),
                   [](const FrameAction &a, const FrameAction &b) {
                     return a.frame < b.frame;
                   });

  return res;
}

CmlParseResult parse(const std::filesystem::path &path) {
  std::ifstream file(path.string(), std::ios::binary);
  if (!file) {
    throw std::runtime_error("Cannot open .cml file: " +
                             path.filename().string());
  }
  file.seekg(0, std::ios::end);
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  if (size <= 0) {
    throw std::runtime_error("CML file is empty: " + path.filename().string());
  }

  std::vector<uint8_t> buffer(static_cast<size_t>(size));
  if (!file.read(reinterpret_cast<char *>(buffer.data()), size)) {
    throw std::runtime_error("Failed to read .cml file: " +
                             path.filename().string());
  }

  return parse(buffer);
}
} // namespace CmlParser
