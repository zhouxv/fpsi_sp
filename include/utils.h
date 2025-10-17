#pragma once

#include <cryptoTools/Common/Defines.h>
#include <tuple>
#include <vector>

using namespace osuCrypto;

std::tuple<std::vector<std::vector<u64>>, std::vector<std::vector<u64>>> genInputs(u64 n, u64 d);