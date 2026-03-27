#include "Core/DataTable/DataTableSystem/DataTableSystem.h"
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Core {

struct TableData {
    std::vector<std::string>              headers;
    std::unordered_map<std::string,uint32_t> colIndex;
    std::vector<std::vector<std::string>> rows;
    std::function<void(uint32_t)>         onLoad;
};

static std::vector<std::string> SplitCSVRow(const std::string& line){
    std::vector<std::string> out;
    std::string cur; bool inQ=false;
    for(char c:line){
        if(c=='"'){ inQ=!inQ; continue; }
        if(c==','&&!inQ){ out.push_back(cur); cur.clear(); continue; }
        cur+=c;
    }
    out.push_back(cur);
    return out;
}

struct DataTableSystem::Impl {
    std::unordered_map<uint32_t,TableData> tables;
    TableData* Find(uint32_t id){ auto it=tables.find(id); return it!=tables.end()?&it->second:nullptr; }
};

DataTableSystem::DataTableSystem(): m_impl(new Impl){}
DataTableSystem::~DataTableSystem(){ Shutdown(); delete m_impl; }
void DataTableSystem::Init(){}
void DataTableSystem::Shutdown(){ m_impl->tables.clear(); }
void DataTableSystem::Reset(){ m_impl->tables.clear(); }

void DataTableSystem::AddTable   (uint32_t id){ m_impl->tables[id]=TableData{}; }
void DataTableSystem::RemoveTable(uint32_t id){ m_impl->tables.erase(id); }

bool DataTableSystem::LoadCSV(uint32_t id, const std::string& csv){
    auto* t=m_impl->Find(id); if(!t) return false;
    t->headers.clear(); t->rows.clear(); t->colIndex.clear();
    std::istringstream ss(csv);
    std::string line; bool first=true;
    while(std::getline(ss,line)){
        if(line.empty()) continue;
        // Strip CR
        if(!line.empty()&&line.back()=='\r') line.pop_back();
        auto cols=SplitCSVRow(line);
        if(first){ first=false;
            t->headers=cols;
            for(uint32_t i=0;i<cols.size();i++) t->colIndex[cols[i]]=i;
            continue;
        }
        t->rows.push_back(cols);
    }
    if(t->onLoad) t->onLoad((uint32_t)t->rows.size());
    return true;
}

uint32_t DataTableSystem::GetRowCount(uint32_t id) const {
    auto* t=m_impl->Find(id); return t?(uint32_t)t->rows.size():0;
}
uint32_t DataTableSystem::GetColumnCount(uint32_t id) const {
    auto* t=m_impl->Find(id); return t?(uint32_t)t->headers.size():0;
}
uint32_t DataTableSystem::GetColumnNames(uint32_t id, std::vector<std::string>& out) const {
    auto* t=m_impl->Find(id); if(!t) return 0; out=t->headers; return (uint32_t)out.size();
}

std::string DataTableSystem::GetString(uint32_t id, uint32_t row, uint32_t col) const {
    auto* t=m_impl->Find(id); if(!t||row>=t->rows.size()) return "";
    if(col>=t->rows[row].size()) return "";
    return t->rows[row][col];
}
std::string DataTableSystem::GetStringByName(uint32_t id, uint32_t row, const std::string& name) const {
    auto* t=m_impl->Find(id); if(!t) return "";
    auto it=t->colIndex.find(name); if(it==t->colIndex.end()) return "";
    return GetString(id,row,it->second);
}
int32_t DataTableSystem::GetInt(uint32_t id,uint32_t row,uint32_t col) const {
    return std::strtol(GetString(id,row,col).c_str(),nullptr,10);
}
float DataTableSystem::GetFloat(uint32_t id,uint32_t row,uint32_t col) const {
    return std::strtof(GetString(id,row,col).c_str(),nullptr);
}

int32_t DataTableSystem::FindRow(uint32_t id,uint32_t col,const std::string& val) const {
    auto* t=m_impl->Find(id); if(!t) return -1;
    for(uint32_t r=0;r<t->rows.size();r++)
        if(col<t->rows[r].size()&&t->rows[r][col]==val) return (int32_t)r;
    return -1;
}
uint32_t DataTableSystem::FindAllRows(uint32_t id,uint32_t col,const std::string& val,
                                       std::vector<uint32_t>& out) const {
    out.clear(); auto* t=m_impl->Find(id); if(!t) return 0;
    for(uint32_t r=0;r<t->rows.size();r++)
        if(col<t->rows[r].size()&&t->rows[r][col]==val) out.push_back(r);
    return (uint32_t)out.size();
}

void DataTableSystem::AddRow(uint32_t id,const std::vector<std::string>& vals){
    auto* t=m_impl->Find(id); if(t) t->rows.push_back(vals);
}
void DataTableSystem::RemoveRow(uint32_t id,uint32_t row){
    auto* t=m_impl->Find(id); if(!t||row>=t->rows.size()) return;
    t->rows.erase(t->rows.begin()+row);
}

void DataTableSystem::SetOnLoad(uint32_t id,std::function<void(uint32_t)> cb){
    auto* t=m_impl->Find(id); if(t) t->onLoad=cb;
}

} // namespace Core
