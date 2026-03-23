#include "gef/core/system/System.h"

#include "gef/core/module/AtomicModule.h"
#include "gef/core/module/ModuleVariant.h"

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace gef {

auto loadModule(System& system, const std::filesystem::path& path)
    -> std::expected<ModuleId, Error> {
    spdlog::debug("Loading module from: {}", path.string());

    auto name = loadAtomicModule(system.module_registry, path);
    if (!name) [[unlikely]] {
        return std::unexpected(std::move(name.error()));
    }

    auto atomic = getAtomicModule(system.module_registry, *name);
    if (!atomic) [[unlikely]] {
        return std::unexpected(std::move(atomic.error()));
    }

    AtomicModule cloned = cloneAtomicModuleNonOwning(**atomic);

    const auto* metadata = moduleMetadata(cloned);
    if (!metadata) [[unlikely]] {
        return std::unexpected(Error{ErrorCode::MetadataInvalid,
                                     "Module metadata is null"});
    }

    ModuleSignature sig;
    sig.version = metadata->version;
    for (std::size_t i = 0; i < metadata->num_bindings; ++i) {
        const auto& b = metadata->bindings[i];
        sig.bindings.push_back(Binding{
            b.name,
            static_cast<BindingRole>(b.role),
            b.type_name,
        });
    }

    auto existing_id = registryFind(system.module_registry, *name);
    if (existing_id) {
        auto existing_def = registryGet(system.module_registry, *existing_id);
        if (!existing_def) [[unlikely]] {
            return std::unexpected(std::move(existing_def.error()));
        }

        (*existing_def)->signature = std::move(sig);
        (*existing_def)->variant   = std::move(cloned);

        spdlog::info("Reloaded module '{}' with id {}", *name, *existing_id);
        return *existing_id;
    }

    if (existing_id.error().code != ErrorCode::ModuleNotFound) [[unlikely]] {
        return std::unexpected(std::move(existing_id.error()));
    }

    ModuleDef def{
        .name      = *name,
        .signature = std::move(sig),
        .variant   = std::move(cloned),
    };

    auto id = registryAdd(system.module_registry, std::move(def));
    spdlog::info("Registered module '{}' with id {}", *name, id);
    return id;
}

auto executeModule(System& system, std::string_view name, Context& ctx)
    -> std::expected<void, Error> {
    if (name.empty()) [[unlikely]] {
        throw std::invalid_argument("Module name cannot be empty");
    }

    auto id = registryFind(system.module_registry, name);
    if (!id) [[unlikely]] {
        return std::unexpected(std::move(id.error()));
    }

    return executeModule(system, *id, ctx);
}

auto executeModule(System& system, ModuleId id, Context& ctx)
    -> std::expected<void, Error> {
    auto def = registryGet(system.module_registry, id);
    if (!def) [[unlikely]] {
        return std::unexpected(std::move(def.error()));
    }

    std::visit(overloaded{
        [&](const AtomicModule& atomic) {
            executeAtomicModule(atomic, ctx);
        },
        [&](const FlowModule&) {},
        [&](const PipelineModule&) {},
    }, (*def)->variant);

    return {};
}

} // namespace gef
