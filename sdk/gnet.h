/**
 * @file   sdk/gnet.h
 * @brief  Registration helper for the built-in gnet-v1 protocol layer.
 *
 * Host programs that link `GoodNet::protocol_gnet` call
 * `gn_gnet_register_protocol` once per kernel instance to install the
 * canonical mesh-framing layer. The call may happen at any time between
 * `gn_core_create` and the first `notify_connect` — typically right
 * after `gn_core_init` and before `gn_core_start`.
 *
 * @code
 * gn_core_t* core = gn_core_create();
 * gn_core_init(core);
 * gn_gnet_register_protocol(core);  // install gnet-v1 framing
 * gn_core_start(core);
 * @endcode
 */
#ifndef GOODNET_SDK_GNET_H
#define GOODNET_SDK_GNET_H

#include <sdk/abi.h>
#include <sdk/core.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register the gnet-v1 protocol layer on @p core.
 *
 * Creates a `GnetProtocol` instance and registers it via
 * `gn_core_register_protocol`. Safe to call before or after
 * `gn_core_start`; connections established after this call will use
 * the gnet-v1 framing.
 *
 * @param core  kernel instance; must be non-null.
 * @return @ref GN_OK on success, @ref GN_ERR_NULL_ARG when @p core is
 *         null, @ref GN_ERR_OUT_OF_MEMORY on allocation failure, or any
 *         error code returned by `gn_core_register_protocol`.
 */
GN_EXPORT gn_result_t gn_gnet_register_protocol(gn_core_t* core);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GOODNET_SDK_GNET_H */
