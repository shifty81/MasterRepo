#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional>
#include <cstdint>

namespace Core {

using DbRow    = std::map<std::string, std::string>;
using DbResult = std::vector<DbRow>;

struct DbColumn {
    std::string name;
    std::string type;
    bool        primaryKey = false;
    bool        notNull    = false;
    std::string defaultVal;
};

struct DbTable {
    std::string           name;
    std::vector<DbColumn> columns;
    std::vector<DbRow>    rows;
};

class Database {
public:
    bool Open(const std::string& path);
    bool IsOpen() const;
    void Close();
    bool Save() const;

    bool CreateTable(const std::string& name, const std::vector<DbColumn>& columns);
    bool DropTable(const std::string& name);
    bool TableExists(const std::string& name) const;
    std::vector<std::string> ListTables() const;

    bool Insert(const std::string& table, const DbRow& row);
    bool Update(const std::string& table, const DbRow& set, const DbRow& where);
    bool Delete(const std::string& table, const DbRow& where);

    DbResult Select(const std::string& table, const DbRow& where = {}) const;
    DbResult SelectAll(const std::string& table) const;
    std::optional<DbRow> SelectOne(const std::string& table, const DbRow& where) const;

    void        KVSet(const std::string& key, const std::string& value);
    std::string KVGet(const std::string& key, const std::string& defaultVal = "") const;
    bool        KVHas(const std::string& key) const;
    bool        KVDelete(const std::string& key);

    size_t RowCount(const std::string& table) const;
    size_t TableCount() const;

private:
    bool rowMatchesWhere(const DbRow& row, const DbRow& where) const;
    bool ensureKVTable();

    std::string                    m_path;
    bool                           m_open = false;
    std::map<std::string, DbTable> m_tables;
};

} // namespace Core
