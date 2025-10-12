#pragma once

#include <array>
#include <coproto/Socket/AsioSocket.h>
#include <coproto/Socket/LocalAsyncSock.h>
#include <cryptoTools/Common/block.h>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <libOTe/TwoChooseOne/Silent/SilentOtExtSender.h>
#include <macoro/sync_wait.h>
#include <secure-join/Defines.h>
#include <thread>
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

void LocalMap(std::vector<std::vector<u64>> &inputs, std::vector<block> &pid, std::vector<block> &listKey, std::vector<block> &listVal, int delta);

inline void Fmap(const oc::CLP &cmd)
{
    u64 n = cmd.getOr("n", 1ull << cmd.getOr("nn", 10));
    size_t d = cmd.getOr("d", 2);
    int delta = cmd.getOr("delta", 2);

    oc::Timer time;

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

    // fmap start

    time.setTimePoint("begin");

    auto sock = coproto::LocalAsyncSocket::makePair();
    auto sock2 = coproto::LocalAsyncSocket::makePair();

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

        recv.OPPRF(inputs, rand_S);

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                rand_S_j[i] ^= rand_S[i * d + j];
            }
            rand_S_j[i] = rand_S_j[i] ^ sendPid[i];
        }

        SiOPRFRecver siRecv(n, 1, false, &sock[0], &sock2[0], k0);

        std::vector<block> share_S(n);

        siRecv.OPRF(rand_S_j, share_S);

        std::vector<block> share_R(n);

        coproto::sync_wait(sock[0].recv(share_R));

        for (int i = 0; i < n; i++) {
            ID_S[i] = share_R[i] ^ share_S[i];
        }
    });

    std::thread recvSoOPPRFReverse([&] {
        SoOPPRFSender send(n * d, n * d * (2 * delta + 1), 1, false, &sock[1]);

        std::vector<block> rand_R(n * d);

        send.OPPRF(recvListKey, recvListVal, rand_R);

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                rand_R_j[i] ^= rand_R[i * d + j];
            }
        }

        SiOPRFSender siSend(n, 1, false, &sock[1], &sock2[1], k1);

        std::vector<block> share_R(n);

        siSend.OPRF(rand_R_j, share_R);

        coproto::sync_wait(sock[1].send(share_R));
    });

    sendSoOPPRFReverse.join();
    recvSoOPPRFReverse.join();

    rand_R_j = std::vector<block>(n);
    rand_S_j = std::vector<block>(n);

    std::thread sendSoOPPRF([&] {
        SoOPPRFSender send(n * d, n * d * (2 * delta + 1), 1, false, &sock[0]);

        std::vector<block> rand_S(n * d);

        send.OPPRF(sendListKey, sendListVal, rand_S);

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                rand_S_j[i] ^= rand_S[i * d + j];
            }
        }

        SiOPRFSender siSend(n, 1, false, &sock[0], &sock2[0], k0);

        std::vector<block> share_S(n);

        siSend.OPRF(rand_S_j, share_S);

        coproto::sync_wait(sock[0].send(share_S));
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

        SiOPRFRecver siRecv(n, 1, false, &sock[1], &sock2[1], k1);

        std::vector<block> share_R(n);

        siRecv.OPRF(rand_R_j, share_R);

        std::vector<block> share_S(n);

        coproto::sync_wait(sock[1].recv(share_S));

        for (int i = 0; i < n; i++) {
            ID_R[i] = share_R[i] ^ share_S[i];
        }
    });

    sendSoOPPRF.join();
    recvSoOPPRF.join();

    rand_R_j = std::vector<block>(n);
    rand_S_j = std::vector<block>(n);

    // fmap finish
    time.setTimePoint("fmap done");
    std::cout << (sock[0].bytesReceived() + sock[0].bytesSent() + sock2[0].bytesReceived() + sock2[0].bytesSent()) / 1024.0 / 1024.0 << " MB " << std::endl;

    std::vector<u8> choiceBit(n, 0);

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

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                rand_S_j[i] ^= rand_S[i * d + j];
            }
        }

        PEqTSender eqSend(n, 1, false, &sock[0]);

        eqSend.eq(rand_S_j);
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

        for (u64 i = 0; i < n; i++) {
            for (u64 j = 0; j < d; j++) {
                rand_R_j[i] ^= rand_R[i * d + j];
            }
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

    time.setTimePoint("OT done");

    std::cout << time << std::endl;

    std::cout << (sock[0].bytesReceived() + sock[0].bytesSent() + sock2[0].bytesReceived() + sock2[0].bytesSent()) / 1024.0 / 1024.0 << " MB " << std::endl;
}
