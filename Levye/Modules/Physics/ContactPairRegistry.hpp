#pragma once

#include <Levye/Modules/Physics/PhysicsTypes.hpp>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <set>
#include <vector>

namespace Levye {

/** @brief One Jolt subshape callback captured without touching body registries. */
struct ContactObservation {
  enum class Kind : std::uint8_t { Added, Persisted, Removed };
  Kind kind = Kind::Added;
  std::uint32_t bodyA = 0;
  std::uint32_t bodyB = 0;
  std::uint32_t shapeA = 0;
  std::uint32_t shapeB = 0;
  Vector3 position{};
  Vector3 normal{};
  bool isSensor = false;
};

/**
 * @brief Bounded event FIFO with terminal-event recovery.
 *
 * The FIFO holds at most 8192 events. On overflow it discards Persist first,
 * then undelivered Begin events. An End for a suppressed Begin is also
 * suppressed. End events for delivered Begins that cannot fit are retained in
 * a pair-keyed reconciliation map until polled. Sequence numbers keep those
 * Ends before later recontacts. The map stores at most one End per undrained
 * pair.
 */
class ContactEventQueue {
 public:
  static constexpr std::size_t Capacity = 8192;
  /** @brief Queues a contact event, applying overflow reconciliation if full. */
  void Push(PhysicsContactEvent event);
  /** @brief Reads the oldest queued or reconciled event. */
  [[nodiscard]] bool Poll(PhysicsContactEvent& output);
  /** @brief Discards queued events and overflow diagnostics. */
  void Clear();
  /** @brief Returns and clears the number of coalesced overflow events. */
  [[nodiscard]] std::uint64_t TakeOverflowCount();
  /** @brief Number of events awaiting polling, including retained End events. */
  [[nodiscard]] std::size_t PendingCount() const {
    return events_.size() + terminalBySequence_.size();
  }

 private:
  using Pair = std::pair<std::uint64_t, std::uint64_t>;
  struct QueuedEvent {
    PhysicsContactEvent event{};
    std::uint64_t sequence = 0;
  };
  [[nodiscard]] static Pair Key(const PhysicsContactEvent& event);
  std::deque<QueuedEvent> events_;
  std::map<std::uint64_t, PhysicsContactEvent> terminalBySequence_;
  std::map<Pair, std::uint64_t> terminalSequenceByPair_;
  std::set<Pair> suppressedBegins_;
  std::uint64_t overflowCount_ = 0;
  std::uint64_t nextSequence_ = 0;
};

/**
 * @brief Host-owned subshape registry producing at most one event per body pair per step.
 *
 * Body IDs include Jolt's sequence bits. Call Apply only after Jolt's Update has
 * completed; callbacks may append observations concurrently during Update.
 */
class ContactPairRegistry {
 public:
  using BodyPair = std::pair<std::uint32_t, std::uint32_t>;
  /** @brief One body-pair transition with its last known contact sample. */
  struct Event {
    PhysicsContactEventType type = PhysicsContactEventType::Begin;
    BodyPair pair{};
    Vector3 position{};
    Vector3 normal{};
    bool isSensor = false;
  };

  /** @brief Aggregates subshape observations after a physics step. */
  [[nodiscard]] std::vector<Event> Apply(const std::vector<ContactObservation>& observations);
  /** @brief Ends every active pair involving a body being destroyed. */
  [[nodiscard]] std::vector<Event> EndForBody(std::uint32_t bodyID);
  /**
   * @brief Ends active pairs involving a body when the supplied
   * predicate identifies a pair that is no longer permitted.
   *
   * The predicate receives the other body's Jolt ID.
   * Surviving pairs remain untouched.
   */
  [[nodiscard]] std::vector<Event> EndFilteredPairs(
      std::uint32_t bodyID, const std::function<bool(std::uint32_t)>& shouldEnd);
  /** @brief Discards all active pairs without emitting events. */
  void Clear();
  /** @brief Number of currently active body pairs. */
  [[nodiscard]] std::size_t ActivePairCount() const { return pairs_.size(); }

 private:
  using SubPair = std::pair<std::uint32_t, std::uint32_t>;
  struct PairState {
    std::set<SubPair> subshapes;
    Vector3 position{};
    Vector3 normal{};
    bool isSensor = false;
  };
  [[nodiscard]] static BodyPair Normalize(std::uint32_t a, std::uint32_t b);
  std::map<BodyPair, PairState> pairs_;
};

}  // namespace Levye
