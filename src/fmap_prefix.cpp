#include "fmap_prefix.h"
#include <coproto/Socket/AsioSocket.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/block.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <map>
#include <vector>
#include "Defines.h"
#include "OKVS.h"
#include "SiOPRF.h"
#include "SoOPPRF.h"
#include "b2a.h"
#include "eq.h"
#include "fmap.h"
#include "mul.h"
#include "mux.h"
#include "secure-join/Prf/AltModPrf.h"
#include "utils.h"

using namespace secJoin;

// to be fixed
void LocalMapPrefix(std::vector<std::vector<u64>> &inputs, std::vector<block> &pid, std::vector<block> &listKey, std::vector<block> &listVal, int delta)
{
    PRNG prng(ZeroBlock);

    u64 m = inputs.size();
    u64 d = inputs[0].size();
    int prefixLen = static_cast<int>(std::ceil(std::log2(delta * 2 + 1)));

    pid.resize(m);

    std::vector<std::vector<std::pair<u64, u64>>> intervals(d);

    std::vector<block> randR(m * d);
    prng.get(randR.data(), randR.size());

    u64 maxInter = 0;

    // Merge overlapping intervals
    for (u64 i = 0; i < d; i++) {
        std::vector<std::pair<u64, u64>> interval;
        interval.reserve(m);

        // get interval [a_i - radius, a_i + radius]
        for (auto &elem : inputs) {
            interval.push_back({ elem[i] - delta, elem[i] + delta });
        }

        // Sort points by x-coordinate; if equal, sort by y-coordinate
        std::sort(interval.begin(), interval.end());

        for (auto [start, end] : interval) {
            // If intervals overlap, merge them
            // If no overlap, add the new interval
            if (!intervals[i].empty() && start <= intervals[i].back().second) {
                intervals[i].back().second = max(intervals[i].back().second, end);
            } else {
                intervals[i].emplace_back(start, end);
            }
            maxInter = max(intervals[i].back().second - intervals[i].back().first, maxInter);
        }
        std::cout << "Dimension " << i << " : total intervals = " << intervals[i].size() << std::endl;
    }

    // gen Local ID

    auto compare_lambda = [](const pair<u64, u64> &a, u64 value) {
        return a.second < value; // Find the first interval where value <= interval.second
    };

    for (u64 j = 0; j < m; j++) {
        auto &elem = inputs[j];
        for (u64 i = 0; i < d; i++) {
            auto it = std::lower_bound(intervals[i].begin(), intervals[i].end(), elem[i], compare_lambda);

            if (it != intervals[i].end() && it->first <= elem[i]) {
                auto interval_index = distance(intervals[i].begin(), it);
                pid[j] += randR[i * m + interval_index];
            } else {
                std::cout << i << " " << elem[i] << std::endl;
                throw runtime_error("recv getID random error");
            }
        }
    }

    // build listKey and listVal
    for (u64 i = 0; i < d; i++) {
        for (u64 j = 0; j < intervals[i].size(); j++) {
            auto [start, end] = intervals[i][j];
            auto randR_i_j = randR[i * m + j];
            u32 segNum = static_cast<u32>(std::ceil((end - start + 1) / double(2 * delta + 1))); // every sub interval at most length of (2*delta+1)
            for (u64 k = 0; k < segNum; k++) {
                u64 segStart = start + k * (2 * delta + 1);
                u64 segEnd = std::min(end, segStart + (2 * delta));
                auto prefixes = getIntervalPrefix(segStart, segEnd);
                for (auto &p : prefixes) {
                    block key = block(i << 32, 0) ^ p;
                    block val = block(0, low(randR_i_j));
                    listKey.push_back(key);
                    listVal.push_back(val);
                }
            }
        }
    }
    std::cout << listKey.size() << std::endl;
    if (listKey.size() > prefixLen * d * m) {
        throw runtime_error("something wrong in LocalMapPrefix");
    }

    while (listKey.size() < prefixLen * d * m) {
        listKey.push_back(prng.get<block>());
        listVal.push_back(prng.get<block>());
    }
}

void fuzzyPsiPrefix(const oc::CLP &cmd)
{
    u64 n = cmd.getOr("n", 1ull << cmd.getOr("nn", 10));
    size_t d = cmd.getOr("d", 2);
    int delta = cmd.getOr("delta", 2);

    int prefixLen = static_cast<int>(std::ceil(std::log2(delta * 2 + 1)));

    std::vector<std::vector<u64>> sendSet;
    std::vector<block> sendPid;
    std::vector<block> sendListKey;
    std::vector<block> sendListVal;

    PRNG prng(sysRandomSeed());

    for (u64 i = 0; i < n; i++) {
        std::vector<u64> tmp;
        for (u64 j = 0; j < d - 1; j++) {
            tmp.push_back(prng.get<u64>() + 2 * delta);
        }
        tmp.push_back(prng.get<u64>() + 2 * delta); // make sure there are some differences
        sendSet.push_back(tmp);
    }

    std::vector<std::vector<u64>> recvSet;
    std::vector<block> recvPid;
    std::vector<block> recvListKey;
    std::vector<block> recvListVal;

    for (u64 i = 0; i < n; i++) {
        std::vector<u64> tmp;
        for (u64 j = 0; j < d - 1; j++) {
            tmp.push_back(prng.get<u64>() + 2 * delta);
        }
        tmp.push_back(prng.get<u64>() + 2 * delta); // make sure there are some differences
        recvSet.push_back(tmp);
    }

    std::thread sendLocalMap([&] { LocalMapPrefix(sendSet, sendPid, sendListKey, sendListVal, delta); });
    std::thread recvLocalMap([&] { LocalMapPrefix(recvSet, recvPid, recvListKey, recvListVal, delta); });

    sendLocalMap.join();
    recvLocalMap.join();

    auto dummyOKVS = OKVS(n * d * prefixLen);
    // local encoding from set, totally offline

    // fmap start
    oc::Timer time;

    time.setTimePoint("begin");

    auto dummyEncoding = dummyOKVS.encode(sendListKey, sendListVal);

    auto s = time.setTimePoint("dummy OKVS done");

    auto sock = coproto::AsioSocket::makePair();
    auto sock2 = coproto::AsioSocket::makePair();

    std::vector<block> rand_R_j(n);
    std::vector<block> rand_S_j(n);

    std::vector<block> ID_R(n);
    std::vector<block> ID_S(n);

    AltModPrf RO(prng.get());
    auto key = RO.mExpandedKey;
    AltModPrf::KeyType k1 = prng.get();
    AltModPrf::KeyType k0 = k1 ^ key;

    std::thread sendSoOPPRFReverse([&] {
        SoOPPRFRecver recv(n * d * prefixLen, n * d * prefixLen, 1, false, &sock[0]);

        std::vector<block> inputs(n * d * prefixLen);
        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                auto prefixes = getPrefix(sendSet[i][j], prefixLen);
                for (int k = 0; k < prefixLen; k++) {
                    inputs[i * d * prefixLen + j * prefixLen + k] = block(j << 32, 0) ^ prefixes[k];
                }
            }
        }
        std::vector<block> rand_S(n * d * prefixLen);

        recv.OPPRF(inputs, rand_S);

        std::vector<block> u(n * d * prefixLen);
        std::vector<block> v(n * d * prefixLen);

        for (u64 i = 0; i < n * d * prefixLen; i++) {
            u[i] = rand_S[i] >> 64;
            v[i] = block(0, low(rand_S[i]));
        }

        MuxSender mux(n * d * prefixLen, &sock[0]);

        std::vector<block> t(n * d * prefixLen);

        mux.mux(u, v, t);

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                for (int k = 0; k < prefixLen; k++) {
                    rand_S_j[i] ^= t[i * d * prefixLen + j * prefixLen + k];
                }
            }
            rand_S_j[i] = rand_S_j[i] ^ sendPid[i];
        }
    });

    std::thread recvSoOPPRFReverse([&] {
        SoOPPRFSender send(n * d * prefixLen, n * d * prefixLen, 1, false, &sock[1]);

        std::vector<block> rand_R(n * d * prefixLen);

        // send.OPPRF(recvListKey, recvListVal, rand_R);
        send.OPPRF(dummyEncoding, rand_R);

        std::vector<block> u(n * d * prefixLen);
        std::vector<block> v(n * d * prefixLen);

        for (u64 i = 0; i < n * d * prefixLen; i++) {
            u[i] = rand_R[i] >> 64;
            v[i] = block(0, low(rand_R[i]));
        }

        MuxRecver mux(n * d * prefixLen, &sock[1]);

        std::vector<block> t(n * d * prefixLen);

        mux.mux(u, v, t);

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                for (int k = 0; k < prefixLen; k++) {
                    rand_R_j[i] ^= t[i * d * prefixLen + j * prefixLen + k];
                }
            }
        }
    });

    sendSoOPPRFReverse.join();
    recvSoOPPRFReverse.join();

    time.setTimePoint("soOPRF Reverse done");

    std::thread sendSiOPRFReverse([&] {
        SiOPRFRecver siRecv(n, 1, false, &sock[0], &sock2[0], k0);

        std::vector<block> share_S(n);

        siRecv.OPRF(rand_S_j, share_S);

        std::vector<block> share_R(n);

        coproto::sync_wait(sock[0].recv(share_R));

        for (int i = 0; i < n; i++) {
            ID_S[i] = share_R[i] ^ share_S[i];
        }
    });

    std::thread recvSiOPRFReverse([&] {
        SiOPRFSender siSend(n, 1, false, &sock[1], &sock2[1], k1);

        std::vector<block> share_R(n);

        siSend.OPRF(rand_R_j, share_R);

        coproto::sync_wait(sock[1].send(share_R));
    });

    sendSiOPRFReverse.join();
    recvSiOPRFReverse.join();

    time.setTimePoint("siOPRF Reverse done");

    rand_R_j = std::vector<block>(n);
    rand_S_j = std::vector<block>(n);

    std::thread sendSoOPPRF([&] {
        SoOPPRFSender send(n * d * prefixLen, n * d * prefixLen, 1, false, &sock[0]);

        std::vector<block> rand_S(n * d * prefixLen);

        // send.OPPRF(sendListKey, sendListVal, rand_S);
        send.OPPRF(dummyEncoding, rand_S);

        std::vector<block> u(n * d * prefixLen);
        std::vector<block> v(n * d * prefixLen);

        for (u64 i = 0; i < n * d * prefixLen; i++) {
            u[i] = rand_S[i] >> 64;
            v[i] = block(0, low(rand_S[i]));
        }

        MuxSender mux(n * d * prefixLen, &sock[0]);

        std::vector<block> t(n * d * prefixLen);

        mux.mux(u, v, t);

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                for (int k = 0; k < prefixLen; k++) {
                    rand_S_j[i] ^= t[i * d * prefixLen + j * prefixLen + k];
                }
            }
        }
    });

    std::thread recvSoOPPRF([&] {
        SoOPPRFRecver recv(n * d * prefixLen, n * d * prefixLen, 1, false, &sock[1]);

        std::vector<block> inputs(n * d * prefixLen);
        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                auto prefixes = getPrefix(recvSet[i][j], prefixLen);
                for (int k = 0; k < prefixLen; k++) {
                    inputs[i * d * prefixLen + j * prefixLen + k] = block(j << 32, 0) ^ prefixes[k];
                }
            }
        }
        std::vector<block> rand_R(n * d * prefixLen);

        recv.OPPRF(inputs, rand_R);

        std::vector<block> u(n * d * prefixLen);
        std::vector<block> v(n * d * prefixLen);

        for (u64 i = 0; i < n * d * prefixLen; i++) {
            u[i] = rand_R[i] >> 64;
            v[i] = block(0, low(rand_R[i]));
        }

        MuxRecver mux(n * d * prefixLen, &sock[1]);

        std::vector<block> t(n * d * prefixLen);

        mux.mux(u, v, t);

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                for (int k = 0; k < prefixLen; k++) {
                    rand_R_j[i] ^= t[i * d * prefixLen + j * prefixLen + k];
                }
            }
            rand_R_j[i] = rand_R_j[i] ^ sendPid[i];
        }
    });

    sendSoOPPRF.join();
    recvSoOPPRF.join();

    time.setTimePoint("soOPRF done");

    std::thread sendSiOPRF([&] {
        SiOPRFSender siSend(n, 1, false, &sock[0], &sock2[0], k0);

        std::vector<block> share_S(n);

        siSend.OPRF(rand_S_j, share_S);

        coproto::sync_wait(sock[0].send(share_S));
    });

    std::thread recvSiOPRF([&] {
        SiOPRFRecver siRecv(n, 1, false, &sock[1], &sock2[1], k1);

        std::vector<block> share_R(n);

        siRecv.OPRF(rand_R_j, share_R);

        std::vector<block> share_S(n);

        coproto::sync_wait(sock[1].recv(share_S));

        for (int i = 0; i < n; i++) {
            ID_R[i] = share_R[i] ^ share_S[i];
        }
    });

    sendSiOPRF.join();
    recvSiOPRF.join();

    time.setTimePoint("siOPRF done");

    rand_R_j = std::vector<block>(n);
    rand_S_j = std::vector<block>(n);

    // fmap finish
    time.setTimePoint("fmap-prefix done");
    std::cout << (sock[0].bytesReceived() + sock[0].bytesSent() + sock2[0].bytesReceived() + sock2[0].bytesSent()) / 1024.0 / 1024.0 << " MB " << std::endl;

    std::vector<u8> choiceBit(n, 0);

    std::thread sendFilter([&] {
        std::vector<block> inputs(n * d * prefixLen);

        for (int i = 0; i < n; i++) {
            for (int j = 0; j < d; j++) {
                auto prefixes = getPrefix(sendSet[i][j], prefixLen);
                for (int k = 0; k < prefixLen; k++) {
                    inputs[i * d * prefixLen + j * prefixLen + k] = (ID_S[i] << 72) ^ block(j << 4, 0) ^ prefixes[k];
                }
            }
        }

        SoOPPRFRecver recv(n * d * prefixLen, n * d * prefixLen, 1, false, &sock[0]);

        std::vector<block> rand_S(n * d * prefixLen);

        recv.OPPRF(inputs, rand_S);

        std::vector<block> u(n * d * prefixLen);
        std::vector<block> v(n * d * prefixLen);

        for (u64 i = 0; i < n * d * prefixLen; i++) {
            u[i] = rand_S[i] >> 64;
            v[i] = block(0, low(rand_S[i]));
        }

        MuxSender mux(n * d * prefixLen, &sock[0]);
        std::vector<block> t(n * d * prefixLen);

        mux.mux(u, v, t);

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                for (int k = 0; k < prefixLen; k++) {
                    rand_S_j[i] ^= t[i * d * prefixLen + j * prefixLen + k];
                }
            }
        }

        PEqTSender eqSend(n, 1, false, &sock[0]);

        eqSend.eq(rand_S_j);
    });

    std::thread recvFilter([&] {
        std::vector<block> filterKey(n * d * prefixLen);
        std::vector<block> filterVal(n * d * prefixLen);
        u64 idx = 0;

        std::vector<block> dim_rand(d);
        prng.get<block>(dim_rand.data(), dim_rand.size());

        block target = ZeroBlock;
        for (int i = 0; i < d; i++) {
            target ^= block(0, low(dim_rand[i]));
        }

        for (int i = 0; i < n; i++) {
            for (int j = 0; j < d; j++) {
                auto prefixes = getIntervalPrefix(recvSet[i][j] - delta, recvSet[i][j] + delta);
                for (auto &p : prefixes) {
                    block key = (ID_R[i] << 72) ^ block(j << 4, 0) ^ p;
                    block val = block(0, low(dim_rand[j]));
                    filterKey[idx] = key;
                    filterVal[idx] = val;
                    idx++;
                }
            }
        }

        while (filterKey.size() < n * d * prefixLen) {
            filterKey.push_back(prng.get<block>());
            filterVal.push_back(prng.get<block>());
        }

        SoOPPRFSender send(n * d * prefixLen, n * d * prefixLen, 1, false, &sock[1]);

        std::vector<block> rand_R(n * d * prefixLen);

        // send.OPPRF(filterKey, filterVal, rand_R);
        send.OPPRF(dummyEncoding, rand_R);

        std::vector<block> u(n * d * prefixLen);
        std::vector<block> v(n * d * prefixLen);

        for (u64 i = 0; i < n * d * prefixLen; i++) {
            u[i] = rand_R[i] >> 64;
            v[i] = block(0, low(rand_R[i]));
        }

        MuxRecver mux(n * d * prefixLen, &sock[1]);
        std::vector<block> t(n * d * prefixLen);

        mux.mux(u, v, t);

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                for (int k = 0; k < prefixLen; k++) {
                    rand_R_j[i] ^= t[i * d * prefixLen + j * prefixLen + k];
                }
            }
            rand_R_j[i] = rand_R_j[i] ^ target;
        }

        PEqTRecver eqRecv(n, 1, false, &sock[1]);

        std::vector<u64> intersection;

        eqRecv.eq(rand_R_j, intersection);

        for (auto &v : intersection) {
            choiceBit[v] = 1;
        }
    });

    sendFilter.join();
    recvFilter.join();

    time.setTimePoint("filter done");
    std::cout << (sock[0].bytesReceived() + sock[0].bytesSent() + sock2[0].bytesReceived() + sock2[0].bytesSent()) / 1024.0 / 1024.0 << " MB " << std::endl;

    std::thread sendOT([&] {
        SilentOtExtSender send;
        send.configure(n, 128);

        coproto::sync_wait(send.genSilentBaseOts(prng, sock[0]));

        std::vector<std::array<block, 2>> messages(n);

        coproto::sync_wait(send.send(messages, prng, sock[0]));

        std::vector<block> correctMessages(n * d / 2 * 2);
        PRNG prng0, prng1;
        for (int i = 0; i < n; i++) {
            prng0.SetSeed(messages[i][0]);
            prng1.SetSeed(messages[i][1]);
            for (int j = 0; j < d / 2; j++) {
                correctMessages[i * d + j * 2] = prng0.get<block>();
                correctMessages[i * d + j * 2 + 1] = block(sendSet[i][j * 2], sendSet[i][j * 2 + 1]) ^ prng0.get<block>();
            }
        }
        coproto::sync_wait(sock[0].send(correctMessages));
    });

    std::vector<std::vector<block>> matches;

    std::thread recvOT([&] {
        SilentOtExtReceiver recv;
        recv.configure(n, 128);

        coproto::sync_wait(recv.genSilentBaseOts(prng, sock[1]));

        std::vector<block> messages(n);
        BitVector choices(choiceBit.data(), choiceBit.size());

        coproto::sync_wait(recv.receive(choices, messages, prng, sock[1]));

        std::vector<block> correctMessages(n * d / 2 * 2);
        coproto::sync_wait(sock[1].recv(correctMessages));

        PRNG prng;
        for (int i = 0; i < n; i++) {
            if (choiceBit[i]) {
                prng.SetSeed(messages[i]);
                std::vector<block> element;
                for (int j = 0; j < d / 2; j++) {
                    block val = prng.get<block>() ^ correctMessages[i * d + j * 2 + 1];
                    element.push_back(val);
                }
                matches.push_back(element);
            }
        }
    });

    sendOT.join();
    recvOT.join();

    auto e = time.setTimePoint("OT done");

    std::cout << time << std::endl;

    std::cout << "comm: " << (sock[0].bytesReceived() + sock[0].bytesSent() + sock2[0].bytesReceived() + sock2[0].bytesSent()) / 1024.0 / 1024.0 << " MB, "
              << " time: " << std::chrono::duration_cast<std::chrono::microseconds>(e - s).count() / double(1000 * 1000) << " s" << std::endl;
}

void fuzzyPsiLpPrefix(const oc::CLP &cmd)
{
    u64 n = cmd.getOr("n", 1ull << cmd.getOr("nn", 10));
    size_t d = cmd.getOr("d", 2);
    int delta = cmd.getOr("delta", 2);
    int lp = cmd.getOr("p", 2);

    u64 delta_p = std::pow(delta, lp);
    int prefixLen = static_cast<int>(std::ceil(std::log2(delta * 2 + 1)));

    std::vector<std::vector<u64>> sendSet;
    std::vector<block> sendPid;
    std::vector<block> sendListKey;
    std::vector<block> sendListVal;

    for (u64 i = 0; i < n; i++) {
        std::vector<u64> tmp;
        for (u64 j = 0; j < d; j++) {
            tmp.push_back(i + j);
        }
        sendSet.push_back(tmp);
    }

    std::vector<std::vector<u64>> recvSet;
    std::vector<block> recvPid;
    std::vector<block> recvListKey;
    std::vector<block> recvListVal;

    for (u64 i = 0; i < n; i++) {
        std::vector<u64> tmp;
        for (u64 j = 0; j < d; j++) {
            tmp.push_back(i + j + 5);
        }
        recvSet.push_back(tmp);
    }

    std::thread sendLocalMap([&] { LocalMap(sendSet, sendPid, sendListKey, sendListVal, delta); });
    std::thread recvLocalMap([&] { LocalMap(recvSet, recvPid, recvListKey, recvListVal, delta); });

    sendLocalMap.join();
    recvLocalMap.join();

    auto dummyOKVS = OKVS(n * d * (2 * delta + 1));
    // local encoding from set, totally offline

    // fmap start
    oc::Timer time;

    time.setTimePoint("begin");

    auto dummyEncoding = dummyOKVS.encode(sendListKey, sendListVal);

    auto s = time.setTimePoint("dummy OKVS done");

    auto sock = coproto::AsioSocket::makePair();
    auto sock2 = coproto::AsioSocket::makePair();

    std::vector<block> rand_R_j(n);
    std::vector<block> rand_S_j(n);

    std::vector<block> ID_R(n);
    std::vector<block> ID_S(n);

    PRNG prng(ZeroBlock);
    AltModPrf RO(prng.get());
    auto key = RO.mExpandedKey;
    AltModPrf::KeyType k1 = prng.get();
    AltModPrf::KeyType k0 = k1 ^ key;

    std::thread sendSoOPPRFReverse([&] {
        SoOPPRFRecver recv(n * d, n * d * (2 * delta + 1), 1, false, &sock[0]);

        std::vector<block> inputs(n * d);
        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                inputs[i * d + j] = block(j, sendSet[i][j]);
            }
        }
        std::vector<block> rand_S(n * d);

        Hash(inputs);
        recv.OPPRF(inputs, rand_S);

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                rand_S_j[i] ^= rand_S[i * d + j];
            }
            rand_S_j[i] = rand_S_j[i] ^ sendPid[i];
        }
    });

    std::thread recvSoOPPRFReverse([&] {
        SoOPPRFSender send(n * d, n * d * (2 * delta + 1), 1, false, &sock[1]);

        std::vector<block> rand_R(n * d);

        // send.OPPRF(recvListKey, recvListVal, rand_R);
        send.OPPRF(dummyEncoding, rand_R);

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                rand_R_j[i] ^= rand_R[i * d + j];
            }
        }
    });

    sendSoOPPRFReverse.join();
    recvSoOPPRFReverse.join();

    time.setTimePoint("soOPRF Reverse done");

    std::thread sendSiOPRFReverse([&] {
        SiOPRFRecver siRecv(n, 1, false, &sock[0], &sock2[0], k0);

        std::vector<block> share_S(n);

        siRecv.OPRF(rand_S_j, share_S);

        std::vector<block> share_R(n);

        coproto::sync_wait(sock[0].recv(share_R));

        for (int i = 0; i < n; i++) {
            ID_S[i] = share_R[i] ^ share_S[i];
        }
    });

    std::thread recvSiOPRFReverse([&] {
        SiOPRFSender siSend(n, 1, false, &sock[1], &sock2[1], k1);

        std::vector<block> share_R(n);

        siSend.OPRF(rand_R_j, share_R);

        coproto::sync_wait(sock[1].send(share_R));
    });

    sendSiOPRFReverse.join();
    recvSiOPRFReverse.join();

    time.setTimePoint("siOPRF Reverse done");

    rand_R_j = std::vector<block>(n);
    rand_S_j = std::vector<block>(n);

    std::thread sendSoOPPRF([&] {
        SoOPPRFSender send(n * d, n * d * (2 * delta + 1), 1, false, &sock[0]);

        std::vector<block> rand_S(n * d);

        // send.OPPRF(sendListKey, sendListVal, rand_S);
        send.OPPRF(dummyEncoding, rand_S);

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                rand_S_j[i] ^= rand_S[i * d + j];
            }
        }
    });

    std::thread recvSoOPPRF([&] {
        SoOPPRFRecver recv(n * d, n * d * (2 * delta + 1), 1, false, &sock[1]);

        std::vector<block> inputs(n * d);
        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                inputs[i * d + j] = block(j, recvSet[i][j]);
            }
        }
        std::vector<block> rand_R(n * d);

        recv.OPPRF(inputs, rand_R);

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                rand_R_j[i] ^= rand_R[i * d + j];
            }
            rand_R_j[i] = rand_R_j[i] ^ recvPid[i];
        }
    });

    sendSoOPPRF.join();
    recvSoOPPRF.join();

    time.setTimePoint("soOPRF done");

    std::thread sendSiOPRF([&] {
        SiOPRFSender siSend(n, 1, false, &sock[0], &sock2[0], k0);

        std::vector<block> share_S(n);

        siSend.OPRF(rand_S_j, share_S);

        coproto::sync_wait(sock[0].send(share_S));
    });

    std::thread recvSiOPRF([&] {
        SiOPRFRecver siRecv(n, 1, false, &sock[1], &sock2[1], k1);

        std::vector<block> share_R(n);

        siRecv.OPRF(rand_R_j, share_R);

        std::vector<block> share_S(n);

        coproto::sync_wait(sock[1].recv(share_S));

        for (int i = 0; i < n; i++) {
            ID_R[i] = share_R[i] ^ share_S[i];
        }
    });

    sendSiOPRF.join();
    recvSiOPRF.join();

    time.setTimePoint("siOPRF done");

    rand_R_j = std::vector<block>(n);
    rand_S_j = std::vector<block>(n);

    // fmap finish
    time.setTimePoint("fmap done");
    std::cout << (sock[0].bytesReceived() + sock[0].bytesSent() + sock2[0].bytesReceived() + sock2[0].bytesSent()) / 1024.0 / 1024.0 << " MB " << std::endl;

    std::vector<u8> choiceBit(n, 0);

    std::vector<u64> disR(n, 0);
    std::vector<u64> disS(n, 0);

    std::vector<block> prefixR;
    std::vector<block> prefixS;

    std::thread sendFilter([&] {
        std::vector<block> inputs(n * d);

        for (int i = 0; i < n; i++) {
            for (int j = 0; j < d; j++) {
                inputs[i * d + j] = (ID_S[i] << 8) ^ block(j, sendSet[i][j]); // 8 bits for dimension
            }
        }

        SoOPPRFRecver recv(n * d, n * d * (2 * delta + 1), 1, false, &sock[0]);

        std::vector<block> rand_S(n * d);

        recv.OPPRF(inputs, rand_S);

        std::vector<u64> rand_S_A(n * d);

        oc::Timer localTime;

        auto s = localTime.setTimePoint("b2a start");

        B2aSender b2aSender(n * d, &sock[0]);
        b2aSender.b2a(rand_S, rand_S_A);

        auto e = localTime.setTimePoint("b2a done");

        auto dt = std::chrono::duration_cast<std::chrono::microseconds>(e - s).count() / double(1000);

        std::cout << "b2a time: " << dt << " ms" << std::endl;

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                disS[i] += rand_S_A[i * d + j];
            }
        }

        for (u64 i = 0; i < n; i++) {
            auto pre = getIntervalPrefix(0ULL - disS[i], delta_p - disS[i]);
            for (auto &p : pre) {
                p = p ^ block(i << 32, 0);
                prefixS.push_back(p);
            }
        }
        while (prefixS.size() != (n * prefixLen)) {
            prefixS.push_back(prng.get<block>());
        }

        PEqTSender eqSend(n * prefixLen, 1, false, &sock[0]);

        eqSend.eq(prefixS);
    });

    std::thread recvFilter([&] {
        std::vector<block> filterKey(n * d * (2 * delta + 1));
        std::vector<block> filterVal(n * d * (2 * delta + 1));
        u64 idx = 0;

        for (int i = 0; i < n; i++) {
            for (int j = 0; j < d; j++) {
                for (int t = -delta; t <= delta; t++) {
                    block key = (ID_R[i] << 8) ^ block(j, recvSet[i][j] + t);
                    block val = ZeroBlock;
                    filterKey[idx] = key;
                    filterVal[idx] = val;
                    idx++;
                }
            }
        }

        SoOPPRFSender send(n * d, n * d * (2 * delta + 1), 1, false, &sock[1]);

        std::vector<block> rand_R(n * d);

        send.OPPRF(filterKey, filterVal, rand_R);

        std::vector<u64> rand_R_A(n * d);

        B2aRecver b2aRecver(n * d, &sock[1]);
        b2aRecver.b2a(rand_R, rand_R_A);

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                disR[i] += rand_R_A[i * d + j];
            }
        }

        for (u64 i = 0; i < n; i++) {
            auto pre = getPrefix(disR[i], prefixLen);
            for (auto &p : pre) {
                p = p ^ block(i << 32, 0);
                prefixR.push_back(p);
            }
        }

        PEqTRecver eqRecv(n * prefixLen, 1, false, &sock[1]);

        std::vector<u64> intersection;

        eqRecv.eq(prefixR, intersection);

        for (auto &v : intersection) {
            choiceBit[v / prefixLen] = 1;
        }
    });

    sendFilter.join();
    recvFilter.join();

    time.setTimePoint("filter done");
    std::cout << (sock[0].bytesReceived() + sock[0].bytesSent() + sock2[0].bytesReceived() + sock2[0].bytesSent()) / 1024.0 / 1024.0 << " MB " << std::endl;

    std::thread sendOT([&] {
        SilentOtExtSender send;
        send.configure(n, 128);

        coproto::sync_wait(send.genSilentBaseOts(prng, sock[0]));

        std::vector<std::array<block, 2>> messages(n);

        coproto::sync_wait(send.send(messages, prng, sock[0]));

        std::vector<block> correctMessages(n * d / 2 * 2);
        PRNG prng0, prng1;
        for (int i = 0; i < n; i++) {
            prng0.SetSeed(messages[i][0]);
            prng1.SetSeed(messages[i][1]);
            for (int j = 0; j < d / 2; j++) {
                correctMessages[i * d + j * 2] = prng0.get<block>();
                correctMessages[i * d + j * 2 + 1] = block(sendSet[i][j * 2], sendSet[i][j * 2 + 1]) ^ prng0.get<block>();
            }
        }
        coproto::sync_wait(sock[0].send(correctMessages));
    });

    std::vector<std::vector<block>> matches;

    std::thread recvOT([&] {
        SilentOtExtReceiver recv;
        recv.configure(n, 128);

        coproto::sync_wait(recv.genSilentBaseOts(prng, sock[1]));

        std::vector<block> messages(n);
        BitVector choices(choiceBit.data(), choiceBit.size());

        coproto::sync_wait(recv.receive(choices, messages, prng, sock[1]));

        std::vector<block> correctMessages(n * d / 2 * 2);
        coproto::sync_wait(sock[1].recv(correctMessages));

        PRNG prng;
        for (int i = 0; i < n; i++) {
            if (choiceBit[i]) {
                prng.SetSeed(messages[i]);
                std::vector<block> element;
                for (int j = 0; j < d / 2; j++) {
                    block val = prng.get<block>() ^ correctMessages[i * d + j * 2 + 1];
                    element.push_back(val);
                }
                matches.push_back(element);
            }
        }
    });

    sendOT.join();
    recvOT.join();

    auto e = time.setTimePoint("OT done");

    std::cout << time << std::endl;

    std::cout << "comm: " << (sock[0].bytesReceived() + sock[0].bytesSent() + sock2[0].bytesReceived() + sock2[0].bytesSent()) / 1024.0 / 1024.0 << " MB, "
              << " time: " << std::chrono::duration_cast<std::chrono::microseconds>(e - s).count() / double(1000 * 1000) << " s" << std::endl;
}
