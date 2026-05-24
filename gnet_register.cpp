/// @file   plugins/protocols/gnet/gnet_register.cpp
/// @brief  C vtable adapter for GnetProtocol + gn_gnet_register_protocol.
///
/// Wraps the C++ `GnetProtocol` in a `gn_protocol_layer_vtable_t` so
/// host programs that do not have access to kernel-internal C++ headers
/// can register the built-in gnet-v1 framing layer through the public
/// C ABI (`gn_core_register_protocol`).
///
/// Thread-safety: `GnetProtocol::deframe` already uses a static
/// thread-local buffer per the class doc ("reused across calls on the
/// same thread").  The vtable functions here are stateless bridges; the
/// per-instance state is only the `GnetProtocol` member whose deframe
/// result points into the same TLS buffer.

#include "protocol.hpp"

#include <cstdlib>
#include <cstring>
#include <new>

#include <sdk/core.h>
#include <sdk/gnet.h>
#include <sdk/protocol.h>
#include <sdk/types.h>

namespace {

struct GnetInstance {
    gn::plugins::gnet::GnetProtocol proto;
};

static const char* vtable_protocol_id(void* /*self*/) noexcept {
    return "gnet-v1";
}

static gn_result_t vtable_deframe(void* self,
                                   gn_connection_context_t* ctx,
                                   const uint8_t* bytes,
                                   size_t bytes_size,
                                   gn_deframe_result_t* out) noexcept {
    auto* inst = static_cast<GnetInstance*>(self);
    auto res = inst->proto.deframe(*ctx, {bytes, bytes_size});
    if (!res) return res.error().code;
    // messages span points into GnetProtocol's thread-local buffer.
    // Valid until the next deframe call on this thread, which is after
    // the kernel has dispatched all messages — safe per the contract.
    out->messages       = res->messages.data();
    out->count          = res->messages.size();
    out->bytes_consumed = res->bytes_consumed;
    return GN_OK;
}

static gn_result_t vtable_frame(void* self,
                                 gn_connection_context_t* ctx,
                                 const gn_message_t* msg,
                                 uint8_t** out_bytes,
                                 size_t* out_size,
                                 void** out_user_data,
                                 void (**out_free)(void* ud,
                                                   uint8_t* b)) noexcept {
    auto* inst = static_cast<GnetInstance*>(self);
    auto res = inst->proto.frame(*ctx, *msg);
    if (!res) return res.error().code;
    const size_t n = res->size();
    // Allocate with malloc so the kernel can free via the callback.
    uint8_t* buf = static_cast<uint8_t*>(std::malloc(n ? n : 1));
    if (!buf && n) return GN_ERR_OUT_OF_MEMORY;
    if (n) std::memcpy(buf, res->data(), n);
    *out_bytes     = buf;
    *out_size      = n;
    *out_user_data = nullptr;
    *out_free      = [](void* /*ud*/, uint8_t* b) noexcept { std::free(b); };
    return GN_OK;
}

static size_t vtable_max_payload_size(void* self) noexcept {
    return static_cast<GnetInstance*>(self)->proto.max_payload_size();
}

static void vtable_destroy(void* self) noexcept {
    delete static_cast<GnetInstance*>(self);
}

static uint32_t vtable_allowed_trust_mask(void* self) noexcept {
    return static_cast<GnetInstance*>(self)->proto.allowed_trust_mask();
}

static constexpr gn_protocol_layer_vtable_t kGnetVtable = {
    .api_size           = sizeof(gn_protocol_layer_vtable_t),
    .protocol_id        = vtable_protocol_id,
    .deframe            = vtable_deframe,
    .frame              = vtable_frame,
    .max_payload_size   = vtable_max_payload_size,
    .destroy            = vtable_destroy,
    .allowed_trust_mask = vtable_allowed_trust_mask,
    ._reserved          = {nullptr, nullptr, nullptr, nullptr},
};

} // namespace

extern "C"
gn_result_t gn_gnet_register_protocol(gn_core_t* core) {
    if (!core) return GN_ERR_NULL_ARG;
    auto* inst = new(std::nothrow) GnetInstance{};
    if (!inst) return GN_ERR_OUT_OF_MEMORY;
    const gn_result_t rc = gn_core_register_protocol(core, &kGnetVtable, inst);
    if (rc != GN_OK) delete inst;
    return rc;
}
