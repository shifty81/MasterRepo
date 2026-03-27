#pragma once
/**
 * @file DataTableSystem.h
 * @brief CSV/JSON-style runtime data tables with typed column access and row queries.
 *
 * Features:
 *   - LoadCSV(tableId, csvText) → bool: parse comma-separated rows, first row = headers
 *   - GetRowCount(tableId) → uint32_t
 *   - GetColumnCount(tableId) → uint32_t
 *   - GetColumnNames(tableId, outNames[]) → uint32_t
 *   - GetString(tableId, row, col) → string
 *   - GetInt(tableId, row, col) → int32_t
 *   - GetFloat(tableId, row, col) → float
 *   - FindRow(tableId, col, value) → int32_t: first row where col == value, -1 if not found
 *   - FindAllRows(tableId, col, value, out[]) → uint32_t
 *   - AddRow(tableId, values[]): append row
 *   - RemoveRow(tableId, row)
 *   - AddTable(tableId) / RemoveTable(tableId)
 *   - SetOnLoad(tableId, cb): callback(rowCount) after CSV load
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Core {

class DataTableSystem {
public:
    DataTableSystem();
    ~DataTableSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Table management
    void AddTable   (uint32_t tableId);
    void RemoveTable(uint32_t tableId);

    // Load
    bool LoadCSV(uint32_t tableId, const std::string& csvText);

    // Shape
    uint32_t GetRowCount   (uint32_t tableId) const;
    uint32_t GetColumnCount(uint32_t tableId) const;
    uint32_t GetColumnNames(uint32_t tableId, std::vector<std::string>& out) const;

    // Cell access
    std::string GetString(uint32_t tableId, uint32_t row, uint32_t col) const;
    std::string GetStringByName(uint32_t tableId, uint32_t row,
                                const std::string& colName) const;
    int32_t     GetInt   (uint32_t tableId, uint32_t row, uint32_t col) const;
    float       GetFloat (uint32_t tableId, uint32_t row, uint32_t col) const;

    // Queries
    int32_t  FindRow    (uint32_t tableId, uint32_t col,
                         const std::string& value) const;
    uint32_t FindAllRows(uint32_t tableId, uint32_t col,
                         const std::string& value,
                         std::vector<uint32_t>& out) const;

    // Mutation
    void AddRow   (uint32_t tableId, const std::vector<std::string>& values);
    void RemoveRow(uint32_t tableId, uint32_t row);

    // Callback
    void SetOnLoad(uint32_t tableId, std::function<void(uint32_t rowCount)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
