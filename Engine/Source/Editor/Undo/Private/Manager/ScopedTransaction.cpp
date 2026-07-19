#include "UndoInternal.h"

namespace we::editor::undo {
namespace detail {

class ScopedTransactionImpl final : public IScopedTransaction {
public:
    ScopedTransactionImpl(ITransactionManager& manager, TransactionId id)
        : m_Manager(manager)
        , m_Id(id)
        , m_Open(id.IsValid())
    {}

    ~ScopedTransactionImpl() override {
        if (m_Open) {
            (void)m_Manager.CancelTransaction();
            m_Open = false;
        }
    }

    [[nodiscard]] bool IsOpen() const noexcept override { return m_Open; }
    [[nodiscard]] TransactionId Id() const noexcept override { return m_Id; }

    void End() override {
        if (!m_Open) {
            return;
        }
        (void)m_Manager.EndTransaction();
        m_Open = false;
    }

    void Cancel() override {
        if (!m_Open) {
            return;
        }
        (void)m_Manager.CancelTransaction();
        m_Open = false;
    }

private:
    ITransactionManager& m_Manager;
    TransactionId m_Id{};
    bool m_Open = false;
};

} // namespace detail

std::unique_ptr<IScopedTransaction> BeginScopedTransaction(
    ITransactionManager& manager,
    std::string_view label,
    TransactionKind kind,
    UndoWorldKey worldKey)
{
    TransactionDescriptor desc;
    desc.label = std::string(label);
    desc.kind = kind;
    desc.worldKey = worldKey;
    const TransactionId id = manager.BeginTransaction(desc);
    return std::make_unique<detail::ScopedTransactionImpl>(manager, id);
}

} // namespace we::editor::undo
