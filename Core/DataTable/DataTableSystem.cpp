#include "Core/DataTable/DataTableSystem.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Core {

// ── CSV parser ────────────────────────────────────────────────────────────────

static std::vector<std::string> SplitCSV(const std::string& line) {
    std::vector<std::string> cols;
    std::string cur;
    bool inQuote=false;
    for(char c:line){
        if(c=='"'){ inQuote=!inQuote; }
        else if(c==','&&!inQuote){ cols.push_back(cur); cur.clear(); }
        else { cur+=c; }
    }
    cols.push_back(cur);
    return cols;
}

static DataCell ParseCell(const std::string& val, DataColumnType type) {
    DataCell c; c.type=type;
    try {
        switch(type){
            case DataColumnType::String:  c.strVal=val; break;
            case DataColumnType::Int32:   c.intVal=std::stoi(val); break;
            case DataColumnType::Float:   c.floatVal=std::stof(val); break;
            case DataColumnType::Bool:    c.boolVal=(val=="true"||val=="1"||val=="yes"); break;
            case DataColumnType::Vec2: case DataColumnType::Vec3: {
                std::istringstream ss(val); char comma;
                ss>>c.vecVal[0]>>comma>>c.vecVal[1];
                if(type==DataColumnType::Vec3) ss>>comma>>c.vecVal[2];
                break;
            }
        }
    } catch(...) { c.strVal=val; }
    return c;
}

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct TableData {
    DataTableInfo             info;
    std::vector<DataRow>      rows;
    std::unordered_map<std::string,size_t> rowIndex;
};

struct DataTableSystem::Impl {
    std::unordered_map<std::string,TableData> tables;
    std::function<void(const std::string&)>   onReloaded;

    bool ParseCSV(TableData& td, const std::string& path) {
        std::ifstream f(path); if(!f.is_open()) return false;
        std::string headerLine;
        if(!std::getline(f,headerLine)) return false;
        auto headers=SplitCSV(headerLine);
        td.info.columns.clear();
        for(auto& h:headers){ DataColumn dc; dc.name=h; dc.type=DataColumnType::String; td.info.columns.push_back(dc); }
        td.rows.clear(); td.rowIndex.clear();
        std::string line;
        while(std::getline(f,line)){
            if(line.empty()) continue;
            auto cols=SplitCSV(line);
            DataRow row;
            row.key=cols.empty()?"":cols[0];
            for(size_t i=0;i<td.info.columns.size()&&i<cols.size();++i){
                DataCell cell=ParseCell(cols[i], td.info.columns[i].type);
                row.cells[td.info.columns[i].name]=cell;
            }
            td.rowIndex[row.key]=td.rows.size();
            td.rows.push_back(std::move(row));
        }
        td.info.rowCount=static_cast<uint32_t>(td.rows.size());
        return true;
    }
};

DataTableSystem::DataTableSystem() : m_impl(new Impl()) {}
DataTableSystem::~DataTableSystem() { delete m_impl; }
void DataTableSystem::Init()     {}
void DataTableSystem::Shutdown() { m_impl->tables.clear(); }

bool DataTableSystem::LoadTable(const std::string& name, const std::string& path) {
    TableData td; td.info.name=name; td.info.sourcePath=path;
    if(!m_impl->ParseCSV(td,path)) return false;
    m_impl->tables[name]=std::move(td);
    return true;
}

bool DataTableSystem::LoadTableJSON(const std::string& name, const std::string& path) {
    std::ifstream f(path); if(!f.is_open()) return false;
    TableData td; td.info.name=name; td.info.sourcePath=path;
    m_impl->tables[name]=std::move(td);
    return true;
}

bool DataTableSystem::ReloadTable(const std::string& name) {
    auto it=m_impl->tables.find(name); if(it==m_impl->tables.end()) return false;
    bool ok=LoadTable(name, it->second.info.sourcePath);
    if(ok&&m_impl->onReloaded) m_impl->onReloaded(name);
    return ok;
}

bool DataTableSystem::MergeTable(const std::string& name, const std::string& path) {
    auto it=m_impl->tables.find(name); if(it==m_impl->tables.end()) return LoadTable(name,path);
    TableData patch; patch.info.name=name; patch.info.sourcePath=path;
    if(!m_impl->ParseCSV(patch,path)) return false;
    for(auto& row:patch.rows){
        auto rit=it->second.rowIndex.find(row.key);
        if(rit!=it->second.rowIndex.end())
            for(auto&[col,cell]:row.cells) it->second.rows[rit->second].cells[col]=cell;
        else{ it->second.rowIndex[row.key]=it->second.rows.size(); it->second.rows.push_back(row); }
    }
    it->second.info.rowCount=static_cast<uint32_t>(it->second.rows.size());
    return true;
}

bool DataTableSystem::HasTable(const std::string& n) const { return m_impl->tables.count(n)>0; }
void DataTableSystem::RemoveTable(const std::string& n) { m_impl->tables.erase(n); }

DataTableInfo DataTableSystem::GetTableInfo(const std::string& n) const {
    auto it=m_impl->tables.find(n); return it!=m_impl->tables.end()?it->second.info:DataTableInfo{};
}
std::vector<DataTableInfo> DataTableSystem::AllTables() const {
    std::vector<DataTableInfo> out; for(auto&[k,v]:m_impl->tables) out.push_back(v.info); return out;
}

bool DataTableSystem::HasRow(const std::string& t, const std::string& key) const {
    auto it=m_impl->tables.find(t); return it!=m_impl->tables.end()&&it->second.rowIndex.count(key)>0;
}
DataRow DataTableSystem::GetRow(const std::string& t, const std::string& key) const {
    auto it=m_impl->tables.find(t); if(it==m_impl->tables.end()) return {};
    auto rit=it->second.rowIndex.find(key); if(rit==it->second.rowIndex.end()) return {};
    return it->second.rows[rit->second];
}
std::vector<std::string> DataTableSystem::RowKeys(const std::string& t) const {
    auto it=m_impl->tables.find(t); if(it==m_impl->tables.end()) return {};
    std::vector<std::string> keys; for(auto&[k,_]:it->second.rowIndex) keys.push_back(k); return keys;
}

std::string DataTableSystem::GetString(const std::string& t,const std::string& r,const std::string& c,const std::string& def) const {
    auto it=m_impl->tables.find(t); if(it==m_impl->tables.end()) return def;
    auto rit=it->second.rowIndex.find(r); if(rit==it->second.rowIndex.end()) return def;
    auto& row=it->second.rows[rit->second];
    auto cit=row.cells.find(c); return cit!=row.cells.end()?cit->second.strVal:def;
}
int32_t DataTableSystem::GetInt(const std::string& t,const std::string& r,const std::string& c,int32_t def) const {
    auto it=m_impl->tables.find(t); if(it==m_impl->tables.end()) return def;
    auto rit=it->second.rowIndex.find(r); if(rit==it->second.rowIndex.end()) return def;
    auto cit=it->second.rows[rit->second].cells.find(c); if(cit==it->second.rows[rit->second].cells.end()) return def;
    return cit->second.type==DataColumnType::Int32?cit->second.intVal:(int32_t)cit->second.floatVal;
}
float DataTableSystem::GetFloat(const std::string& t,const std::string& r,const std::string& c,float def) const {
    auto it=m_impl->tables.find(t); if(it==m_impl->tables.end()) return def;
    auto rit=it->second.rowIndex.find(r); if(rit==it->second.rowIndex.end()) return def;
    auto cit=it->second.rows[rit->second].cells.find(c); if(cit==it->second.rows[rit->second].cells.end()) return def;
    return cit->second.type==DataColumnType::Float?cit->second.floatVal:(float)cit->second.intVal;
}
bool DataTableSystem::GetBool(const std::string& t,const std::string& r,const std::string& c,bool def) const {
    auto it=m_impl->tables.find(t); if(it==m_impl->tables.end()) return def;
    auto rit=it->second.rowIndex.find(r); if(rit==it->second.rowIndex.end()) return def;
    auto cit=it->second.rows[rit->second].cells.find(c); return cit!=it->second.rows[rit->second].cells.end()?cit->second.boolVal:def;
}
void DataTableSystem::GetVec3(const std::string& t,const std::string& r,const std::string& c,float out[3]) const {
    auto it=m_impl->tables.find(t); if(it==m_impl->tables.end()) return;
    auto rit=it->second.rowIndex.find(r); if(rit==it->second.rowIndex.end()) return;
    auto cit=it->second.rows[rit->second].cells.find(c);
    if(cit!=it->second.rows[rit->second].cells.end()){ out[0]=cit->second.vecVal[0];out[1]=cit->second.vecVal[1];out[2]=cit->second.vecVal[2]; }
}

bool DataTableSystem::SaveTable(const std::string& name, const std::string& path) const {
    auto it=m_impl->tables.find(name); if(it==m_impl->tables.end()) return false;
    std::ofstream f(path); if(!f.is_open()) return false;
    auto& td=it->second;
    for(size_t i=0;i<td.info.columns.size();++i){ if(i) f<<","; f<<td.info.columns[i].name; } f<<"\n";
    for(auto& row:td.rows){
        bool first=true;
        for(auto& col:td.info.columns){
            if(!first) f<<","; first=false;
            auto cit=row.cells.find(col.name);
            if(cit!=row.cells.end()) f<<cit->second.strVal;
        }
        f<<"\n";
    }
    return true;
}

void DataTableSystem::OnTableReloaded(std::function<void(const std::string&)> cb) {
    m_impl->onReloaded=std::move(cb);
}

} // namespace Core
