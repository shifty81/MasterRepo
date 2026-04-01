#pragma once
#include <memory>
#include <vector>

namespace NF {

class Texture;

/// @brief Abstract base class for a single post-processing effect.
class PostProcessPass {
public:
    virtual ~PostProcessPass() = default;

    /// @brief Apply the effect, reading from @p input and writing to @p output.
    /// @param input  Source texture produced by the previous pass (or scene).
    /// @param output Destination texture for the result of this pass.
    virtual void Apply(Texture& input, Texture& output) = 0;
};

/// @brief Ordered chain of post-processing passes executed each frame.
class PostProcessStack {
public:
    /// @brief Append a pass to the end of the chain.
    /// @param pass Shared ownership of the pass object.
    void AddPass(std::shared_ptr<PostProcessPass> pass);

    /// @brief Execute all passes in order.
    ///
    /// Passes are chained so that each pass's output becomes the next pass's
    /// input.  The final result ends up in @p output.
    ///
    /// @param scene  The initial scene render texture.
    /// @param output The texture that receives the final composited result.
    void Execute(Texture& scene, Texture& output);

private:
    std::vector<std::shared_ptr<PostProcessPass>> m_Passes;
};

} // namespace NF
