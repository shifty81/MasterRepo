#include "systems/trade_flow_system.h"
#include "components/game_components.h"
#include <algorithm>

namespace atlas {

using namespace components;

void TradeFlowSystem::initialize(ecs::Entity* market) {
    if (!market->hasComponent<TradeFlow>()) {
        market->addComponent(std::make_unique<TradeFlow>());
    }
}

void TradeFlowSystem::addFlow(ecs::Entity* market, const std::string& item, float supply, float demand) {
    auto* trade = market->getComponent<TradeFlow>();
    if (!trade) return;

    TradeFlow::FlowEntry entry;
    entry.item_type = item;
    entry.supply_rate = supply;
    entry.demand_rate = demand;
    entry.price_modifier = 1.0f;
    trade->flows.push_back(entry);
}

void TradeFlowSystem::update(ecs::Entity* market, float dt) {
    auto* trade = market->getComponent<TradeFlow>();
    if (!trade) return;

    float scarcity_sum = 0.0f;
    float volume_sum = 0.0f;

    for (auto& flow : trade->flows) {
        if (flow.demand_rate > flow.supply_rate) {
            flow.price_modifier += (flow.demand_rate - flow.supply_rate) * 0.01f * dt;
        } else {
            flow.price_modifier -= (flow.supply_rate - flow.demand_rate) * 0.01f * dt;
        }
        flow.price_modifier = std::max(flow.price_modifier, 0.01f);

        float effective_supply = std::max(flow.supply_rate, 0.01f);
        scarcity_sum += std::clamp(flow.demand_rate / effective_supply, 0.0f, 1.0f);
        volume_sum += std::min(flow.supply_rate, flow.demand_rate);
    }

    if (!trade->flows.empty()) {
        trade->scarcity_index = std::clamp(scarcity_sum / static_cast<float>(trade->flows.size()), 0.0f, 1.0f);
    }
    trade->trade_volume = volume_sum;
}

float TradeFlowSystem::getScarcityIndex(ecs::Entity* market) {
    auto* trade = market->getComponent<TradeFlow>();
    return trade ? trade->scarcity_index : 0.0f;
}

float TradeFlowSystem::getPriceModifier(ecs::Entity* market, const std::string& item) {
    auto* trade = market->getComponent<TradeFlow>();
    if (!trade) return 1.0f;

    for (const auto& flow : trade->flows) {
        if (flow.item_type == item) {
            return flow.price_modifier;
        }
    }
    return 1.0f;
}

} // namespace atlas
