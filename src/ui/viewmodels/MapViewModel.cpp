#include "MapViewModel.h"
#include "core/Assert.h"

#include <algorithm>
#include <cmath>

namespace CF::UI {

using namespace CF::Domain;
using namespace CF::Core;

// Catppuccin Mocha palette — one colour per region, cycling
static const QColor REGION_COLOURS[] = {
    QColor("#89b4fa"),  // blue
    QColor("#a6e3a1"),  // green
    QColor("#f38ba8"),  // red
    QColor("#fab387"),  // peach
    QColor("#cba6f7"),  // mauve
    QColor("#94e2d5"),  // teal
    QColor("#89dceb"),  // sky
    QColor("#f9e2af"),  // yellow
};
static constexpr int REGION_COLOUR_COUNT = 8;

MapViewModel::MapViewModel(
    std::shared_ptr<Services::PlaceService>     placeService,
    std::shared_ptr<Services::CharacterService> characterService,
    Core::Logger& logger,
    QObject* parent)
    : QObject(parent)
    , m_placeService(std::move(placeService))
    , m_characterService(std::move(characterService))
    , m_logger(logger)
{
    CF_REQUIRE(m_placeService != nullptr, "PlaceService must not be null");
}

// ── loadAll ───────────────────────────────────────────────────────────────────

void MapViewModel::loadAll()
{
    auto result = m_placeService->getAll();
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    m_nodes.clear();
    m_idToIndex.clear();

    const auto& places = result.value();
    m_nodes.reserve(places.size());

    for (const auto& p : places) {
        PlaceNode node;
        node.id          = p.id;
        node.name        = QString::fromStdString(p.name);
        node.type        = QString::fromStdString(p.type);
        node.region      = QString::fromStdString(p.region);
        node.normX       = p.mapX;
        node.normY       = p.mapY;
        node.isMobile    = p.isMobile;
        node.hasChildren = false;  // will be set below

        m_idToIndex[p.id.value()] = static_cast<int>(m_nodes.size());
        m_nodes.push_back(std::move(node));
    }

    // Mark nodes that have children (parentPlaceId set on another node)
    for (const auto& p : places) {
        if (p.parentPlaceId.has_value()) {
            const auto it = m_idToIndex.find(p.parentPlaceId->value());
            if (it != m_idToIndex.end()) {
                m_nodes[it->second].hasChildren = true;
            }
        }
    }

    loadCharacterCounts();
    buildRegions();

    m_logger.debug("Map loaded: " + std::to_string(m_nodes.size()) + " places",
                   "MapViewModel");
    emit nodesChanged();
}

// ── Accessors ─────────────────────────────────────────────────────────────────

const std::vector<MapViewModel::PlaceNode>& MapViewModel::nodes() const
{
    return m_nodes;
}

const std::vector<MapViewModel::RegionPanel>& MapViewModel::regions() const
{
    return m_regions;
}

int MapViewModel::nodeIndexForId(PlaceId id) const
{
    const auto it = m_idToIndex.find(id.value());
    return it != m_idToIndex.end() ? it->second : -1;
}

// ── moveNode ──────────────────────────────────────────────────────────────────

void MapViewModel::moveNode(int nodeIndex, double normX, double normY)
{
    CF_ASSERT(nodeIndex >= 0 && nodeIndex < static_cast<int>(m_nodes.size()),
              "Invalid node index");

    normX = std::max(0.0, std::min(1.0, normX));
    normY = std::max(0.0, std::min(1.0, normY));

    m_nodes[nodeIndex].normX = normX;
    m_nodes[nodeIndex].normY = normY;

    // Persist position back to DB
    auto result = m_placeService->updateMapPosition(
        m_nodes[nodeIndex].id, normX, normY);
    if (result.isErr()) {
        m_logger.warning("Failed to persist node position: "
                         + result.error().message, "MapViewModel");
    }

    // Recompute regions (bounding boxes may have changed)
    buildRegions();
    emit nodesChanged();
}

// ── nodeAtPosition ────────────────────────────────────────────────────────────

std::optional<int>
MapViewModel::nodeAtPosition(double normX, double normY, double radius) const
{
    int   bestIndex = -1;
    double bestDist  = radius * radius;  // squared for comparison

    for (int i = 0; i < static_cast<int>(m_nodes.size()); ++i) {
        const auto& n  = m_nodes[i];
        const double dx = n.normX - normX;
        const double dy = n.normY - normY;
        const double d2 = dx * dx + dy * dy;
        if (d2 < bestDist) {
            bestDist  = d2;
            bestIndex = i;
        }
    }

    return bestIndex >= 0 ? std::optional<int>(bestIndex) : std::nullopt;
}

// ── Selection ─────────────────────────────────────────────────────────────────

void MapViewModel::selectNode(std::optional<int> nodeIndex)
{
    if (m_selectedIndex == nodeIndex) return;
    m_selectedIndex = nodeIndex;
    emit selectionChanged();
}

std::optional<int> MapViewModel::selectedNodeIndex() const
{
    return m_selectedIndex;
}

std::optional<MapViewModel::PlaceNode> MapViewModel::selectedNode() const
{
    if (!m_selectedIndex.has_value()) return std::nullopt;
    return m_nodes[*m_selectedIndex];
}

// ── Private helpers ───────────────────────────────────────────────────────────

void MapViewModel::buildRegions()
{
    m_regions.clear();

    // Collect unique region names
    std::vector<QString> regionNames;
    for (const auto& node : m_nodes) {
        if (node.region.isEmpty()) continue;
        if (std::find(regionNames.begin(), regionNames.end(), node.region)
            == regionNames.end()) {
            regionNames.push_back(node.region);
        }
    }
    std::sort(regionNames.begin(), regionNames.end());

    // Build region panels
    for (int i = 0; i < static_cast<int>(regionNames.size()); ++i) {
        RegionPanel panel;
        panel.name   = regionNames[i];
        panel.colour = regionColour(i);

        for (int j = 0; j < static_cast<int>(m_nodes.size()); ++j) {
            if (m_nodes[j].region == regionNames[i]) {
                panel.nodeIndices.push_back(j);
            }
        }
        m_regions.push_back(std::move(panel));
    }
}

void MapViewModel::loadCharacterCounts()
{
    // Reset all counts
    for (auto& node : m_nodes) node.characterCount = 0;

    // For each character, find their current place and increment its count
    auto result = m_characterService->getAll();
    if (result.isErr()) return;

    for (const auto& character : result.value()) {
        // Find current location (most recent open stint)
        for (const auto& stint : character.locationHistory) {
            if (stint.isCurrent()) {
                const auto it = m_idToIndex.find(stint.placeId.value());
                if (it != m_idToIndex.end()) {
                    m_nodes[it->second].characterCount++;
                }
                break;
            }
        }
    }
}

QColor MapViewModel::regionColour(int regionIndex) const
{
    return REGION_COLOURS[regionIndex % REGION_COLOUR_COUNT];
}

} // namespace CF::UI