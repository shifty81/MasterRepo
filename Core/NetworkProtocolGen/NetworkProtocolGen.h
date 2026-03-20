#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace Core {

// ──────────────────────────────────────────────────────────────
// Field descriptor for a protocol message
// ──────────────────────────────────────────────────────────────

enum class FieldType {
    Int8, Int16, Int32, Int64,
    UInt8, UInt16, UInt32, UInt64,
    Float32, Float64,
    Bool,
    String,
    Bytes,
    Nested   // references another message by name
};

struct FieldDescriptor {
    std::string name;
    FieldType   type      = FieldType::UInt32;
    std::string nestedTypeName; // used when type == Nested
    bool        repeated  = false; // array field
    bool        optional  = false;
    uint32_t    tag       = 0;     // wire tag (1-based index)
};

// ──────────────────────────────────────────────────────────────
// Message descriptor
// ──────────────────────────────────────────────────────────────

struct MessageDescriptor {
    std::string                  name;
    std::vector<FieldDescriptor> fields;
};

// ──────────────────────────────────────────────────────────────
// NetworkProtocolGen
// Auto-generates C++ serialization stubs for multiplayer messages
// ──────────────────────────────────────────────────────────────

class NetworkProtocolGen {
public:
    // Add message definitions
    void AddMessage(const MessageDescriptor& msg);
    void ClearMessages();
    const std::vector<MessageDescriptor>& Messages() const { return m_messages; }

    // Schema import: parse a simple .proto-like schema file
    bool LoadSchema(const std::string& schemaPath);

    // Code generation
    // Generates a .h and a .cpp file pair in the output directory
    bool GenerateHeader(const std::string& outputPath,
                        const std::string& namespaceName = "Net") const;
    bool GenerateCpp   (const std::string& outputPath,
                        const std::string& namespaceName = "Net") const;
    bool GenerateBoth  (const std::string& outputDir,
                        const std::string& baseName,
                        const std::string& namespaceName = "Net") const;

    // Binary wire format helpers (big-endian, tag-length-value)
    static std::vector<uint8_t> Serialize(const std::unordered_map<std::string,std::string>& fields,
                                          const MessageDescriptor& desc);
    static std::unordered_map<std::string,std::string> Deserialize(
                                          const std::vector<uint8_t>& data,
                                          const MessageDescriptor& desc);

private:
    std::string FieldTypeName(FieldType t) const;
    std::string SerializeCall(const FieldDescriptor& f) const;
    std::string DeserializeCall(const FieldDescriptor& f) const;

    std::vector<MessageDescriptor> m_messages;
};

} // namespace Core
