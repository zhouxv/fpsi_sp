// #pragma once

// #include <cryptoTools/Common/block.h>
// #include <vector>
// #include "OKVS.h"
// #include "client.h"
// #include "pir_parms.h"

// class OkvrReceiver {
// public:
//     SparseOKVS *okvs;
//     OkvrReceiver(std::vector<oc::block> keys, u64 numItems_, u64 num_query);

//     std::tuple<std::stringstream, std::vector<uint32_t>> genQuery(std::vector<std::vector<u64>> &idxs);

//     std::stringstream save_keys();

//     std::tuple<vector<block>, vector<vector<u64>>> genIndex(std::vector<oc::block> &keys);

//     std::vector<oc::block> decode(
//         std::stringstream &response,
//         std::vector<uint32_t> &batch_query_index,
//         std::vector<oc::block> &hashs,
//         std::vector<std::vector<u64>> &idxs,
//         std::vector<block> &densePart);

//     ~OkvrReceiver();

//     int numItems;
//     vector<oc::block> E;
//     Client *client;
//     PirParms *pir_parms;
// };