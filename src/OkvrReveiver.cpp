#include <cryptoTools/Common/block.h>
#include <cstdint>
#include <tuple>
#include <vector>
#include "OkvrReceiver.h"

OkvrReceiver::OkvrReceiver(std::vector<oc::block> keys, u64 numItems_, u64 num_query) : numItems(numItems_)
{
    okvs = new SparseOKVS(numItems_);

    pir_parms = new PirParms(okvs->sizeOfSparse(), sizeof(oc::block), num_query, true, true);

    client = new Client(*pir_parms);
}

std::stringstream OkvrReceiver::save_keys()
{
    return client->save_keys();
}

std::tuple<std::stringstream, std::vector<uint32_t>> OkvrReceiver::genQuery(std::vector<std::vector<u64>> &idxs)
{
    vector<uint32_t> batch_query_index(idxs.size() * 3);

    for (u64 i = 0; i < idxs.size(); i++) {
        for (u64 j = 0; j < 3; j++) {
            batch_query_index[i * 3 + j] = idxs[i][j] + idxs[i][3] * okvs->binSize; //  real pos = pos in the bin + bin index * bin size
        }
    }

    return std::make_tuple(client->gen_batch_query(batch_query_index), batch_query_index);
}

std::tuple<vector<block>, vector<vector<u64>>> OkvrReceiver::genIndex(std::vector<oc::block> &keys)
{
    vector<block> hashs(keys.size());
    vector<vector<u64>> idxs(keys.size(), vector<u64>(3 + 1)); // 3 postions + 1 bin index

    okvs->computeIndex(keys, hashs, idxs, 0);

    return std::make_tuple(hashs, idxs);
}

std::vector<oc::block> OkvrReceiver::decode(
    std::stringstream &response,
    std::vector<uint32_t> &batch_query_index,
    std::vector<oc::block> &hashs,
    std::vector<std::vector<u64>> &idxs,
    std::vector<oc::block> &densePart)
{
    std::vector<oc::block> values(hashs.size());

    std::vector<oc::block> retrievalBlocks;

    std::vector<std::vector<uint64_t>> answer = client->extract_batch_answer(response);

    client->getBlockFromAnswer(retrievalBlocks, answer, batch_query_index);

    vector<std::map<u64, block>> pp(okvs->binNum, std::map<u64, block>());

    for (u64 i = 0; i < okvs->binNum; i++) {
        for (u64 j = 0; j < okvs->denseSize; j++) {
            pp[i][okvs->sparseSize + j] = densePart[i * okvs->denseSize + j];
        }
    }
    for (u64 i = 0; i < idxs.size(); i++) {
        for (u64 j = 0; j < 3; j++) {
            pp[idxs[i][3]][idxs[i][j]] = retrievalBlocks[i * 3 + j];
            std::cout << retrievalBlocks[i * 3 + j] << std::endl;
        }
    }

    okvs->decode(hashs, idxs, values, pp, 0);

    return values;
}

OkvrReceiver::~OkvrReceiver()
{
    delete okvs;
    delete client;
}