#include "ContactPairRegistry.hpp"

#include <algorithm>
#include <tuple>

namespace Levye {
namespace {
std::uint64_t HandleKey(PhysicsBodyHandle handle) {
  return (static_cast<std::uint64_t>(handle.generation) << 32) | handle.index;
}
}  // namespace

ContactEventQueue::Pair ContactEventQueue::Key(const PhysicsContactEvent& event) {
  const auto a = HandleKey(event.bodyA);
  const auto b = HandleKey(event.bodyB);
  return std::minmax(a, b);
}

void ContactEventQueue::Push(PhysicsContactEvent event) {
  const auto key = Key(event);
  const auto sequence = nextSequence_++;
  if (event.type == PhysicsContactEventType::End && suppressedBegins_.erase(key) != 0) {
    ++overflowCount_;
    return;  // Its Begin was never delivered.
  }
  if (event.type == PhysicsContactEventType::Persist && suppressedBegins_.count(key) != 0) {
    ++overflowCount_;
    return;
  }
  if (events_.size() < Capacity) {
    events_.push_back({event, sequence});
    return;
  }
  ++overflowCount_;
  if (event.type == PhysicsContactEventType::Begin) {
    suppressedBegins_.insert(key);
    return;
  }
  if (event.type == PhysicsContactEventType::Persist) return;

  const auto eraseUndeliveredCycle = [&](std::deque<QueuedEvent>::iterator begin) {
    const auto cycle = Key(begin->event);
    for (auto it = begin; it != events_.end();) {
      if (Key(it->event) == cycle) {
        it = events_.erase(it);
        ++overflowCount_;
      } else {
        ++it;
      }
    }
  };
  auto begin = std::find_if(events_.begin(), events_.end(), [&](const auto& queued) {
    return queued.event.type == PhysicsContactEventType::Begin && Key(queued.event) == key;
  });
  if (begin != events_.end()) {
    eraseUndeliveredCycle(begin);
    return;  // Neither transition was delivered, so they cancel.
  }
  auto disposable = std::find_if(events_.begin(), events_.end(), [](const auto& queued) {
    return queued.event.type == PhysicsContactEventType::Persist;
  });
  if (disposable == events_.end()) {
    disposable = std::find_if(events_.begin(), events_.end(), [](const auto& queued) {
      return queued.event.type == PhysicsContactEventType::Begin;
    });
  }
  if (disposable != events_.end()) {
    if (disposable->event.type == PhysicsContactEventType::Begin) {
      suppressedBegins_.insert(Key(disposable->event));
      eraseUndeliveredCycle(disposable);
    } else {
      events_.erase(disposable);
    }
    events_.push_back({event, sequence});
  } else if (terminalSequenceByPair_.count(key) == 0) {
    terminalSequenceByPair_.emplace(key, sequence);
    terminalBySequence_.emplace(sequence, event);
  }
}

bool ContactEventQueue::Poll(PhysicsContactEvent& output) {
  if (events_.empty() && terminalBySequence_.empty()) return false;
  if (!events_.empty() && (terminalBySequence_.empty() ||
                           events_.front().sequence < terminalBySequence_.begin()->first)) {
    output = events_.front().event;
    events_.pop_front();
    return true;
  }
  const auto first = terminalBySequence_.begin();
  output = first->second;
  terminalSequenceByPair_.erase(Key(output));
  terminalBySequence_.erase(first);
  return true;
}

void ContactEventQueue::Clear() {
  events_.clear();
  terminalBySequence_.clear();
  terminalSequenceByPair_.clear();
  suppressedBegins_.clear();
  overflowCount_ = 0;
  nextSequence_ = 0;
}

std::uint64_t ContactEventQueue::TakeOverflowCount() {
  const auto count = overflowCount_;
  overflowCount_ = 0;
  return count;
}

ContactPairRegistry::BodyPair ContactPairRegistry::Normalize(std::uint32_t a, std::uint32_t b) {
  return std::minmax(a, b);
}

std::vector<ContactPairRegistry::Event> ContactPairRegistry::Apply(
    const std::vector<ContactObservation>& observations) {
  struct SubChange {
    bool added = false;
    bool removed = false;
    bool hasSample = false;
    ContactObservation sample{};
  };
  using ChangeKey = std::pair<BodyPair, SubPair>;
  std::map<ChangeKey, SubChange> changes;
  std::set<BodyPair> touched;
  for (auto observation : observations) {
    if (observation.bodyA == observation.bodyB) continue;
    const auto pair = Normalize(observation.bodyA, observation.bodyB);
    if (pair.first != observation.bodyA) {
      std::swap(observation.shapeA, observation.shapeB);
      observation.normal = {-observation.normal.x, -observation.normal.y, -observation.normal.z};
    }
    auto& change = changes[{pair, {observation.shapeA, observation.shapeB}}];
    touched.insert(pair);
    if (observation.kind == ContactObservation::Kind::Removed) {
      change.removed = true;
    } else {
      change.added = true;
      const auto rank = [](const ContactObservation& value) {
        return std::tuple{value.kind != ContactObservation::Kind::Added,
                          value.position.x,
                          value.position.y,
                          value.position.z,
                          value.normal.x,
                          value.normal.y,
                          value.normal.z};
      };
      if (!change.hasSample || rank(observation) < rank(change.sample)) {
        change.sample = observation;
        change.hasSample = true;
      }
    }
  }

  std::vector<Event> events;
  events.reserve(touched.size());
  for (const auto& pair : touched) {
    auto& state = pairs_[pair];
    const bool wasActive = !state.subshapes.empty();
    for (auto it = changes.lower_bound({pair, {0, 0}});
         it != changes.end() && it->first.first == pair; ++it) {
      const auto& [key, change] = *it;
      // A simultaneous remove/add of the same subshape remains active.
      if (change.added) {
        state.subshapes.insert(key.second);
        if (change.hasSample) {
          state.position = change.sample.position;
          state.normal = change.sample.normal;
          state.isSensor = change.sample.isSensor;
        }
      } else if (change.removed) {
        state.subshapes.erase(key.second);
      }
    }
    const bool active = !state.subshapes.empty();
    if (wasActive || active) {
      events.push_back({!wasActive ? PhysicsContactEventType::Begin
                        : active   ? PhysicsContactEventType::Persist
                                   : PhysicsContactEventType::End,
                        pair, state.position, state.normal, state.isSensor});
    }
    if (!active) pairs_.erase(pair);
  }
  return events;
}

std::vector<ContactPairRegistry::Event> ContactPairRegistry::EndForBody(std::uint32_t bodyID) {
  std::vector<Event> events;
  for (auto it = pairs_.begin(); it != pairs_.end();) {
    if (it->first.first == bodyID || it->first.second == bodyID) {
      events.push_back({PhysicsContactEventType::End, it->first, it->second.position,
                        it->second.normal, it->second.isSensor});
      it = pairs_.erase(it);
    } else {
      ++it;
    }
  }
  return events;
}

std::vector<ContactPairRegistry::Event> ContactPairRegistry::EndFilteredPairs(
    std::uint32_t bodyID, const std::function<bool(std::uint32_t)>& shouldEnd) {
  std::vector<Event> events;
  for (auto it = pairs_.begin(); it != pairs_.end();) {
    const auto& pair = it->first;
    if ((pair.first == bodyID || pair.second == bodyID) &&
        shouldEnd(pair.first == bodyID ? pair.second : pair.first)) {
      events.push_back({PhysicsContactEventType::End, pair, it->second.position, it->second.normal,
                        it->second.isSensor});
      it = pairs_.erase(it);
    } else {
      ++it;
    }
  }
  return events;
}

void ContactPairRegistry::Clear() { pairs_.clear(); }
}  // namespace Levye
