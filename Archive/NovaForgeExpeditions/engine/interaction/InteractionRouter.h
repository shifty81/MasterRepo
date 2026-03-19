#pragma once
#include "Intent.h"
#include "Utterance.h"

namespace atlas::interaction {

class InteractionRouter {
public:
    virtual ~InteractionRouter() = default;

    virtual Utterance Route(
        const Intent& intent,
        const Utterance& source
    ) = 0;
};

} // namespace atlas::interaction
