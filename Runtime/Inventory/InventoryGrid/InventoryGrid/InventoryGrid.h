#pragma once
/**
 * @file InventoryGrid.h
 * @brief Tetris-style 2D grid inventory with item rotation, overlap detection, and stack limits.
 *
 * Features:
 *   - Init(cols, rows): create fixed-size grid
 *   - Item shapes defined as bitmask rectangles (w×h cells)
 *   - TryPlace(item, col, row, rotated) → bool: place if all cells free
 *   - Remove(itemId) → bool: clear all cells the item occupies
 *   - FindFit(item) → optional<GridPos>: first position that fits (auto-place helper)
 *   - CanPlace(item, col, row, rotated) const → bool: non-destructive check
 *   - GetItemAt(col, row) → itemId or 0 (empty sentinel)
 *   - Item stacking: stackable items accumulate count up to stackMax
 *   - AddStack(itemId, count) → overflow count
 *   - Clear(): remove all items
 *   - GetItems() → list of placed items with position/rotation
 *   - Serialisation: SaveToJSON / LoadFromJSON
 */

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Runtime {

struct GridPos { int col, row; };

struct GridItem {
    uint32_t    id{0};
    std::string name;
    uint8_t     w{1}, h{1};     ///< extents in cells (pre-rotation)
    bool        stackable{false};
    uint32_t    stackMax{99};
    uint32_t    stackCount{1};
};

struct PlacedItem {
    uint32_t itemId;
    int      col, row;
    bool     rotated;   ///< if true, w/h are swapped
};

class InventoryGrid {
public:
    InventoryGrid();
    ~InventoryGrid();

    void Init    (uint32_t cols, uint32_t rows);
    void Shutdown();
    void Clear   ();

    // Item registry
    void RegisterItem  (const GridItem& item);
    void UnregisterItem(uint32_t id);

    // Placement
    bool TryPlace  (uint32_t itemId, int col, int row, bool rotated=false);
    bool CanPlace  (uint32_t itemId, int col, int row, bool rotated=false) const;
    bool Remove    (uint32_t itemId);
    std::optional<GridPos> FindFit(uint32_t itemId, bool tryRotated=true) const;

    // Stack helpers (stackable items only)
    uint32_t AddStack   (uint32_t itemId, uint32_t count);   ///< returns overflow
    uint32_t RemoveStack(uint32_t itemId, uint32_t count);   ///< returns actually removed

    // Query
    uint32_t GetItemAt(int col, int row) const; ///< 0 = empty
    const std::vector<PlacedItem>& GetPlacedItems() const;
    uint32_t Cols() const;
    uint32_t Rows() const;

    // Serialisation
    std::string SaveToJSON () const;
    bool        LoadFromJSON(const std::string& json);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
