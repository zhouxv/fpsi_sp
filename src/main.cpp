#include <coproto/Socket/AsioSocket.h>
#include <cryptoTools/Common/block.h>
#include <cstddef>
#include <cstring>
#include <secure-join/Defines.h>
#include <vector>
#include "OkvrReceiver.h"
#include "OkvrSender.h"
#include "common.h"
#include "cryptoTools/Common/CLP.h"
#include "secure-join/Prf/AltModPrfProto.h"

using namespace secJoin;

std::map<std::string, TimerStat> timers;

void AltModWPrf_proto_bench(const oc::CLP &cmd)
{
    u64 n = cmd.getOr("n", 1ull << cmd.getOr("nn", 10));
    u64 trials = cmd.getOr("trials", 1);
    bool nt = cmd.getOr("nt", 1);
    auto useOle = cmd.isSet("ole");

    oc::Timer timer;

    AltModWPrfSender sender;
    AltModWPrfReceiver recver;

    sender.mUseMod2F4Ot = !useOle;
    recver.mUseMod2F4Ot = !useOle;

    sender.setTimer(timer);
    recver.setTimer(timer);

    std::vector<oc::block> x(n);
    std::vector<oc::block> y0(n), y1(n);

    auto sock = coproto::AsioSocket::makePair();
    auto sock2 = coproto::AsioSocket::makePair();

    macoro::thread_pool pool0;
    auto e0 = pool0.make_work();
    pool0.create_threads(nt);
    macoro::thread_pool pool1;
    auto e1 = pool1.make_work();
    pool1.create_threads(nt);
    sock[0].setExecutor(pool0);
    sock[1].setExecutor(pool1);
    sock2[0].setExecutor(pool0);
    sock2[1].setExecutor(pool1);

    PRNG prng0(oc::ZeroBlock);
    PRNG prng1(oc::OneBlock);

    AltModPrf dm;
    AltModPrf::KeyType kk;
    kk = prng0.get();
    dm.setKey(kk);
    // sender.setKey(kk);

    CorGenerator ole0, ole1;
    ole0.init(sock2[0].fork(), prng0, 0, nt, 1 << 18, cmd.getOr("mock", 1));
    ole1.init(sock2[1].fork(), prng1, 1, nt, 1 << 18, cmd.getOr("mock", 1));

    prng0.get(x.data(), x.size());
    std::vector<oc::block> rk(AltModPrf::KeySize);
    std::vector<std::array<oc::block, 2>> sk(AltModPrf::KeySize);
    for (u64 i = 0; i < AltModPrf::KeySize; ++i) {
        sk[i][0] = oc::block(i, 0);
        sk[i][1] = oc::block(i, 1);
        rk[i] = oc::block(i, *oc::BitIterator((u8 *)&sender.mKeyMultRecver.mKey, i));
    }
    sender.setKeyOts(kk, rk);
    recver.setKeyOts(sk);
    u64 numOle = 0;
    u64 numF4BitOt = 0;
    u64 numOt = 0;

    auto begin = timer.setTimePoint("begin");
    for (u64 t = 0; t < trials; ++t) {
        sender.init(n, ole0);
        recver.init(n, ole1);

        numOle += ole0.mGenState->mNumOle;
        numF4BitOt += ole0.mGenState->mNumF4BitOt;
        numOt += ole0.mGenState->mNumOt;

        auto r = coproto::sync_wait(coproto::when_all_ready(
            ole0.start() | macoro::start_on(pool0),
            ole1.start() | macoro::start_on(pool1),
            sender.evaluate({}, y0, sock[0], prng0) | macoro::start_on(pool0),
            recver.evaluate(x, y1, sock[1], prng1) | macoro::start_on(pool1)));
        std::get<0>(r).result();
        std::get<1>(r).result();
        std::get<2>(r).result();
        std::get<3>(r).result();
    }
    auto end = timer.setTimePoint("end");

    auto ntr = n * trials;

    std::cout << "AltModWPrf n:" << n << ", " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / double(ntr) << "ns/eval "
              << sock[0].bytesSent() / double(ntr) << "+" << sock[0].bytesReceived() / double(ntr) << "="
              << (sock[0].bytesSent() + sock[0].bytesReceived()) / double(ntr) << " bytes/eval ";

    std::cout << numOle / double(ntr) << " ole/eval ";
    std::cout << numF4BitOt / double(ntr) << " f4/eval ";
    std::cout << numOt / double(ntr) << " ot/eval ";

    std::cout << std::endl;

    if (cmd.isSet("v")) {
        std::cout << timer << std::endl;
        std::cout << sock[0].bytesReceived() / 1000.0 << " " << sock[0].bytesSent() / 1000.0 << " kB " << std::endl;
    }
}

auto sender(std::vector<block> &y0, CorGenerator &ole, AltModPrf &dm, AltModWPrfSender &s, coproto::Socket &sock, PRNG &prng)
{
    size_t n = y0.size();

    std::vector<oc::block> rk(AltModPrf::KeySize);
    for (u64 i = 0; i < AltModPrf::KeySize; ++i) {
        rk[i] = oc::block(i, *oc::BitIterator((u8 *)&dm.mExpandedKey, i));
    }

    s.init(n, ole, AltModPrfKeyMode::SenderOnly, AltModPrfInputMode::ReceiverOnly, dm.getKey(), rk);

    return s.evaluate({}, y0, sock, prng);
}

void AltModWPrf_proto_test(const oc::CLP &cmd)
{
    u64 n = cmd.getOr("n", 1024);
    bool noCheck = cmd.isSet("nc");
    bool debug = cmd.isSet("debug");

    oc::Timer timer;

    AltModWPrfSender sender;
    AltModWPrfReceiver recver;
    sender.mDebug = debug;
    recver.mDebug = debug;

    sender.setTimer(timer);
    recver.setTimer(timer);

    std::vector<oc::block> x(n);
    std::vector<oc::block> y0(n), y1(n);

    auto sock = coproto::AsioSocket::makePair();

    PRNG prng0(oc::ZeroBlock);
    PRNG prng1(oc::OneBlock);

    AltModPrf dm(prng0.get());

    CorGenerator ole0, ole1;
    ole0.init(sock[0].fork(), prng0, 0, 1, 1 << 18, cmd.getOr("mock", 1));
    ole1.init(sock[1].fork(), prng1, 1, 1, 1 << 18, cmd.getOr("mock", 1));

    prng0.get(x.data(), x.size());

    if (cmd.isSet("doKeyGen") == false) {
        std::vector<oc::block> rk(AltModPrf::KeySize);
        std::vector<std::array<oc::block, 2>> sk(AltModPrf::KeySize);
        for (u64 i = 0; i < AltModPrf::KeySize; ++i) {
            sk[i][0] = oc::block(i, 0);
            sk[i][1] = oc::block(i, 1);
            rk[i] = oc::block(i, *oc::BitIterator((u8 *)&dm.mExpandedKey, i));
        }
        sender.init(n, ole0, AltModPrfKeyMode::SenderOnly, AltModPrfInputMode::ReceiverOnly, dm.getKey(), rk);
        recver.init(n, ole1, AltModPrfKeyMode::SenderOnly, AltModPrfInputMode::ReceiverOnly, {}, sk);
    } else {
        sender.init(n, ole0);
        recver.init(n, ole1);
    }

    auto r = coproto::sync_wait(
        coproto::when_all_ready(sender.evaluate({}, y0, sock[0], prng0), recver.evaluate(x, y1, sock[1], prng1), ole0.start(), ole1.start()));

    std::get<0>(r).result();
    std::get<1>(r).result();
    std::get<2>(r).result();
    std::get<3>(r).result();

    if (cmd.isSet("v")) {
        std::cout << timer << std::endl;
        std::cout << sock[0].bytesReceived() / 1000.0 << " " << sock[0].bytesSent() / 1000.0 << " kB " << std::endl;
    }

    if (noCheck)
        return;

    AltModPrf prf(sender.getKey());
    std::vector<block> y(x.size());
    prf.eval(x, y);
    for (u64 ii = 0; ii < n; ++ii) {
        auto yy = (y0[ii] ^ y1[ii]);
        if (yy != y[ii]) {
            std::cout << "i   " << ii << std::endl;
            std::cout << "act " << yy << std::endl;
            std::cout << "exp " << y[ii] << std::endl;
            throw RTE_LOC;
        }
    }
}

void simulation_2pc()
{
    u64 n = 1024;
    bool debug = false, doKeyGen = false, verbose = true, noCheck = false;
    int mock = 1;

    AltModWPrfSender sender;
    AltModWPrfReceiver recver;
    sender.mDebug = recver.mDebug = debug;

    oc::Timer timer;
    sender.setTimer(timer);
    recver.setTimer(timer);

    // 两端 socket（同进程内但走 TCP 栈）
    auto pair = coproto::AsioSocket::makePair();

    PRNG prng0(oc::ZeroBlock), prng1(oc::OneBlock);

    // 公共 key 准备（可按你的分工调整）
    AltModPrf dm(prng0.get());
    std::vector<oc::block> rk(AltModPrf::KeySize);
    std::vector<std::array<oc::block, 2>> sk(AltModPrf::KeySize);
    for (u64 i = 0; i < AltModPrf::KeySize; ++i) {
        sk[i][0] = oc::block(i, 0);
        sk[i][1] = oc::block(i, 1);
        rk[i] = oc::block(i, *oc::BitIterator((u8 *)&dm.mExpandedKey, i));
    }

    std::vector<oc::block> x(n), y0(n), y1(n);
    prng0.get(x.data(), x.size());

    CorGenerator ole0, ole1;

    std::thread th_sender([&] {
        ole0.init(pair[0].fork(), prng0, 0, 1, 1 << 18, mock);
        if (!doKeyGen)
            sender.init(n, ole0, AltModPrfKeyMode::SenderOnly, AltModPrfInputMode::ReceiverOnly, dm.getKey(), rk);
        else
            sender.init(n, ole0);

        auto all = when_all_ready(sender.evaluate({}, y0, pair[0], prng0), ole0.start());
        auto r = sync_wait(std::move(all));
        std::get<0>(r).result();
        std::get<1>(r).result();
    });

    std::thread th_receiver([&] {
        ole1.init(pair[1].fork(), prng1, 1, 1, 1 << 18, mock);
        if (!doKeyGen)
            recver.init(n, ole1, AltModPrfKeyMode::SenderOnly, AltModPrfInputMode::ReceiverOnly, {}, sk);
        else
            recver.init(n, ole1);

        auto all = when_all_ready(recver.evaluate(x, y1, pair[1], prng1), ole1.start());
        auto r = sync_wait(std::move(all));
        std::get<0>(r).result();
        std::get<1>(r).result();
    });

    th_sender.join();
    th_receiver.join();

    if (verbose) {
        std::cout << timer << "\n";
        std::cout << pair[0].bytesReceived() / 1000.0 << " " << pair[0].bytesSent() / 1000.0 << " kB\n";
    }

    if (!noCheck) {
        AltModPrf prf(sender.getKey());
        std::vector<oc::block> y(n);
        prf.eval(x, y);
        for (u64 i = 0; i < n; ++i) {
            if ((y0[i] ^ y1[i]) != y[i])
                throw RTE_LOC;
        }
        if (verbose)
            std::cout << "check pass\n";
    }
}

void sim_okvr()
{
    u64 n = 1 << 11;
    std::vector<block> key(n), val(n);

    auto decode_size = 10;
    auto numQuery = 3 * decode_size;

    std::vector<block> decode_val(decode_size);
    std::vector<block> decode_key(decode_size);

    PRNG prng(ZeroBlock);
    prng.get<block>(key);
    prng.get<block>(val);

    memcpy(decode_key.data(), key.data(), decode_size * sizeof(block));

    // offline phase

    OkvrSender sender(key, val, n, numQuery);
    OkvrReceiver receiver(decode_key, n, numQuery);

    auto seal_key = receiver.save_keys();

    sender.set_keys(seal_key);

    // online phase

    start_timer("online");

    auto [hashs, idxs] = receiver.genIndex(decode_key);

    auto [query, batch_query_index] = receiver.genQuery(idxs);

    auto response = sender.genResponse(query);

    auto densePart = sender.getDensePart();
    auto sparsePart = sender.getSparsePart();

    auto okvs = receiver.okvs;

    std::vector<block> values(hashs.size());

    vector<std::map<u64, block>> pp(okvs->binNum, std::map<u64, block>());

    for (u64 i = 0; i < okvs->binNum; i++) {
        for (u64 j = 0; j < okvs->denseSize; j++) {
            pp[i][okvs->sparseSize + j] = densePart[i * okvs->denseSize + j];
        }
    }
    for (u64 i = 0; i < idxs.size(); i++) {
        for (u64 j = 0; j < 3; j++) {
            pp[idxs[i][3]][idxs[i][j]] = sparsePart[idxs[i][3] * okvs->sparseSize + idxs[i][j]];
            std::cout << sparsePart[idxs[i][3] * okvs->sparseSize + idxs[i][j]] << std::endl;
        }
    }
    std::cout << "---------------------------------" << std::endl;
    okvs->decode(hashs, idxs, values, pp, 0);

    // auto as = receiver.client->extract_batch_answer(response);

    // test_batch_pir_correctness(*sender.server, as, batch_query_index, *receiver.pir_parms);

    receiver.decode(response, batch_query_index, hashs, idxs, densePart);

    stop_timer("online");

    print_all_timers("");

    memcmp(values.data(), val.data(), decode_size * sizeof(block)) == 0 ? std::cout << "ok\n" : std::cout << "error\n";
}

int main(int argc, char **argv)
{
    // oc::CLP cmd(argc, argv);

    // AltModWPrf_proto_bench(cmd);

    // AltModWPrf_proto_test(cmd);

    // simulation_2pc();

    sim_okvr();

    return 0;
}