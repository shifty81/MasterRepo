#pragma once
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace NF {

/// @brief Abstract base for all serialization archives.
class Archive {
public:
    virtual ~Archive() = default;

    /// @return true when the archive is deserialising (reading from a source).
    [[nodiscard]] virtual bool IsReading() const noexcept = 0;

    /// @return true when the archive is serialising (writing to a destination).
    [[nodiscard]] bool IsWriting() const noexcept { return !IsReading(); }
};

/// @brief In-memory binary archive.
///
/// In write mode the archive accumulates bytes in an internal buffer; in read
/// mode it consumes bytes from a caller-supplied span.
class BinaryArchive : public Archive {
public:
    /// @brief Construct a write-mode archive with an empty internal buffer.
    BinaryArchive() noexcept : m_Reading(false) {}

    /// @brief Construct a read-mode archive over existing byte data.
    /// @param data  Pointer to the source bytes (must remain valid for the
    ///              lifetime of the archive).
    /// @param size  Number of bytes available.
    BinaryArchive(const std::byte* data, size_t size)
        : m_Buffer(data, data + size), m_Reading(true) {}

    [[nodiscard]] bool IsReading() const noexcept override { return m_Reading; }

    /// @brief Read or write a trivially-copyable value.
    ///
    /// In write mode the bytes of @p val are appended to the internal buffer.
    /// In read mode @p val is filled from the next sizeof(T) bytes.
    ///
    /// @tparam T  A TriviallyCopyable type.
    /// @param val Value to serialize (in/out).
    /// @throws std::out_of_range if reading past the end of the buffer.
    template<typename T>
    requires std::is_trivially_copyable_v<T>
    void Serialize(T& val) {
        if (m_Reading) {
            if (m_ReadPos + sizeof(T) > m_Buffer.size())
                throw std::out_of_range("BinaryArchive: read past end of buffer");
            std::memcpy(&val, m_Buffer.data() + m_ReadPos, sizeof(T));
            m_ReadPos += sizeof(T);
        } else {
            const auto* src = reinterpret_cast<const std::byte*>(&val);
            m_Buffer.insert(m_Buffer.end(), src, src + sizeof(T));
        }
    }

    /// @brief Access the written byte buffer (only meaningful in write mode).
    [[nodiscard]] const std::vector<std::byte>& Data() const noexcept { return m_Buffer; }

private:
    std::vector<std::byte> m_Buffer;
    size_t                 m_ReadPos{0};
    bool                   m_Reading;
};

} // namespace NF
