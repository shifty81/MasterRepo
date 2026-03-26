#pragma once
/**
 * @file DataTableSystem.h
 * @brief Row-based data tables for game configuration and balance data.
 *
 * A DataTable is a named collection of typed rows identified by a string key.
 * Tables are loaded from CSV or JSON and queried at runtime by row key + column.
 * Supports:
 *   - Multiple tables (weapons, enemies, items, upgrades…)
 *   - Strong-typed column access (int, float, bool, string, vec3)
 *   - Hot-reload from disk
 *   - Merge / override from patch files (for DLC / modding)
 *
 * Typical usage:
 * @code
 *   DataTableSystem dts;
 *   dts.Init();
 *   dts.LoadTable("weapons", "Data/Tables/weapons.csv");
 *   float damage = dts.GetFloat("weapons", "plasma_rifle", "base_damage", 10.f);
 *   std::string name = dts.GetString("weapons", "plasma_rifle", "display_name");
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Core {

// ── Column type ───────────────────────────────────────────────────────────────

enum class DataColumnType : uint8_t {
    String = 0, Int32, Float, Bool, Vec2, Vec3
};

// ── Column definition ─────────────────────────────────────────────────────────

struct DataColumn {
    std::string    name;
    DataColumnType type{DataColumnType::String};
};

// ── A single cell ─────────────────────────────────────────────────────────────

struct DataCell {
    std::string strVal;
    int32_t     intVal{0};
    float       floatVal{0.f};
    bool        boolVal{false};
    float       vecVal[3]{};
    DataColumnType type{DataColumnType::String};
};

// ── Row ───────────────────────────────────────────────────────────────────────

struct DataRow {
    std::string                              key;
    std::unordered_map<std::string,DataCell> cells; ///< column name → cell
};

// ── Table metadata ────────────────────────────────────────────────────────────

struct DataTableInfo {
    std::string              name;
    std::string              sourcePath;
    std::vector<DataColumn>  columns;
    uint32_t                 rowCount{0};
};

// ── DataTableSystem ───────────────────────────────────────────────────────────

class DataTableSystem {
public:
    DataTableSystem();
    ~DataTableSystem();

    void Init();
    void Shutdown();

    // ── Loading ───────────────────────────────────────────────────────────────

    bool LoadTable(const std::string& name, const std::string& csvPath);
    bool LoadTableJSON(const std::string& name, const std::string& jsonPath);
    bool ReloadTable(const std::string& name);
    bool MergeTable(const std::string& name, const std::string& patchPath);

    bool HasTable(const std::string& name) const;
    void RemoveTable(const std::string& name);
    DataTableInfo GetTableInfo(const std::string& name) const;
    std::vector<DataTableInfo> AllTables() const;

    // ── Row queries ───────────────────────────────────────────────────────────

    bool HasRow(const std::string& table, const std::string& key) const;
    DataRow GetRow(const std::string& table, const std::string& key) const;
    std::vector<std::string> RowKeys(const std::string& table) const;

    // ── Typed accessors ───────────────────────────────────────────────────────

    std::string GetString(const std::string& table, const std::string& rowKey,
                          const std::string& col,
                          const std::string& defaultVal = "") const;

    int32_t     GetInt(const std::string& table, const std::string& rowKey,
                       const std::string& col, int32_t defaultVal = 0) const;

    float       GetFloat(const std::string& table, const std::string& rowKey,
                         const std::string& col, float defaultVal = 0.f) const;

    bool        GetBool(const std::string& table, const std::string& rowKey,
                        const std::string& col, bool defaultVal = false) const;

    void        GetVec3(const std::string& table, const std::string& rowKey,
                        const std::string& col, float out[3]) const;

    // ── Serialization ─────────────────────────────────────────────────────────

    bool SaveTable(const std::string& name, const std::string& csvPath) const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnTableReloaded(std::function<void(const std::string& name)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Core
