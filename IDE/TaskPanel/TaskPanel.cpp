#include "IDE/TaskPanel/TaskPanel.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cctype>
#include <unordered_map>

namespace IDE {

const char* TaskMarkerName(TaskMarker m) {
    switch (m) {
    case TaskMarker::TODO:     return "TODO";
    case TaskMarker::FIXME:    return "FIXME";
    case TaskMarker::HACK:     return "HACK";
    case TaskMarker::NOTE:     return "NOTE";
    case TaskMarker::OPTIMIZE: return "OPTIMIZE";
    case TaskMarker::REVIEW:   return "REVIEW";
    case TaskMarker::BUG:      return "BUG";
    default:                   return "UNKNOWN";
    }
}

static uint64_t tpNowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct MarkerConfig {
    std::string  keyword;
    TaskMarker   marker;
    TaskPriority defaultPriority;
};

struct TaskPanel::Impl {
    std::vector<std::string>     roots;
    std::vector<std::string>     extensions;
    std::vector<MarkerConfig>    markers;
    std::vector<TaskItem>        tasks;
    TaskPanel::SortBy            sortBy{TaskPanel::SortBy::File};
    bool                         sortDesc{false};
    JumpToFileCb                 jumpCb;

    void sortTasks() {
        std::stable_sort(tasks.begin(), tasks.end(),
            [this](const TaskItem& a, const TaskItem& b) {
                bool less = false;
                switch (sortBy) {
                case TaskPanel::SortBy::File:     less = a.filePath < b.filePath; break;
                case TaskPanel::SortBy::Line:     less = (a.filePath < b.filePath) ||
                                                         (a.filePath == b.filePath && a.line < b.line); break;
                case TaskPanel::SortBy::Priority: less = a.priority > b.priority; break;
                case TaskPanel::SortBy::Marker:   less = static_cast<int>(a.marker) < static_cast<int>(b.marker); break;
                }
                return sortDesc ? !less : less;
            });
    }

    size_t scanFile(const std::string& path) {
        // Remove existing tasks for this file
        tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
            [&](const TaskItem& t){ return t.filePath == path; }), tasks.end());

        std::ifstream f(path);
        if (!f.is_open()) return 0;

        size_t found = 0;
        std::string line;
        uint32_t lineNo = 0;
        uint64_t ts = tpNowMs();

        while (std::getline(f, line)) {
            ++lineNo;
            for (const auto& mc : markers) {
                auto pos = line.find(mc.keyword);
                if (pos == std::string::npos) continue;
                TaskItem item;
                item.filePath  = path;
                item.line      = lineNo;
                item.column    = static_cast<uint32_t>(pos) + 1;
                item.marker    = mc.marker;
                item.priority  = mc.defaultPriority;
                item.scanTime  = ts;
                // Extract message after the keyword
                size_t msgStart = pos + mc.keyword.size();
                while (msgStart < line.size() && (line[msgStart] == ':' ||
                       line[msgStart] == ' ' || line[msgStart] == '\t'))
                    ++msgStart;
                item.message = line.substr(msgStart);
                // Trim trailing whitespace
                while (!item.message.empty() &&
                       std::isspace(static_cast<unsigned char>(item.message.back())))
                    item.message.pop_back();
                // Extract author: look for @author
                auto authPos = item.message.find("@");
                if (authPos != std::string::npos) {
                    item.author  = item.message.substr(authPos + 1);
                    auto sp = item.author.find(' ');
                    if (sp != std::string::npos) item.author = item.author.substr(0, sp);
                    item.message = item.message.substr(0, authPos);
                    while (!item.message.empty() &&
                           std::isspace(static_cast<unsigned char>(item.message.back())))
                        item.message.pop_back();
                }
                tasks.push_back(std::move(item));
                ++found;
                break; // one marker per position
            }
        }
        return found;
    }
};

TaskPanel::TaskPanel() : m_impl(new Impl()) {
    // Default extensions
    for (const char* e : {".cpp",".cxx",".cc",".c",".h",".hpp",".hxx",
                           ".lua",".py",".js",".ts",".cs"})
        m_impl->extensions.push_back(e);
    // Default markers
    m_impl->markers = {
        {"TODO",     TaskMarker::TODO,     TaskPriority::Medium},
        {"FIXME",    TaskMarker::FIXME,    TaskPriority::High},
        {"HACK",     TaskMarker::HACK,     TaskPriority::Medium},
        {"NOTE",     TaskMarker::NOTE,     TaskPriority::Low},
        {"OPTIMIZE", TaskMarker::OPTIMIZE, TaskPriority::Low},
        {"REVIEW",   TaskMarker::REVIEW,   TaskPriority::Medium},
        {"BUG",      TaskMarker::BUG,      TaskPriority::Critical},
    };
}
TaskPanel::~TaskPanel() { delete m_impl; }

void TaskPanel::AddScanRoot(const std::string& dir) {
    if (std::find(m_impl->roots.begin(), m_impl->roots.end(), dir) == m_impl->roots.end())
        m_impl->roots.push_back(dir);
}
void TaskPanel::RemoveScanRoot(const std::string& dir) {
    m_impl->roots.erase(std::remove(m_impl->roots.begin(), m_impl->roots.end(), dir),
                        m_impl->roots.end());
}
void TaskPanel::ClearScanRoots()  { m_impl->roots.clear(); }

void TaskPanel::AddExtension(const std::string& ext) {
    if (std::find(m_impl->extensions.begin(), m_impl->extensions.end(), ext) ==
        m_impl->extensions.end())
        m_impl->extensions.push_back(ext);
}

void TaskPanel::AddMarkerKeyword(const std::string& kw, TaskMarker marker,
                                   TaskPriority defaultPri)
{
    m_impl->markers.push_back({kw, marker, defaultPri});
}

size_t TaskPanel::Refresh() {
    m_impl->tasks.clear();
    size_t total = 0;
    namespace fs = std::filesystem;
    for (const auto& root : m_impl->roots) {
        if (!fs::exists(root)) continue;
        for (const auto& entry : fs::recursive_directory_iterator(root,
             fs::directory_options::skip_permission_denied))
        {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            bool match = false;
            for (const auto& e : m_impl->extensions)
                if (e == ext) { match = true; break; }
            if (match) total += m_impl->scanFile(entry.path().string());
        }
    }
    m_impl->sortTasks();
    return total;
}

size_t TaskPanel::RefreshFile(const std::string& path) {
    size_t n = m_impl->scanFile(path);
    m_impl->sortTasks();
    return n;
}

std::vector<TaskItem> TaskPanel::GetAll() const { return m_impl->tasks; }

std::vector<TaskItem> TaskPanel::GetFiltered(const TaskFilter& filter) const {
    std::vector<TaskItem> out;
    for (const auto& t : m_impl->tasks) {
        if (filter.markerFilter && t.marker != *filter.markerFilter) continue;
        if (filter.priorityMin  && t.priority < *filter.priorityMin)   continue;
        if (!filter.fileContains.empty() &&
            t.filePath.find(filter.fileContains) == std::string::npos) continue;
        if (!filter.messageContains.empty() &&
            t.message.find(filter.messageContains) == std::string::npos) continue;
        out.push_back(t);
    }
    return out;
}

std::vector<TaskItem> TaskPanel::GetByFile(const std::string& filePath) const {
    std::vector<TaskItem> out;
    for (const auto& t : m_impl->tasks)
        if (t.filePath == filePath) out.push_back(t);
    return out;
}

size_t TaskPanel::TotalCount() const { return m_impl->tasks.size(); }

size_t TaskPanel::CountByMarker(TaskMarker m) const {
    size_t n = 0;
    for (const auto& t : m_impl->tasks) if (t.marker == m) ++n;
    return n;
}

void TaskPanel::SetSortOrder(SortBy sort, bool descending) {
    m_impl->sortBy   = sort;
    m_impl->sortDesc = descending;
    m_impl->sortTasks();
}

void TaskPanel::SetJumpCallback(JumpToFileCb cb) { m_impl->jumpCb = std::move(cb); }

void TaskPanel::JumpTo(const TaskItem& item) {
    if (m_impl->jumpCb) m_impl->jumpCb(item.filePath, item.line);
}

std::string TaskPanel::ExportText(const TaskFilter& filter) const {
    auto items = GetFiltered(filter);
    std::ostringstream ss;
    for (const auto& t : items) {
        ss << t.filePath << ":" << t.line << " [" << t.MarkerName() << "] " << t.message;
        if (!t.author.empty()) ss << " @" << t.author;
        ss << "\n";
    }
    return ss.str();
}

std::string TaskPanel::ExportCSV(const TaskFilter& filter) const {
    auto items = GetFiltered(filter);
    std::ostringstream ss;
    ss << "file,line,column,marker,priority,message,author\n";
    for (const auto& t : items) {
        ss << t.filePath << "," << t.line << "," << t.column << ","
           << t.MarkerName() << "," << static_cast<int>(t.priority) << ","
           << "\"" << t.message << "\","
           << t.author << "\n";
    }
    return ss.str();
}

} // namespace IDE
