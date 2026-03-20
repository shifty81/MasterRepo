#include "Core/NetworkProtocolGen/NetworkProtocolGen.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstdint>

namespace Core {

// ── message management ─────────────────────────────────────────

void NetworkProtocolGen::AddMessage(const MessageDescriptor& msg) {
    // Auto-assign tags if missing
    MessageDescriptor copy = msg;
    uint32_t tag = 1;
    for (auto& f : copy.fields)
        if (f.tag == 0) f.tag = tag++;
    m_messages.push_back(std::move(copy));
}

void NetworkProtocolGen::ClearMessages() { m_messages.clear(); }

// ── schema loading ─────────────────────────────────────────────
// Simple schema format (one message per block):
//   message PlayerUpdate {
//     uint32 id;
//     float32 x;
//     float32 y;
//   }

bool NetworkProtocolGen::LoadSchema(const std::string& schemaPath) {
    std::ifstream f(schemaPath);
    if (!f.is_open()) return false;

    static const std::unordered_map<std::string, FieldType> typeMap = {
        {"int8",FieldType::Int8},   {"int16",FieldType::Int16},
        {"int32",FieldType::Int32}, {"int64",FieldType::Int64},
        {"uint8",FieldType::UInt8}, {"uint16",FieldType::UInt16},
        {"uint32",FieldType::UInt32},{"uint64",FieldType::UInt64},
        {"float32",FieldType::Float32},{"float64",FieldType::Float64},
        {"bool",FieldType::Bool},   {"string",FieldType::String},
        {"bytes",FieldType::Bytes}
    };

    std::string line;
    MessageDescriptor cur;
    bool inMessage = false;
    uint32_t tag = 1;

    while (std::getline(f, line)) {
        // trim
        auto start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line[0] == '/' ) continue;
        line = line.substr(start);

        if (line.rfind("message ", 0) == 0) {
            cur = {};
            tag = 1;
            auto name = line.substr(8);
            auto brace = name.find('{');
            if (brace != std::string::npos) name = name.substr(0, brace);
            auto end = name.find_last_not_of(" \t");
            cur.name = (end != std::string::npos) ? name.substr(0, end + 1) : name;
            inMessage = true;
        } else if (line[0] == '}') {
            if (inMessage && !cur.name.empty())
                AddMessage(cur);
            inMessage = false;
        } else if (inMessage) {
            // "type name;" or "repeated type name;"
            std::istringstream ss(line);
            std::string tok1, tok2;
            ss >> tok1 >> tok2;
            if (tok1.empty() || tok2.empty()) continue;

            FieldDescriptor fd;
            fd.repeated = (tok1 == "repeated");
            if (fd.repeated) {
                std::string tok3; ss >> tok3;
                std::swap(tok1, tok2); // tok1=type tok2=name (old tok2->tok3)
                tok2 = tok3;
            }
            fd.optional = (tok1 == "optional");
            if (fd.optional) { std::string t3; ss >> t3; tok2 = t3; }

            // strip trailing semicolon from name
            if (!tok2.empty() && tok2.back() == ';') tok2.pop_back();
            fd.name = tok2;
            fd.tag  = tag++;

            auto it = typeMap.find(tok1);
            if (it != typeMap.end()) {
                fd.type = it->second;
            } else {
                fd.type = FieldType::Nested;
                fd.nestedTypeName = tok1;
            }
            cur.fields.push_back(fd);
        }
    }
    return true;
}

// ── code generation ────────────────────────────────────────────

std::string NetworkProtocolGen::FieldTypeName(FieldType t) const {
    switch (t) {
        case FieldType::Int8:    return "int8_t";
        case FieldType::Int16:   return "int16_t";
        case FieldType::Int32:   return "int32_t";
        case FieldType::Int64:   return "int64_t";
        case FieldType::UInt8:   return "uint8_t";
        case FieldType::UInt16:  return "uint16_t";
        case FieldType::UInt32:  return "uint32_t";
        case FieldType::UInt64:  return "uint64_t";
        case FieldType::Float32: return "float";
        case FieldType::Float64: return "double";
        case FieldType::Bool:    return "bool";
        case FieldType::String:  return "std::string";
        case FieldType::Bytes:   return "std::vector<uint8_t>";
        case FieldType::Nested:  return "/* nested */";
    }
    return "uint32_t";
}

bool NetworkProtocolGen::GenerateHeader(const std::string& outputPath,
                                        const std::string& namespaceName) const {
    std::ofstream f(outputPath);
    if (!f.is_open()) return false;

    f << "#pragma once\n";
    f << "#include <cstdint>\n#include <string>\n#include <vector>\n\n";
    f << "namespace " << namespaceName << " {\n\n";

    for (const auto& msg : m_messages) {
        f << "struct " << msg.name << " {\n";
        for (const auto& field : msg.fields) {
            std::string typeName = (field.type == FieldType::Nested)
                                   ? field.nestedTypeName
                                   : FieldTypeName(field.type);
            if (field.repeated)
                typeName = "std::vector<" + typeName + ">";
            f << "    " << typeName << " " << field.name;
            if (field.type == FieldType::Bool) f << " = false";
            else if (field.type != FieldType::String &&
                     field.type != FieldType::Bytes  &&
                     field.type != FieldType::Nested &&
                     !field.repeated)
                f << " = 0";
            f << ";\n";
        }
        f << "\n";
        f << "    std::vector<uint8_t> Serialize() const;\n";
        f << "    static " << msg.name << " Deserialize(const std::vector<uint8_t>& data);\n";
        f << "};\n\n";
    }

    f << "} // namespace " << namespaceName << "\n";
    return true;
}

bool NetworkProtocolGen::GenerateCpp(const std::string& outputPath,
                                     const std::string& namespaceName) const {
    std::ofstream f(outputPath);
    if (!f.is_open()) return false;

    f << "#include <cstring>\n#include <cstdint>\n";
    f << "#include \"NetProtocol.h\"\n\n";
    f << "namespace " << namespaceName << " {\n\n";

    // Helper lambdas (inline in generated code)
    f << "// -- wire helpers --\n";
    f << "static void WriteU32(std::vector<uint8_t>& buf, uint32_t v) {\n";
    f << "    buf.push_back((v>>24)&0xFF); buf.push_back((v>>16)&0xFF);\n";
    f << "    buf.push_back((v>>8)&0xFF);  buf.push_back(v&0xFF); }\n";
    f << "static uint32_t ReadU32(const uint8_t* p) {\n";
    f << "    return (uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3]; }\n\n";

    for (const auto& msg : m_messages) {
        // Serialize
        f << "std::vector<uint8_t> " << msg.name << "::Serialize() const {\n";
        f << "    std::vector<uint8_t> buf;\n";
        for (const auto& field : msg.fields) {
            if (field.type == FieldType::String) {
                f << "    WriteU32(buf, " << field.tag << ");\n";
                f << "    WriteU32(buf, static_cast<uint32_t>(" << field.name << ".size()));\n";
                f << "    buf.insert(buf.end(), " << field.name << ".begin(), " << field.name << ".end());\n";
            } else if (field.type == FieldType::UInt32 || field.type == FieldType::Int32) {
                f << "    WriteU32(buf, " << field.tag << ");\n";
                f << "    WriteU32(buf, static_cast<uint32_t>(" << field.name << "));\n";
            } else if (field.type == FieldType::Float32) {
                f << "    WriteU32(buf, " << field.tag << ");\n";
                f << "    { uint32_t tmp; std::memcpy(&tmp, &" << field.name << ", 4); WriteU32(buf, tmp); }\n";
            } else if (field.type == FieldType::Bool) {
                f << "    WriteU32(buf, " << field.tag << ");\n";
                f << "    buf.push_back(" << field.name << " ? 1 : 0);\n";
            }
            // other types: left as TODO stubs
        }
        f << "    return buf;\n}\n\n";

        // Deserialize
        f << msg.name << " " << msg.name << "::Deserialize(const std::vector<uint8_t>& data) {\n";
        f << "    " << msg.name << " obj;\n";
        f << "    size_t pos = 0;\n";
        f << "    while (pos + 8 <= data.size()) {\n";
        f << "        uint32_t tag = ReadU32(data.data() + pos); pos += 4;\n";
        f << "        uint32_t val = ReadU32(data.data() + pos); pos += 4;\n";
        for (const auto& field : msg.fields) {
            f << "        if (tag == " << field.tag << ") {\n";
            if (field.type == FieldType::String) {
                f << "            uint32_t len = val;\n";
                f << "            if (pos + len <= data.size()) {\n";
                f << "                obj." << field.name << ".assign(data.begin()+pos, data.begin()+pos+len);\n";
                f << "                pos += len;\n            }\n";
            } else if (field.type == FieldType::Float32) {
                f << "            std::memcpy(&obj." << field.name << ", &val, 4);\n";
            } else if (field.type == FieldType::Bool) {
                f << "            obj." << field.name << " = (val != 0);\n";
            } else {
                f << "            obj." << field.name << " = static_cast<decltype(obj." << field.name << ")>(val);\n";
            }
            f << "        }\n";
        }
        f << "    }\n";
        f << "    return obj;\n}\n\n";
    }

    f << "} // namespace " << namespaceName << "\n";
    return true;
}

bool NetworkProtocolGen::GenerateBoth(const std::string& outputDir,
                                      const std::string& baseName,
                                      const std::string& namespaceName) const {
    return GenerateHeader(outputDir + "/" + baseName + ".h",   namespaceName) &&
           GenerateCpp   (outputDir + "/" + baseName + ".cpp", namespaceName);
}

// ── simple serialize/deserialize helpers ──────────────────────

std::vector<uint8_t> NetworkProtocolGen::Serialize(
    const std::unordered_map<std::string,std::string>& fields,
    const MessageDescriptor& desc)
{
    std::vector<uint8_t> buf;
    for (const auto& fd : desc.fields) {
        auto it = fields.find(fd.name);
        if (it == fields.end()) continue;
        // tag (4 bytes big-endian)
        uint32_t tag = fd.tag;
        buf.push_back((tag>>24)&0xFF); buf.push_back((tag>>16)&0xFF);
        buf.push_back((tag>>8)&0xFF);  buf.push_back(tag&0xFF);
        // value as string bytes
        const std::string& v = it->second;
        uint32_t len = static_cast<uint32_t>(v.size());
        buf.push_back((len>>24)&0xFF); buf.push_back((len>>16)&0xFF);
        buf.push_back((len>>8)&0xFF);  buf.push_back(len&0xFF);
        buf.insert(buf.end(), v.begin(), v.end());
    }
    return buf;
}

std::unordered_map<std::string,std::string> NetworkProtocolGen::Deserialize(
    const std::vector<uint8_t>& data,
    const MessageDescriptor& desc)
{
    std::unordered_map<std::string,std::string> result;
    size_t pos = 0;
    while (pos + 8 <= data.size()) {
        uint32_t tag = (uint32_t(data[pos])<<24)|(uint32_t(data[pos+1])<<16)
                      |(uint32_t(data[pos+2])<<8)|data[pos+3]; pos+=4;
        uint32_t len = (uint32_t(data[pos])<<24)|(uint32_t(data[pos+1])<<16)
                      |(uint32_t(data[pos+2])<<8)|data[pos+3]; pos+=4;
        if (pos + len > data.size()) break;
        std::string value(data.begin()+pos, data.begin()+pos+len);
        pos += len;
        for (const auto& fd : desc.fields)
            if (fd.tag == tag) { result[fd.name] = std::move(value); break; }
    }
    return result;
}

} // namespace Core
