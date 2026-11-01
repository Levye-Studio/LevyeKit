#pragma once

#include "../EditablePose.hpp"

namespace Levye {

/** @brief Partial-success batch IK solver; independent chains solve by ascending id. */
struct MultiChainIK {
  /** @brief Returns false only for invalid batch storage or an unsampled pose. */
  static bool Solve(EditablePose& pose, const AnimationIKChainRequest* requests, int count,
                    AnimationIKChainResult* results);
};

}  // namespace Levye
