#include "Editor/Panels/MaterialEditorPanel.h"
#include "Core/Logging/Log.h"
#include <algorithm>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<MaterialPin> MakeOutputPin(uint32_t& nextId,
                                               const std::string& name,
                                               PinType type)
{
    MaterialPin p;
    p.id       = nextId++;
    p.name     = name;
    p.type     = type;
    p.isOutput = true;
    return {p};
}

static MaterialPin MakeInputPin(uint32_t& nextId,
                                const std::string& name,
                                PinType type)
{
    MaterialPin p;
    p.id       = nextId++;
    p.name     = name;
    p.type     = type;
    p.isOutput = false;
    return p;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

MaterialEditorPanel::MaterialEditorPanel() = default;

// ---------------------------------------------------------------------------
// Node management
// ---------------------------------------------------------------------------

uint32_t MaterialEditorPanel::AddNode(const std::string& kind, float x, float y)
{
    MaterialNode node;
    node.id    = AllocId();
    node.kind  = kind;
    node.label = kind;
    node.posX  = x;
    node.posY  = y;
    node.pins  = MakePins(kind);
    m_Nodes.push_back(std::move(node));
    Logger::Log(LogLevel::Debug, "MaterialEditor",
                "Added node: " + kind);
    return m_Nodes.back().id;
}

void MaterialEditorPanel::RemoveNode(uint32_t nodeId)
{
    // Remove links that reference any pin on this node
    auto* n = FindNode(nodeId);
    if (!n) return;

    std::vector<uint32_t> pinIds;
    pinIds.reserve(n->pins.size());
    for (const auto& p : n->pins) pinIds.push_back(p.id);

    m_Links.erase(
        std::remove_if(m_Links.begin(), m_Links.end(),
            [&](const MaterialLink& lk) {
                for (uint32_t pid : pinIds)
                    if (lk.fromPin == pid || lk.toPin == pid)
                        return true;
                return false;
            }),
        m_Links.end());

    m_Nodes.erase(
        std::remove_if(m_Nodes.begin(), m_Nodes.end(),
            [nodeId](const MaterialNode& nd) { return nd.id == nodeId; }),
        m_Nodes.end());
}

// ---------------------------------------------------------------------------
// Link management
// ---------------------------------------------------------------------------

uint32_t MaterialEditorPanel::AddLink(uint32_t fromPinId, uint32_t toPinId)
{
    if (fromPinId == toPinId) return 0;

    MaterialLink lk;
    lk.id      = AllocId();
    lk.fromPin = fromPinId;
    lk.toPin   = toPinId;
    m_Links.push_back(lk);
    return lk.id;
}

void MaterialEditorPanel::RemoveLink(uint32_t linkId)
{
    m_Links.erase(
        std::remove_if(m_Links.begin(), m_Links.end(),
            [linkId](const MaterialLink& lk) { return lk.id == linkId; }),
        m_Links.end());
}

// ---------------------------------------------------------------------------
// Compile
// ---------------------------------------------------------------------------

std::shared_ptr<Material> MaterialEditorPanel::Compile() const
{
    // Locate the Output node — required for a valid graph.
    const MaterialNode* outputNode = nullptr;
    for (const auto& n : m_Nodes)
        if (n.kind == "Output") { outputNode = &n; break; }

    if (!outputNode) {
        Logger::Log(LogLevel::Warning, "MaterialEditor",
                    "Compile: no Output node found");
        return nullptr;
    }

    auto mat = std::make_shared<Material>();
    Logger::Log(LogLevel::Info, "MaterialEditor", "Compiled material graph");
    return mat;
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void MaterialEditorPanel::Draw()
{
    if (!m_Open) return;
    Logger::Log(LogLevel::Trace, "MaterialEditor", "Draw – MaterialEditorPanel");
    // Concrete node-graph rendering is performed by the platform UI layer.
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

MaterialNode* MaterialEditorPanel::FindNode(uint32_t id)
{
    for (auto& n : m_Nodes)
        if (n.id == id) return &n;
    return nullptr;
}

std::vector<MaterialPin> MaterialEditorPanel::MakePins(const std::string& kind)
{
    std::vector<MaterialPin> pins;

    auto addIn  = [&](const std::string& name, PinType type) {
        pins.push_back(MakeInputPin(m_NextId, name, type));
    };
    auto addOut = [&](const std::string& name, PinType type) {
        for (auto& p : MakeOutputPin(m_NextId, name, type))
            pins.push_back(p);
    };

    if (kind == "Output") {
        addIn("BaseColor", PinType::Color);
        addIn("Metallic",  PinType::Float);
        addIn("Roughness", PinType::Float);
        addIn("Normal",    PinType::Vec3);
    } else if (kind == "Texture2D") {
        addIn("UV",     PinType::Vec3);
        addOut("RGBA",  PinType::Color);
        addOut("R",     PinType::Float);
    } else if (kind == "Multiply") {
        addIn("A",      PinType::Float);
        addIn("B",      PinType::Float);
        addOut("Out",   PinType::Float);
    } else if (kind == "Add") {
        addIn("A",      PinType::Float);
        addIn("B",      PinType::Float);
        addOut("Out",   PinType::Float);
    } else if (kind == "Constant") {
        addOut("Value", PinType::Float);
    } else if (kind == "ConstantVec3") {
        addOut("Value", PinType::Vec3);
    } else {
        // Unknown kind: one generic input + one generic output
        addIn("In",    PinType::Float);
        addOut("Out",  PinType::Float);
    }

    return pins;
}

} // namespace NF::Editor
