#pragma once

#include "Text/Export.h"
#include "Text/Unicode/Grapheme.h"

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::text::editing {

struct TextSelection {
    size_t anchor = 0; // codepoint index
    size_t focus = 0;

    [[nodiscard]] size_t Start() const { return anchor < focus ? anchor : focus; }
    [[nodiscard]] size_t End() const { return anchor < focus ? focus : anchor; }
    [[nodiscard]] bool Empty() const { return anchor == focus; }
};

struct CaretState {
    size_t codepointIndex = 0;
    float x = 0.0f;
    float y = 0.0f;
    uint32_t lineIndex = 0;
};

class TEXT_API ICaretEngine {
public:
    virtual ~ICaretEngine() = default;
    virtual void SetOffset(size_t codepointIndex) = 0;
    [[nodiscard]] virtual size_t Offset() const = 0;
    virtual void MoveLeft(std::span<const Codepoint> codepoints, bool extendSelection) = 0;
    virtual void MoveRight(std::span<const Codepoint> codepoints, bool extendSelection) = 0;
    virtual void MoveWordLeft(std::span<const Codepoint> codepoints, bool extendSelection) = 0;
    virtual void MoveWordRight(std::span<const Codepoint> codepoints, bool extendSelection) = 0;
    virtual void MoveHome(std::span<const Codepoint> codepoints, bool extendSelection) = 0;
    virtual void MoveEnd(std::span<const Codepoint> codepoints, bool extendSelection) = 0;
    [[nodiscard]] virtual TextSelection Selection() const = 0;
    virtual void SetSelection(size_t anchor, size_t focus) = 0;
    virtual void ClearSelection() = 0;
};

class TEXT_API ITextSelectionEngine {
public:
    virtual ~ITextSelectionEngine() = default;
    virtual void BeginDrag(size_t codepointIndex) = 0;
    virtual void DragTo(size_t codepointIndex) = 0;
    virtual void EndDrag() = 0;
    virtual void SelectWordAt(std::span<const Codepoint> codepoints, size_t codepointIndex) = 0;
    virtual void SelectLine(std::span<const Codepoint> codepoints, size_t lineStart, size_t lineEnd) = 0;
    virtual void SelectAll(size_t length) = 0;
    [[nodiscard]] virtual TextSelection Selection() const = 0;
};

struct EditCommand {
    enum class Type { Insert, Delete, Replace };
    Type type = Type::Insert;
    size_t start = 0;
    size_t end = 0;
    std::string text;
    std::string previous;
};

class TEXT_API ITextEditSession {
public:
    virtual ~ITextEditSession() = default;
    [[nodiscard]] virtual const std::string& Text() const = 0;
    virtual void SetText(std::string text) = 0;
    [[nodiscard]] virtual ICaretEngine& Caret() = 0;
    [[nodiscard]] virtual ITextSelectionEngine& SelectionEngine() = 0;
    virtual void Insert(std::string_view utf8) = 0;
    virtual void DeleteBackward() = 0;
    virtual void DeleteForward() = 0;
    virtual void DeleteSelection() = 0;
    virtual bool Undo() = 0;
    virtual bool Redo() = 0;
    virtual void SetClipboard(std::string text) = 0;
    [[nodiscard]] virtual std::string Clipboard() const = 0;
    virtual void Copy() = 0;
    virtual void Cut() = 0;
    virtual void Paste() = 0;
    virtual void SetComposition(std::string_view utf8, size_t cursorInComposition) = 0;
    virtual void ConfirmComposition() = 0;
    virtual void CancelComposition() = 0;
    [[nodiscard]] virtual bool HasComposition() const = 0;
    [[nodiscard]] virtual std::string_view Composition() const = 0;
};

[[nodiscard]] TEXT_API std::unique_ptr<ITextEditSession> CreateTextEditSession(std::string initial = {});

} // namespace we::runtime::text::editing
