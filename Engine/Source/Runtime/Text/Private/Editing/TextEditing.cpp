#include "Text/Editing/TextEditing.h"

#include <algorithm>
#include <cctype>

namespace we::runtime::text::editing {
namespace {

bool IsWordChar(const Codepoint cp)
{
    return (cp >= '0' && cp <= '9')
        || (cp >= 'A' && cp <= 'Z')
        || (cp >= 'a' && cp <= 'z')
        || cp == '_'
        || cp > 0x7F;
}

class CaretEngine final : public ICaretEngine {
public:
    void SetOffset(const size_t codepointIndex) override
    {
        m_Offset = codepointIndex;
        m_Selection.focus = codepointIndex;
        m_Selection.anchor = codepointIndex;
    }

    size_t Offset() const override { return m_Offset; }

    void MoveLeft(const std::span<const Codepoint> codepoints, const bool extendSelection) override
    {
        m_Offset = unicode::PrevGraphemeOffset(codepoints, m_Offset);
        ApplyMove(extendSelection);
    }

    void MoveRight(const std::span<const Codepoint> codepoints, const bool extendSelection) override
    {
        m_Offset = unicode::NextGraphemeOffset(codepoints, m_Offset);
        ApplyMove(extendSelection);
    }

    void MoveWordLeft(const std::span<const Codepoint> codepoints, const bool extendSelection) override
    {
        if (m_Offset == 0) {
            ApplyMove(extendSelection);
            return;
        }
        size_t i = m_Offset;
        while (i > 0 && !IsWordChar(codepoints[i - 1])) {
            --i;
        }
        while (i > 0 && IsWordChar(codepoints[i - 1])) {
            --i;
        }
        m_Offset = i;
        ApplyMove(extendSelection);
    }

    void MoveWordRight(const std::span<const Codepoint> codepoints, const bool extendSelection) override
    {
        size_t i = m_Offset;
        while (i < codepoints.size() && !IsWordChar(codepoints[i])) {
            ++i;
        }
        while (i < codepoints.size() && IsWordChar(codepoints[i])) {
            ++i;
        }
        m_Offset = i;
        ApplyMove(extendSelection);
    }

    void MoveHome(const std::span<const Codepoint>, const bool extendSelection) override
    {
        m_Offset = 0;
        ApplyMove(extendSelection);
    }

    void MoveEnd(const std::span<const Codepoint> codepoints, const bool extendSelection) override
    {
        m_Offset = codepoints.size();
        ApplyMove(extendSelection);
    }

    TextSelection Selection() const override { return m_Selection; }

    void SetSelection(const size_t anchor, const size_t focus) override
    {
        m_Selection.anchor = anchor;
        m_Selection.focus = focus;
        m_Offset = focus;
    }

    void ClearSelection() override
    {
        m_Selection.anchor = m_Offset;
        m_Selection.focus = m_Offset;
    }

private:
    void ApplyMove(const bool extendSelection)
    {
        m_Selection.focus = m_Offset;
        if (!extendSelection) {
            m_Selection.anchor = m_Offset;
        }
    }

    size_t m_Offset = 0;
    TextSelection m_Selection{};
};

class TextSelectionEngine final : public ITextSelectionEngine {
public:
    explicit TextSelectionEngine(CaretEngine& caret)
        : m_Caret(caret)
    {
    }

    void BeginDrag(const size_t codepointIndex) override
    {
        m_Dragging = true;
        m_Caret.SetSelection(codepointIndex, codepointIndex);
    }

    void DragTo(const size_t codepointIndex) override
    {
        const auto sel = m_Caret.Selection();
        m_Caret.SetSelection(sel.anchor, codepointIndex);
    }

    void EndDrag() override { m_Dragging = false; }

    void SelectWordAt(const std::span<const Codepoint> codepoints, const size_t codepointIndex) override
    {
        if (codepoints.empty()) {
            return;
        }
        size_t start = std::min(codepointIndex, codepoints.size() - 1);
        size_t end = start;
        while (start > 0 && IsWordChar(codepoints[start - 1])) {
            --start;
        }
        while (end < codepoints.size() && IsWordChar(codepoints[end])) {
            ++end;
        }
        m_Caret.SetSelection(start, end);
    }

    void SelectLine(const std::span<const Codepoint>, const size_t lineStart, const size_t lineEnd) override
    {
        m_Caret.SetSelection(lineStart, lineEnd);
    }

    void SelectAll(const size_t length) override { m_Caret.SetSelection(0, length); }

    TextSelection Selection() const override { return m_Caret.Selection(); }

private:
    CaretEngine& m_Caret;
    bool m_Dragging = false;
};

class TextEditSession final : public ITextEditSession {
public:
    explicit TextEditSession(std::string initial)
        : m_Text(std::move(initial))
        , m_Selection(m_Caret)
    {
        m_Codepoints = unicode::DecodeUtf8(m_Text);
    }

    const std::string& Text() const override { return m_Text; }

    void SetText(std::string text) override
    {
        m_Text = std::move(text);
        m_Codepoints = unicode::DecodeUtf8(m_Text);
        m_Caret.SetOffset(std::min(m_Caret.Offset(), m_Codepoints.size()));
        m_Undo.clear();
        m_Redo.clear();
    }

    ICaretEngine& Caret() override { return m_Caret; }
    ITextSelectionEngine& SelectionEngine() override { return m_Selection; }

    void Insert(const std::string_view utf8) override
    {
        DeleteSelection();
        EditCommand cmd;
        cmd.type = EditCommand::Type::Insert;
        cmd.start = m_Caret.Offset();
        cmd.end = cmd.start;
        cmd.text = std::string(utf8);
        Apply(cmd);
        PushUndo(cmd);
    }

    void DeleteBackward() override
    {
        if (!m_Caret.Selection().Empty()) {
            DeleteSelection();
            return;
        }
        if (m_Caret.Offset() == 0) {
            return;
        }
        const size_t end = m_Caret.Offset();
        const size_t start = unicode::PrevGraphemeOffset(m_Codepoints, end);
        EditCommand cmd;
        cmd.type = EditCommand::Type::Delete;
        cmd.start = start;
        cmd.end = end;
        cmd.previous = SliceUtf8(start, end);
        Apply(cmd);
        PushUndo(cmd);
    }

    void DeleteForward() override
    {
        if (!m_Caret.Selection().Empty()) {
            DeleteSelection();
            return;
        }
        if (m_Caret.Offset() >= m_Codepoints.size()) {
            return;
        }
        const size_t start = m_Caret.Offset();
        const size_t end = unicode::NextGraphemeOffset(m_Codepoints, start);
        EditCommand cmd;
        cmd.type = EditCommand::Type::Delete;
        cmd.start = start;
        cmd.end = end;
        cmd.previous = SliceUtf8(start, end);
        Apply(cmd);
        PushUndo(cmd);
    }

    void DeleteSelection() override
    {
        const auto sel = m_Caret.Selection();
        if (sel.Empty()) {
            return;
        }
        EditCommand cmd;
        cmd.type = EditCommand::Type::Delete;
        cmd.start = sel.Start();
        cmd.end = sel.End();
        cmd.previous = SliceUtf8(cmd.start, cmd.end);
        Apply(cmd);
        PushUndo(cmd);
    }

    bool Undo() override
    {
        if (m_Undo.empty()) {
            return false;
        }
        const EditCommand cmd = m_Undo.back();
        m_Undo.pop_back();
        Invert(cmd);
        m_Redo.push_back(cmd);
        return true;
    }

    bool Redo() override
    {
        if (m_Redo.empty()) {
            return false;
        }
        const EditCommand cmd = m_Redo.back();
        m_Redo.pop_back();
        Apply(cmd);
        m_Undo.push_back(cmd);
        return true;
    }

    void SetClipboard(std::string text) override { m_Clipboard = std::move(text); }
    std::string Clipboard() const override { return m_Clipboard; }

    void Copy() override
    {
        const auto sel = m_Caret.Selection();
        if (!sel.Empty()) {
            m_Clipboard = SliceUtf8(sel.Start(), sel.End());
        }
    }

    void Cut() override
    {
        Copy();
        DeleteSelection();
    }

    void Paste() override
    {
        if (!m_Clipboard.empty()) {
            Insert(m_Clipboard);
        }
    }

    void SetComposition(const std::string_view utf8, const size_t cursorInComposition) override
    {
        m_Composition = std::string(utf8);
        m_CompositionCursor = cursorInComposition;
    }

    void ConfirmComposition() override
    {
        if (!m_Composition.empty()) {
            Insert(m_Composition);
        }
        m_Composition.clear();
        m_CompositionCursor = 0;
    }

    void CancelComposition() override
    {
        m_Composition.clear();
        m_CompositionCursor = 0;
    }

    bool HasComposition() const override { return !m_Composition.empty(); }
    std::string_view Composition() const override { return m_Composition; }

private:
    std::string SliceUtf8(const size_t start, const size_t end) const
    {
        if (start >= end || start >= m_Codepoints.size()) {
            return {};
        }
        const size_t clampedEnd = std::min(end, m_Codepoints.size());
        return unicode::EncodeUtf8(
            std::span(m_Codepoints.data() + start, clampedEnd - start));
    }

    void RebuildFromCodepoints()
    {
        m_Text = unicode::EncodeUtf8(m_Codepoints);
    }

    void Apply(const EditCommand& cmd)
    {
        if (cmd.type == EditCommand::Type::Insert || cmd.type == EditCommand::Type::Replace) {
            auto inserted = unicode::DecodeUtf8(cmd.text);
            if (cmd.type == EditCommand::Type::Replace || cmd.end > cmd.start) {
                m_Codepoints.erase(
                    m_Codepoints.begin() + static_cast<std::ptrdiff_t>(cmd.start),
                    m_Codepoints.begin() + static_cast<std::ptrdiff_t>(cmd.end));
            }
            m_Codepoints.insert(
                m_Codepoints.begin() + static_cast<std::ptrdiff_t>(cmd.start),
                inserted.begin(),
                inserted.end());
            m_Caret.SetOffset(cmd.start + inserted.size());
        } else if (cmd.type == EditCommand::Type::Delete) {
            m_Codepoints.erase(
                m_Codepoints.begin() + static_cast<std::ptrdiff_t>(cmd.start),
                m_Codepoints.begin() + static_cast<std::ptrdiff_t>(cmd.end));
            m_Caret.SetOffset(cmd.start);
        }
        RebuildFromCodepoints();
    }

    void Invert(const EditCommand& cmd)
    {
        if (cmd.type == EditCommand::Type::Insert) {
            EditCommand undo;
            undo.type = EditCommand::Type::Delete;
            undo.start = cmd.start;
            undo.end = cmd.start + unicode::DecodeUtf8(cmd.text).size();
            Apply(undo);
        } else if (cmd.type == EditCommand::Type::Delete) {
            EditCommand undo;
            undo.type = EditCommand::Type::Insert;
            undo.start = cmd.start;
            undo.end = cmd.start;
            undo.text = cmd.previous;
            Apply(undo);
        }
    }

    void PushUndo(const EditCommand& cmd)
    {
        m_Undo.push_back(cmd);
        m_Redo.clear();
        if (m_Undo.size() > 128) {
            m_Undo.erase(m_Undo.begin());
        }
    }

    std::string m_Text;
    std::vector<Codepoint> m_Codepoints;
    CaretEngine m_Caret;
    TextSelectionEngine m_Selection;
    std::vector<EditCommand> m_Undo;
    std::vector<EditCommand> m_Redo;
    std::string m_Clipboard;
    std::string m_Composition;
    size_t m_CompositionCursor = 0;
};

} // namespace

std::unique_ptr<ITextEditSession> CreateTextEditSession(std::string initial)
{
    return std::make_unique<TextEditSession>(std::move(initial));
}

} // namespace we::runtime::text::editing
