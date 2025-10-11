#pragma once

#include "OKVS.h"
#include "SoOPRF.h"

class SoOPPRFSender : public SoOPRFSender {
public:
    SoOPPRFSender(uint64_t num_, uint64_t numThreads_, bool useOle_, coproto::AsioSocket *socket_);
    ~SoOPPRFSender();

    void OPPRF(std::vector<oc::block> &keys, std::vector<oc::block> &values, std::vector<oc::block> &y0);

private:
    OKVS *okvs;
};

class SoOPPRFRecver : public SoOPRFRecver {
public:
    SoOPPRFRecver(uint64_t num_, uint64_t numThreads_, bool useOle_, coproto::AsioSocket *socket_);
    ~SoOPPRFRecver();

    void OPPRF(std::vector<oc::block> &keys, std::vector<oc::block> &y1);

private:
    OKVS *okvs;
};
