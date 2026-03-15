#include <gef/core/system/System.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace gef {

auto System::loadModule(const std::filesystem::path& path)
    -> std::expected<ModuleId, Error> {
    spdlog::debug("Loading module from: {}", path.string());

    auto atomic = loadAtomicModule(path);
    if (!atomic) [[unlikely]] {
        return std::unexpected(std::move(atomic.error()));
    }

    std::string name{atomic->metadata->module_name};

    ModuleSignature sig;
    sig.version = atomic->metadata->version;
    for (std::size_t i = 0; i < atomic->metadata->num_bindings; ++i) {
        const auto& b = atomic->metadata->bindings[i];
        sig.bindings.push_back(Binding{
            b.name,
            static_cast<BindingRole>(b.role),
            b.type_name,
        });
    }

    auto module_name = name;

    ModuleDef def{
        .name      = std::move(name),
        .signature = std::move(sig),
        .variant   = std::move(*atomic),
    };

    try {
        auto id = registerModule(this->module_store_, std::move(def));
        spdlog::info("Registered module '{}' with id {}", module_name, id);
        return id;
    } catch (const std::invalid_argument& e) {
        return std::unexpected(Error{ErrorCode::LoadFailed, e.what()});
    }
}

auto System::executeModule(std::string_view name, Context& ctx)
    -> std::expected<void, Error> {
    if (name.empty()) [[unlikely]] {
        throw std::invalid_argument("Module name cannot be empty");
    }

    auto id = findModule(this->module_store_, name);
    if (!id) [[unlikely]] {
        return std::unexpected(std::move(id.error()));
    }

    return executeModule(*id, ctx);
}

auto System::executeModule(ModuleId id, Context& ctx)
    -> std::expected<void, Error> {
    auto def = getModule(this->module_store_, id);
    if (!def) [[unlikely]] {
        return std::unexpected(std::move(def.error()));
    }

    std::visit(overloaded{
        [&](const AtomicModule& a) { a.execute(ctx); },
        [&](const CompositeModule&) {},
    }, (*def)->variant);

    return {};
}

auto System::moduleStore() const noexcept -> const ModuleStore& {
    return this->module_store_;
}

auto System::moduleStore() noexcept -> ModuleStore& {
    return this->module_store_;
}

} // namespace gef
