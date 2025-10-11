#pragma once

#include <array>
#include <coproto/Socket/AsioSocket.h>
#include <coproto/Socket/LocalAsyncSock.h>
#include <cryptoTools/Common/block.h>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <macoro/sync_wait.h>
#include <secure-join/Defines.h>
#include <vector>
#include "Defines.h"
#include "OkvrReceiver.h"
#include "OkvrSender.h"
#include "SiOPRF.h"
#include "SoOPPRF.h"
#include "SoOPRF.h"
#include "common.h"
#include "cryptoTools/Common/CLP.h"
#include "eq.h"
#include "secure-join/Prf/AltModPrf.h"
#include "secure-join/Prf/AltModPrfProto.h"

void LocalMap(std::vector<std::vector<u64>> &inputs, std::vector<block> &pid, std::vector<block> &list_key, std::vector<block> &list_val, int delta);