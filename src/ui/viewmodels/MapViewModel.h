#pragma once

#include <QObject>
#include <QString>
#include <QPointF>
#include <QColor>
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>

#include "services/PlaceService.h"
#include "services/CharacterService.h"
#include "domain/entities/Place.h"
#include "core/Logger.h"

namespace CF::UI {

/**
 * MapViewModel prepares all data the MapWidget needs to paint.
 *
 * It holds:
 *  - A flat list of PlaceNodes (position + metadata)
 *  - A list of region panels (bounding boxes computed from contained nodes)
 *  - Character counts per place (for the population indicator)
 *
 * Node positions are stored normalised [0.0–1.0] in the domain and
 * converted to canvas pixels by the widget at paint time.
 */
class MapViewModel : public QObject {
    Q_OBJECT

public:
    // ── Data structures ───────────────────────────────────────────────────────

    struct PlaceNode {
        Domain::PlaceId id;
        QString         name;
        QString         type;       // "City", "Spaceship", etc.
        QString         region;
        double          normX;      // 0.0 – 1.0 (stored in DB)
        double          normY;      // 0.0 – 1.0
        bool            isMobile;
        int             characterCount = 0;  // Currently located here
        bool            hasChildren    = false;
    };

    struct RegionPanel {
        QString             name;
        std::vector<int>    nodeIndices;  // Indices into m_nodes
        QColor              colour;       // Assigned per region
    };

    explicit MapViewModel(
        std::shared_ptr<Services::PlaceService>     placeService,
        std::shared_ptr<Services::CharacterService> characterService,
        Core::Logger& logger,
        QObject* parent = nullptr);

    // ── Loading ───────────────────────────────────────────────────────────────
    void loadAll();

    // ── Accessors ─────────────────────────────────────────────────────────────
    [[nodiscard]] const std::vector<PlaceNode>&   nodes()   const;
    [[nodiscard]] const std::vector<RegionPanel>& regions() const;

    // Returns node index for a given PlaceId, or -1
    [[nodiscard]] int nodeIndexForId(Domain::PlaceId id) const;

    // ── Node interaction ──────────────────────────────────────────────────────

    // Called when the user drags a node — updates normalised position
    // and persists it via PlaceService
    void moveNode(int nodeIndex, double normX, double normY);

    // Returns the node at normalised canvas position, within a radius
    [[nodiscard]] std::optional<int>
    nodeAtPosition(double normX, double normY,
                   double radiusNorm = 0.015) const;

    // ── Selection ─────────────────────────────────────────────────────────────
    void selectNode(std::optional<int> nodeIndex);
    [[nodiscard]] std::optional<int> selectedNodeIndex() const;
    [[nodiscard]] std::optional<PlaceNode> selectedNode() const;

signals:
    void nodesChanged();
    void selectionChanged();
    void errorOccurred(const QString& message);

private:
    void buildRegions();
    void loadCharacterCounts();
    [[nodiscard]] QColor regionColour(int regionIndex) const;

    std::vector<PlaceNode>   m_nodes;
    std::vector<RegionPanel> m_regions;
    std::optional<int>       m_selectedIndex;

    // Quick lookup: PlaceId.value() → index in m_nodes
    std::unordered_map<int64_t, int> m_idToIndex;

    std::shared_ptr<Services::PlaceService>     m_placeService;
    std::shared_ptr<Services::CharacterService> m_characterService;
    Core::Logger& m_logger;
};

} // namespace CF::UI