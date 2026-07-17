#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/PaintContext.h"

#include <vector>

namespace we::runtime::kindui {

enum class FlexDirection { Row, Column, RowReverse, ColumnReverse };
enum class FlexWrap { NoWrap, Wrap };
enum class JustifyContent { Start, End, Center, SpaceBetween, SpaceAround, SpaceEvenly };
enum class AlignItems { Start, End, Center, Stretch };

/// CSS-Flexbox-inspired container. Children use Widget flex grow/shrink/basis/margin.
class KINDUI_API Flex : public Widget {
public:
    explicit Flex(FlexDirection direction = FlexDirection::Row);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    Flex& Direction(FlexDirection d) { m_Direction = d; InvalidateLayout(); return *this; }
    Flex& Wrap(FlexWrap w) { m_Wrap = w; InvalidateLayout(); return *this; }
    Flex& Justify(JustifyContent j) { m_Justify = j; InvalidateLayout(); return *this; }
    Flex& Align(AlignItems a) { m_Align = a; InvalidateLayout(); return *this; }
    Flex& Gap(float gap) { m_Gap = gap; InvalidateLayout(); return *this; }
    Flex& Padding(const Margin& p) { m_Padding = p; InvalidateLayout(); return *this; }
    Flex& Background(const Color& c) { m_Background = c; m_HasBackground = true; InvalidatePaint(); return *this; }
    Flex& Style(std::string className);

    [[nodiscard]] FlexDirection GetDirection() const { return m_Direction; }

private:
    [[nodiscard]] bool IsRow() const {
        return m_Direction == FlexDirection::Row || m_Direction == FlexDirection::RowReverse;
    }

    FlexDirection m_Direction = FlexDirection::Row;
    FlexWrap m_Wrap = FlexWrap::NoWrap;
    JustifyContent m_Justify = JustifyContent::Start;
    AlignItems m_Align = AlignItems::Stretch;
    float m_Gap = 0.0f;
    Margin m_Padding{};
    Color m_Background{};
    bool m_HasBackground = false;
};

class KINDUI_API Row : public Flex {
public:
    Row() : Flex(FlexDirection::Row) {}
};

class KINDUI_API Column : public Flex {
public:
    Column() : Flex(FlexDirection::Column) {}
};

/// Factory helpers for declarative composition.
[[nodiscard]] KINDUI_API std::shared_ptr<Row> MakeRow();
[[nodiscard]] KINDUI_API std::shared_ptr<Column> MakeColumn();

} // namespace we::runtime::kindui
