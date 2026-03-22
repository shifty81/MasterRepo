#pragma once
/**
 * @file BinarySerializer.h
 * @brief Fast little-endian binary serialization / deserialization.
 *
 * BinarySerializer provides a pair of stream classes for compact binary I/O:
 *
 *   BinaryWriter:
 *     - Wraps a resizable byte buffer (or a caller-supplied span).
 *     - Write<T>(val): append a trivially-copyable value in little-endian order.
 *     - WriteString(s): length-prefixed (uint32_t) UTF-8 string.
 *     - WriteBytes(ptr, n): raw byte span.
 *     - WriteTag(fourcc): 4-byte section tag for forward-compatibility.
 *     - Position / Size / Data accessors.
 *
 *   BinaryReader:
 *     - Wraps a read-only byte span.
 *     - Read<T>(out): deserialise a trivially-copyable value; returns false on underflow.
 *     - ReadString(out): length-prefixed string.
 *     - ReadBytes(out, n): raw copy.
 *     - ReadTag(fourcc): validate a 4-byte tag.
 *     - Position / Remaining / IsValid.
 *
 * Version field helpers:
 *   - WriteVersion(major, minor) / ReadVersion(major, minor).
 *
 * Endianness:
 *   All multi-byte values are stored little-endian regardless of host byte order.
 */

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <type_traits>
#include <array>

namespace Core {

// ── Byte-swap helpers ─────────────────────────────────────────────────────
namespace detail {
    inline uint16_t bswap16(uint16_t v) { return static_cast<uint16_t>((v >> 8) | (v << 8)); }
    inline uint32_t bswap32(uint32_t v) {
        return ((v & 0xFF000000u) >> 24) | ((v & 0x00FF0000u) >> 8) |
               ((v & 0x0000FF00u) << 8)  | ((v & 0x000000FFu) << 24);
    }
    inline uint64_t bswap64(uint64_t v) {
        return ((v & 0xFF00000000000000ull) >> 56) | ((v & 0x00FF000000000000ull) >> 40) |
               ((v & 0x0000FF0000000000ull) >> 24) | ((v & 0x000000FF00000000ull) >>  8) |
               ((v & 0x00000000FF000000ull) <<  8) | ((v & 0x0000000000FF0000ull) << 24) |
               ((v & 0x000000000000FF00ull) << 40) | ((v & 0x00000000000000FFull) << 56);
    }
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    template<size_t N> struct ToLE;
    template<> struct ToLE<1> { static uint8_t  cvt(uint8_t  v) { return v; } };
    template<> struct ToLE<2> { static uint16_t cvt(uint16_t v) { return bswap16(v); } };
    template<> struct ToLE<4> { static uint32_t cvt(uint32_t v) { return bswap32(v); } };
    template<> struct ToLE<8> { static uint64_t cvt(uint64_t v) { return bswap64(v); } };
#else
    template<size_t N> struct ToLE { template<typename T> static T cvt(T v) { return v; } };
#endif
} // namespace detail

// ── BinaryWriter ─────────────────────────────────────────────────────────
class BinaryWriter {
public:
    BinaryWriter() = default;
    explicit BinaryWriter(size_t reserveBytes) { m_buf.reserve(reserveBytes); }

    // ── primitive ─────────────────────────────────────────────
    template<typename T>
    void Write(T val) {
        static_assert(std::is_trivially_copyable_v<T>);
        T le;
        if constexpr (sizeof(T) == 1) { le = val; }
        else {
            // Reinterpret as unsigned int of same size, swap, copy back
            using U = std::conditional_t<sizeof(T)==2, uint16_t,
                      std::conditional_t<sizeof(T)==4, uint32_t, uint64_t>>;
            U raw; std::memcpy(&raw, &val, sizeof(U));
            raw = detail::ToLE<sizeof(U)>::cvt(raw);
            std::memcpy(&le, &raw, sizeof(T));
        }
        const auto* p = reinterpret_cast<const uint8_t*>(&le);
        m_buf.insert(m_buf.end(), p, p + sizeof(T));
    }

    void WriteString(const std::string& s) {
        Write(static_cast<uint32_t>(s.size()));
        const auto* p = reinterpret_cast<const uint8_t*>(s.data());
        m_buf.insert(m_buf.end(), p, p + s.size());
    }

    void WriteBytes(const void* data, size_t n) {
        const auto* p = reinterpret_cast<const uint8_t*>(data);
        m_buf.insert(m_buf.end(), p, p + n);
    }

    void WriteTag(const char (&fourcc)[5]) {
        m_buf.insert(m_buf.end(), fourcc, fourcc + 4);
    }

    void WriteVersion(uint16_t major, uint16_t minor) {
        Write(major); Write(minor);
    }

    // ── accessors ─────────────────────────────────────────────
    const std::vector<uint8_t>& Data()     const { return m_buf; }
    size_t                       Size()     const { return m_buf.size(); }
    size_t                       Position() const { return m_buf.size(); }
    void                         Clear()          { m_buf.clear(); }

private:
    std::vector<uint8_t> m_buf;
};

// ── BinaryReader ─────────────────────────────────────────────────────────
class BinaryReader {
public:
    BinaryReader() = default;
    BinaryReader(const uint8_t* data, size_t size) : m_data(data), m_size(size) {}
    explicit BinaryReader(const std::vector<uint8_t>& buf)
        : m_data(buf.data()), m_size(buf.size()) {}

    // ── primitive ─────────────────────────────────────────────
    template<typename T>
    bool Read(T& out) {
        static_assert(std::is_trivially_copyable_v<T>);
        if (m_pos + sizeof(T) > m_size) { m_valid = false; return false; }
        std::memcpy(&out, m_data + m_pos, sizeof(T));
        m_pos += sizeof(T);
        if constexpr (sizeof(T) > 1) {
            using U = std::conditional_t<sizeof(T)==2, uint16_t,
                      std::conditional_t<sizeof(T)==4, uint32_t, uint64_t>>;
            U raw; std::memcpy(&raw, &out, sizeof(U));
            raw = detail::ToLE<sizeof(U)>::cvt(raw);
            std::memcpy(&out, &raw, sizeof(T));
        }
        return true;
    }

    bool ReadString(std::string& out) {
        uint32_t len = 0;
        if (!Read(len)) return false;
        if (m_pos + len > m_size) { m_valid = false; return false; }
        out.assign(reinterpret_cast<const char*>(m_data + m_pos), len);
        m_pos += len;
        return true;
    }

    bool ReadBytes(void* dst, size_t n) {
        if (m_pos + n > m_size) { m_valid = false; return false; }
        std::memcpy(dst, m_data + m_pos, n);
        m_pos += n;
        return true;
    }

    bool ReadTag(const char (&expected)[5]) {
        if (m_pos + 4 > m_size) { m_valid = false; return false; }
        bool ok = std::memcmp(m_data + m_pos, expected, 4) == 0;
        m_pos += 4;
        if (!ok) m_valid = false;
        return ok;
    }

    bool ReadVersion(uint16_t& major, uint16_t& minor) {
        return Read(major) && Read(minor);
    }

    // ── accessors ─────────────────────────────────────────────
    size_t Position()  const { return m_pos; }
    size_t Remaining() const { return m_size > m_pos ? m_size - m_pos : 0; }
    bool   IsValid()   const { return m_valid; }
    void   Reset()           { m_pos = 0; m_valid = true; }

private:
    const uint8_t* m_data{nullptr};
    size_t         m_size{0};
    size_t         m_pos{0};
    bool           m_valid{true};
};

} // namespace Core
