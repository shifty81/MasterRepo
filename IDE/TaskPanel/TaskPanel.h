#pragma once
/**
 * @file TaskPanel.h
 * @brief IDE task/TODO scanner: extract TODO/FIXME/HACK/NOTE markers from source files.
 *
 * TaskPanel scans a set of source directories for comment annotations and presents
 * them as a sortable, filterable task list:
 *
 *   - TaskItem: file path, line number, column, marker type, message text, priority
 *   - TaskPanel: directory scan (recursive), filtering by tag/file/priority,
 *     sorting by priority/file/line; jump-to callback for IDE navigation
 *   - Refresh()/RefreshFile() for incremental rescans
 *   - Export to plain text / CSV
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace IDE {

// ── Marker types ─────────────────────────────────────────────────────────
enum class TaskMarker : uint8_t {
    TODO, FIXME, HACK, NOTE, OPTIMIZE, REVIEW, BUG
};

const char* TaskMarkerName(TaskMarker m);

// ── Priority ──────────────────────────────────────────────────────────────
enum class TaskPriority : uint8_t { Low = 0, Medium = 1, High = 2, Critical = 3 };

// ── Task item ─────────────────────────────────────────────────────────────
struct TaskItem {
    std::string  filePath;
    uint32_t     line{0};
    uint32_t     column{0};
    TaskMarker   marker{TaskMarker::TODO};
    TaskPriority priority{TaskPriority::Medium};
    std::string  message;
    std::string  author;   ///< Optional @author tag after marker
    uint64_t     scanTime{0};

    std::string MarkerName() const { return TaskMarkerName(marker); }
};

// ── Filter ────────────────────────────────────────────────────────────────
struct TaskFilter {
    std::optional<TaskMarker>  markerFilter;
    std::optional<TaskPriority> priorityMin;
    std::string  fileContains;   ///< Substring match on filePath
    std::string  messageContains;
};

// ── Jump callback ─────────────────────────────────────────────────────────
using JumpToFileCb = std::function<void(const std::string& file, uint32_t line)>;

// ── Panel ─────────────────────────────────────────────────────────────────
class TaskPanel {
public:
    TaskPanel();
    ~TaskPanel();

    // ── scan roots ────────────────────────────────────────────
    void AddScanRoot(const std::string& dir);
    void RemoveScanRoot(const std::string& dir);
    void ClearScanRoots();
    void AddExtension(const std::string& ext);  ///< e.g. ".cpp", ".h"
    void AddMarkerKeyword(const std::string& kw,
                          TaskMarker marker,
                          TaskPriority defaultPri);

    // ── scanning ──────────────────────────────────────────────
    /// Full rescan of all roots. Returns number of items found.
    size_t Refresh();
    /// Rescan a single file (incremental update).
    size_t RefreshFile(const std::string& path);

    // ── query ─────────────────────────────────────────────────
    std::vector<TaskItem> GetAll() const;
    std::vector<TaskItem> GetFiltered(const TaskFilter& filter) const;
    std::vector<TaskItem> GetByFile(const std::string& filePath) const;
    size_t TotalCount() const;
    size_t CountByMarker(TaskMarker m) const;

    // ── sort ──────────────────────────────────────────────────
    enum class SortBy { File, Line, Priority, Marker };
    void SetSortOrder(SortBy sort, bool descending = false);

    // ── navigation ────────────────────────────────────────────
    void SetJumpCallback(JumpToFileCb cb);
    void JumpTo(const TaskItem& item);

    // ── export ────────────────────────────────────────────────
    std::string ExportText(const TaskFilter& filter = {}) const;
    std::string ExportCSV(const TaskFilter& filter = {}) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace IDE
