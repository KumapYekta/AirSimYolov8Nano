#pragma once

// AirSim baslıklarının tek noktadan dahil edildigi yer.
// AirLib'in rpclib bagımlılıgı msgpack makrosunu ve strict mode'u zorunlu kılıyor,
// bu kirliligi tek dosyada topluyoruz.

#include "common/common_utils/StrictMode.hpp"

STRICT_MODE_OFF

#ifndef RPCLIB_MSGPACK
#define RPCLIB_MSGPACK clmdep_msgpack
#endif

#include "rpc/rpc_error.h"

STRICT_MODE_ON

#include "common/ImageCaptureBase.hpp"
#include "vehicles/multirotor/api/MultirotorRpcLibClient.hpp"
