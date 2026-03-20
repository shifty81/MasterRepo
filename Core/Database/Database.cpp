#include "Core/Database/Database.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace Core {

// ── JSON helpers (hand-rolled, no external deps) ──────────────────────────────

static std::string esc(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else           out += c;
    }
    return out;
}

static std::string extractVal(const std::string& json, const std::string& key) {
    std::string pat = "\"" + key + "\":";
    auto pos = json.find(pat);
    if (pos == std::string::npos) {
        pat = "\"" + key + "\": ";
        pos = json.find(pat);
    }
    if (pos == std::string::npos) return {};
    pos += pat.size();
    while (pos < json.size() && json[pos] == ' ') ++pos;
    if (pos >= json.size()) return {};
    if (json[pos] == '"') {
        ++pos;
        std::string val;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) { ++pos; }
            val += json[pos++];
        }
        return val;
    }
    // number or bool
    std::string val;
    while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != '\n')
        val += json[pos++];
    while (!val.empty() && (val.back() == ' ' || val.back() == '\r')) val.pop_back();
    return val;
}

static bool extractBool(const std::string& json, const std::string& key) {
    return extractVal(json, key) == "true";
}

// ── Open / Close ──────────────────────────────────────────────────────────────

bool Database::Open(const std::string& path) {
    m_path = path;
    m_open = false;
    m_tables.clear();

    std::ifstream f(path);
    if (!f) {
        // New database — will be created on Save().
        m_open = true;
        ensureKVTable();
        return true;
    }

    std::ostringstream ss;
    ss << f.rdbuf();
    std::string json = ss.str();

    // Parse: {"tables":[{"name":"...","columns":[...],"rows":[...]}, ...]}
    size_t tablesPos = json.find("\"tables\"");
    if (tablesPos == std::string::npos) {
        m_open = true;
        ensureKVTable();
        return true;
    }

    // Walk through table objects.
    size_t pos = json.find('[', tablesPos);
    while (pos != std::string::npos) {
        size_t objStart = json.find('{', pos + 1);
        if (objStart == std::string::npos) break;

        // Find matching closing brace (shallow scan for the table object).
        int depth = 1;
        size_t p = objStart + 1;
        while (p < json.size() && depth > 0) {
            if (json[p] == '{') ++depth;
            else if (json[p] == '}') --depth;
            ++p;
        }
        if (depth != 0) break;
        std::string tableJson = json.substr(objStart, p - objStart);

        DbTable tbl;
        tbl.name = extractVal(tableJson, "name");
        if (tbl.name.empty()) { pos = p; continue; }

        // Parse columns array.
        size_t colsPos = tableJson.find("\"columns\"");
        if (colsPos != std::string::npos) {
            size_t colArr = tableJson.find('[', colsPos);
            size_t colEnd = tableJson.find(']', colArr);
            if (colArr != std::string::npos && colEnd != std::string::npos) {
                std::string colsJson = tableJson.substr(colArr, colEnd - colArr + 1);
                size_t cp = 0;
                while ((cp = colsJson.find('{', cp)) != std::string::npos) {
                    size_t ce = colsJson.find('}', cp);
                    if (ce == std::string::npos) break;
                    std::string co = colsJson.substr(cp, ce - cp + 1);
                    DbColumn col;
                    col.name       = extractVal(co, "name");
                    col.type       = extractVal(co, "type");
                    col.primaryKey = extractBool(co, "primaryKey");
                    col.notNull    = extractBool(co, "notNull");
                    col.defaultVal = extractVal(co, "defaultVal");
                    if (!col.name.empty()) tbl.columns.push_back(col);
                    cp = ce + 1;
                }
            }
        }

        // Parse rows array.
        size_t rowsPos = tableJson.find("\"rows\"");
        if (rowsPos != std::string::npos) {
            size_t rowArr = tableJson.find('[', rowsPos);
            if (rowArr != std::string::npos) {
                size_t rp = rowArr + 1;
                while ((rp = tableJson.find('{', rp)) != std::string::npos) {
                    size_t re = tableJson.find('}', rp);
                    if (re == std::string::npos) break;
                    std::string rowJson = tableJson.substr(rp, re - rp + 1);
                    // Parse each key:"value" pair in the row object.
                    DbRow row;
                    size_t kp = 1;
                    while (kp < rowJson.size()) {
                        kp = rowJson.find('"', kp);
                        if (kp == std::string::npos) break;
                        ++kp;
                        std::string k;
                        while (kp < rowJson.size() && rowJson[kp] != '"') k += rowJson[kp++];
                        ++kp; // skip closing "
                        kp = rowJson.find(':', kp);
                        if (kp == std::string::npos) break;
                        ++kp;
                        while (kp < rowJson.size() && rowJson[kp] == ' ') ++kp;
                        std::string v;
                        if (kp < rowJson.size() && rowJson[kp] == '"') {
                            ++kp;
                            while (kp < rowJson.size() && rowJson[kp] != '"') {
                                if (rowJson[kp] == '\\' && kp + 1 < rowJson.size()) { ++kp; }
                                v += rowJson[kp++];
                            }
                            ++kp;
                        } else {
                            while (kp < rowJson.size() && rowJson[kp] != ',' &&
                                   rowJson[kp] != '}') v += rowJson[kp++];
                            while (!v.empty() && (v.back() == ' ' || v.back() == '\r'))
                                v.pop_back();
                        }
                        if (!k.empty()) row[k] = v;
                    }
                    if (!row.empty()) tbl.rows.push_back(row);
                    rp = re + 1;
                }
            }
        }

        m_tables[tbl.name] = std::move(tbl);
        pos = p;
    }

    m_open = true;
    ensureKVTable();
    return true;
}

bool Database::IsOpen() const { return m_open; }

void Database::Close() {
    m_open = false;
    m_tables.clear();
    m_path.clear();
}

// ── Save ──────────────────────────────────────────────────────────────────────

bool Database::Save() const {
    if (!m_open) return false;

    namespace fs = std::filesystem;
    fs::path dir = fs::path(m_path).parent_path();
    if (!dir.empty()) {
        std::error_code ec;
        fs::create_directories(dir, ec);
    }

    std::ofstream f(m_path);
    if (!f) return false;

    f << "{\"tables\":[\n";
    bool firstTbl = true;
    for (auto& [tname, tbl] : m_tables) {
        if (!firstTbl) f << ",\n";
        firstTbl = false;
        f << "  {\"name\":\"" << esc(tbl.name) << "\",\n";

        // columns
        f << "   \"columns\":[";
        bool firstCol = true;
        for (auto& col : tbl.columns) {
            if (!firstCol) f << ",";
            firstCol = false;
            f << "{\"name\":\"" << esc(col.name)       << "\","
              << "\"type\":\""  << esc(col.type)        << "\","
              << "\"primaryKey\":" << (col.primaryKey ? "true" : "false") << ","
              << "\"notNull\":"    << (col.notNull    ? "true" : "false") << ","
              << "\"defaultVal\":\"" << esc(col.defaultVal) << "\"}";
        }
        f << "],\n";

        // rows
        f << "   \"rows\":[";
        bool firstRow = true;
        for (auto& row : tbl.rows) {
            if (!firstRow) f << ",";
            firstRow = false;
            f << "{";
            bool firstKV = true;
            for (auto& [k, v] : row) {
                if (!firstKV) f << ",";
                firstKV = false;
                f << "\"" << esc(k) << "\":\"" << esc(v) << "\"";
            }
            f << "}";
        }
        f << "]\n  }";
    }
    f << "\n]}\n";
    return f.good();
}

// ── DDL ───────────────────────────────────────────────────────────────────────

bool Database::CreateTable(const std::string& name, const std::vector<DbColumn>& columns) {
    if (!m_open) return false;
    if (m_tables.count(name)) return false;
    DbTable tbl;
    tbl.name    = name;
    tbl.columns = columns;
    m_tables[name] = std::move(tbl);
    return true;
}

bool Database::DropTable(const std::string& name) {
    return m_tables.erase(name) > 0;
}

bool Database::TableExists(const std::string& name) const {
    return m_tables.count(name) > 0;
}

std::vector<std::string> Database::ListTables() const {
    std::vector<std::string> names;
    for (auto& [n, _] : m_tables) names.push_back(n);
    return names;
}

size_t Database::TableCount() const { return m_tables.size(); }

// ── DML helpers ───────────────────────────────────────────────────────────────

bool Database::rowMatchesWhere(const DbRow& row, const DbRow& where) const {
    for (auto& [k, v] : where) {
        auto it = row.find(k);
        if (it == row.end() || it->second != v) return false;
    }
    return true;
}

// ── DML ───────────────────────────────────────────────────────────────────────

bool Database::Insert(const std::string& table, const DbRow& row) {
    auto it = m_tables.find(table);
    if (it == m_tables.end()) return false;
    it->second.rows.push_back(row);
    return true;
}

bool Database::Update(const std::string& table, const DbRow& set, const DbRow& where) {
    auto it = m_tables.find(table);
    if (it == m_tables.end()) return false;
    bool any = false;
    for (auto& row : it->second.rows) {
        if (!rowMatchesWhere(row, where)) continue;
        for (auto& [k, v] : set) row[k] = v;
        any = true;
    }
    return any;
}

bool Database::Delete(const std::string& table, const DbRow& where) {
    auto it = m_tables.find(table);
    if (it == m_tables.end()) return false;
    auto& rows = it->second.rows;
    size_t before = rows.size();
    rows.erase(
        std::remove_if(rows.begin(), rows.end(),
                       [&](const DbRow& r) { return rowMatchesWhere(r, where); }),
        rows.end());
    return rows.size() < before;
}

// ── Queries ───────────────────────────────────────────────────────────────────

DbResult Database::Select(const std::string& table, const DbRow& where) const {
    DbResult result;
    auto it = m_tables.find(table);
    if (it == m_tables.end()) return result;
    for (auto& row : it->second.rows)
        if (rowMatchesWhere(row, where)) result.push_back(row);
    return result;
}

DbResult Database::SelectAll(const std::string& table) const {
    auto it = m_tables.find(table);
    if (it == m_tables.end()) return {};
    return it->second.rows;
}

std::optional<DbRow> Database::SelectOne(const std::string& table, const DbRow& where) const {
    auto it = m_tables.find(table);
    if (it == m_tables.end()) return std::nullopt;
    for (auto& row : it->second.rows)
        if (rowMatchesWhere(row, where)) return row;
    return std::nullopt;
}

size_t Database::RowCount(const std::string& table) const {
    auto it = m_tables.find(table);
    if (it == m_tables.end()) return 0;
    return it->second.rows.size();
}

// ── KV store ──────────────────────────────────────────────────────────────────

bool Database::ensureKVTable() {
    if (TableExists("_kv")) return true;
    return CreateTable("_kv", {
        {"key",   "TEXT", true,  true,  ""},
        {"value", "TEXT", false, false, ""}
    });
}

void Database::KVSet(const std::string& key, const std::string& value) {
    ensureKVTable();
    if (Update("_kv", {{"value", value}}, {{"key", key}})) return;
    Insert("_kv", {{"key", key}, {"value", value}});
}

std::string Database::KVGet(const std::string& key, const std::string& defaultVal) const {
    auto row = SelectOne("_kv", {{"key", key}});
    if (!row) return defaultVal;
    auto it = row->find("value");
    return it != row->end() ? it->second : defaultVal;
}

bool Database::KVHas(const std::string& key) const {
    return SelectOne("_kv", {{"key", key}}).has_value();
}

bool Database::KVDelete(const std::string& key) {
    return Delete("_kv", {{"key", key}});
}

} // namespace Core
