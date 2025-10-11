#pragma once

#include <cryptoTools/Common/block.h>
#include <sstream>
#include <vector>
#include "OKVS.h"
#include "server.h"

class OkvrSender {
public:
    OkvrSender(std::vector<oc::block> keys, std::vector<oc::block> values, u64 numItems_, u64 num_query);

    void set_keys(std::stringstream &keys);

    std::stringstream genResponse(std::stringstream &query);

    std::vector<oc::block> getDensePart();

    std::vector<oc::block> getSparsePart();

    ~OkvrSender();

    SparseOKVS *okvs;
    int numItems;
    vector<oc::block> sparsePart;
    vector<oc::block> densePart;
    Server *server;
    PirParms *pir_parms;
};