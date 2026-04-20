#ifndef TESTCPP1_EVENTSYSTEM_H
#define TESTCPP1_EVENTSYSTEM_H

#include <string>
#include <vector>

#include "shared/GameTypes.h"

namespace game {

struct EventDefinition {
    std::string id;
    std::string name;
    EventModifierType modifierType = EventModifierType::Neutral;
    std::string description;
    bool oneTimeOnly = false;
};

class RandomEvent {
public:
    virtual ~RandomEvent() = default;

    virtual const EventDefinition& getDefinition() const = 0;
    virtual bool canTrigger(const std::string& mapId) const = 0;
    virtual void apply(CharacterStats& stats) = 0;
};

class RandomEventSystem {
public:
    void registerEvent(RandomEvent* randomEvent);
    const RandomEvent* rollEventForMap(const std::string& mapId) const;

private:
    std::vector<RandomEvent*> availableEvents;
};

} // namespace game

#endif // TESTCPP1_EVENTSYSTEM_H
