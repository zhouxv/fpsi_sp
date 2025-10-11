#include "SoOPRF.h"
#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <macoro/start_on.h>

SoOPRFSender::SoOPRFSender(uint64_t num_, uint64_t numThreads_, bool useOle_, coproto::AsioSocket *socket_)
    : num(num_), numThreads(numThreads_), useOle(useOle_), socket(socket_)
{
    sender = new AltModWPrfSender();
    pool = new macoro::thread_pool();
    auto e = pool->make_work();
    pool->create_threads(numThreads);
    sender->mUseMod2F4Ot = !useOle;
    // socket[0]->setExecutor(*pool);
    // socket[1]->setExecutor(*pool);
    prng = new PRNG(oc::ZeroBlock);

    // AltModPrf::KeyType kk;

    ole = new CorGenerator();
    ole->init(socket->fork(), *prng, 0, 1, 1 << 18, 1);

    // std::vector<oc::block> rk(AltModPrf::KeySize);

    // for (u64 i = 0; i < AltModPrf::KeySize; ++i) {
    //     rk[i] = oc::block(i, *oc::BitIterator((u8 *)&sender->mKeyMultRecver.mKey, i));
    // }

    // sender->setKeyOts(kk, rk);

    sender->init(num, *ole);
}

void SoOPRFSender::setup()
{
    // Setup implementation (if needed)
}

void SoOPRFSender::OPRF(std::vector<oc::block> &y0)
{
    // coproto::sync_wait(socket[0]->send(std::string("hello from sender\n")) | macoro::start_on(*pool));
    auto r = coproto::sync_wait(coproto::when_all_ready(ole->start(), sender->evaluate({}, y0, *socket, *prng)));

    std::get<0>(r).result();
    std::get<1>(r).result();
}

SoOPRFSender::~SoOPRFSender()
{
    delete sender;
    delete prng;
    delete ole;
}

SoOPRFRecver::SoOPRFRecver(uint64_t num_, uint64_t numThreads_, bool useOle_, coproto::AsioSocket *socket_)
    : num(num_), numThreads(numThreads_), useOle(useOle_), socket(socket_)
{
    recver = new AltModWPrfReceiver();
    pool = new macoro::thread_pool();
    auto e = pool->make_work();
    pool->create_threads(numThreads);
    recver->mUseMod2F4Ot = !useOle;
    // socket[0]->setExecutor(*pool);
    // socket[1]->setExecutor(*pool);
    prng = new PRNG(oc::OneBlock);

    // AltModPrf::KeyType kk;

    ole = new CorGenerator();
    ole->init(socket->fork(), *prng, 1, 1, 1 << 18, 1);

    // std::vector<std::array<oc::block, 2>> sk(AltModPrf::KeySize);

    // for (u64 i = 0; i < AltModPrf::KeySize; ++i) {
    //     sk[i][0] = oc::block(i, 0);
    //     sk[i][1] = oc::block(i, 1);
    // }

    // recver->setKeyOts(sk);

    recver->init(num, *ole);
};

void SoOPRFRecver::setup()
{
    // Setup implementation (if needed)
}

void SoOPRFRecver::OPRF(std::vector<oc::block> &x, std::vector<oc::block> &y1)
{
    // std::string str;
    // coproto::sync_wait(socket[0]->recvResize(str) | macoro::start_on(*pool));
    // std::cout << str << std::endl;
    auto r = coproto::sync_wait(coproto::when_all_ready(ole->start(), recver->evaluate(x, y1, *socket, *prng)));

    std::get<0>(r).result();
    std::get<1>(r).result();
}

SoOPRFRecver::~SoOPRFRecver()
{
    delete recver;
    delete prng;
    delete ole;
}