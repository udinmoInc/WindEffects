#include "Text/Shaping/BidiProcessor.h"

namespace we::runtime::text::shaping {

namespace {

bool IsStrongRtl(const Codepoint codepoint)
{
    return (codepoint >= 0x0590 && codepoint <= 0x08FF)
        || (codepoint >= 0xFB1D && codepoint <= 0xFDFF)
        || (codepoint >= 0xFE70 && codepoint <= 0xFEFF);
}

} // namespace

class BidiProcessor final : public IBidiProcessor {
public:
    std::vector<VisualRun> ReorderVisual(const std::span<const Codepoint> codepoints) const override
    {
        std::vector<VisualRun> runs;
        if (codepoints.empty()) {
            return runs;
        }

        size_t runStart = 0;
        TextDirection currentDirection = IsStrongRtl(codepoints[0])
            ? TextDirection::RightToLeft
            : TextDirection::LeftToRight;

        for (size_t i = 1; i <= codepoints.size(); ++i) {
            const bool atEnd = i == codepoints.size();
            const TextDirection direction = atEnd
                ? currentDirection
                : (IsStrongRtl(codepoints[i]) ? TextDirection::RightToLeft : TextDirection::LeftToRight);

            if (atEnd || direction != currentDirection) {
                VisualRun run;
                run.startIndex = runStart;
                run.length = i - runStart;
                run.direction = currentDirection;
                run.codepoints = codepoints.subspan(runStart, run.length);
                runs.push_back(run);
                runStart = i;
                currentDirection = direction;
            }
        }

        return runs;
    }
};

std::unique_ptr<IBidiProcessor> CreateBidiProcessor()
{
    return std::make_unique<BidiProcessor>();
}

} // namespace we::runtime::text::shaping
