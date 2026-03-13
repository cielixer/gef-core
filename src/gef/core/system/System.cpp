#include <gef/core/system/System.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace gef {

auto System::loadModule(const std::filesystem::path& path)
    -> std::expected<ModuleId, Error> {
    spdlog::debug("Loading module from: {}", path.string());

    auto name = loadAtomicModule(module_registry_, path);
    if (!name) [[unlikely]] {
        return std::unexpected(std::move(name.error()));
    }

    auto atomic = getAtomicModule(module_registry_, *name);
    if (!atomic) [[unlikely]] {
        return std::unexpected(std::move(atomic.error()));
    }

    ModuleSignature sig;
    sig.version = (*atomic)->metadata->version;
    for (std::size_t i = 0; i < (*atomic)->metadata->num_bindings; ++i) {
        const auto& b = (*atomic)->metadata->bindings[i];
        sig.bindings.push_back(Binding{
            b.name,
            static_cast<BindingRole>(b.role),
            b.type_name,
        });
    }

    auto existing_id = module_registry_.find(*name);
    if (existing_id) {
        auto existing_def = module_registry_.get(*existing_id);
        if (!existing_def) [[unlikely]] {
            return std::unexpected(std::move(existing_def.error()));
        }

        (*existing_def)->signature = std::move(sig);
        (*existing_def)->variant = AtomicModule{(*atomic)->handle,
                                                (*atomic)->metadata,
                                                (*atomic)->execute};

        spdlog::info("Reloaded module '{}' with id {}", *name, *existing_id);
        return *existing_id;
    }

    if (existing_id.error().code != ErrorCode::ModuleNotFound) [[unlikely]] {
        return std::unexpected(std::move(existing_id.error()));
    }

    ModuleDef def{
        .name      = *name,
        .signature = std::move(sig),
        .variant   = AtomicModule{(*atomic)->handle, (*atomic)->metadata, (*atomic)->execute},
    };

    auto id = module_registry_.add(std::move(def));
    spdlog::info("Registered module '{}' with id {}", *name, id);
    return id;
}

auto System::executeModule(std::string_view name, Context& ctx)
    -> std::expected<void, Error> {
    if (name.empty()) [[unlikely]] {
        throw std::invalid_argument("Module name cannot be empty");
    }

    auto id = module_registry_.find(name);
    if (!id) [[unlikely]] {
        return std::unexpected(std::move(id.error()));
    }

    return executeModule(*id, ctx);
}

auto System::executeModule(ModuleId id, Context& ctx)
    -> std::expected<void, Error> {
    auto def = module_registry_.get(id);
    if (!def) [[unlikely]] {
        return std::unexpected(std::move(def.error()));
    }

    std::visit(overloaded{
        [&](const AtomicModule& a) { a.execute(ctx); },
        [&](const FlowModule&) {},
        [&](const PipelineModule&) {},
    }, (*def)->variant);

    return {};
}

auto System::moduleRegistry() const noexcept -> const ModuleRegistry& {
    return module_registry_;
}

auto System::moduleRegistry() noexcept -> ModuleRegistry& {
    return module_registry_;
}

} // namespace gef
