// #include "OkvrSender.h"

// OkvrSender::OkvrSender(std::vector<oc::block> keys, std::vector<oc::block> values, u64 numItems_, u64 num_query) : numItems(numItems_)
// {
//     okvs = new SparseOKVS(numItems_);

//     okvs->encode(keys, values, sparsePart, densePart);

//     pir_parms = new PirParms(okvs->sizeOfSparse(), sizeof(oc::block), num_query, true, true);

//     server = new Server(*pir_parms, true, sparsePart);
// };

// void OkvrSender::set_keys(std::stringstream &keys)
// {
//     server->set_keys(keys);
// };

// std::vector<oc::block> OkvrSender::getDensePart()
// {
//     return densePart;
// };

// std::vector<oc::block> OkvrSender::getSparsePart()
// {
//     return sparsePart;
// };

// std::stringstream OkvrSender::genResponse(std::stringstream &query)
// {
//     return server->gen_batch_response(query);
// };

// OkvrSender::~OkvrSender()
// {
//     sparsePart.clear();
//     densePart.clear();
//     delete okvs;
//     delete server;
// };