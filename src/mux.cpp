#include "mux.h"
#include <coproto/Socket/Socket.h>
#include <coproto/coproto.h>
#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Common/CLP.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Common/block.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <sys/types.h>
#include <vector>
#include <volePSI/Defines.h>
#include <volePSI/GMW/Circuit.h>
#include <volePSI/GMW/Gmw.h>
#include <volePSI/Paxos.h>
#include <volePSI/config.h>

void ssPEQT(u32 idx, std::vector<block> &input, BitVector &out, Socket &chl, u32 numThreads)
{
    u32 numBins = input.size();
    u64 keyBitLength = 40 + oc::log2ceil(numBins);
    u64 keyByteLength = oc::divCeil(keyBitLength, 8);
    PRNG prng(sysRandomSeed());

    oc::Matrix<u8> mLabel(numBins, keyByteLength);
    for (u32 i = 0; i < numBins; ++i) {
        memcpy(&mLabel(i, 0), &input[i], keyByteLength);
    }

    // call gmw
    auto cir = volePSI::isZeroCircuit(keyBitLength);

    // volePSI::BetaCircuit cir = volePSI::isZeroCircuit(keyBitLength);
    volePSI::Gmw cmp;
    cmp.init(mLabel.rows(), cir, numThreads, idx, prng.get());

    if (idx == 1) {
        cmp.setInput(0, mLabel);
    } else {
        cmp.implSetInput(0, mLabel, mLabel.cols());
    }

    coproto::sync_wait(cmp.run(chl));

    oc::Matrix<u8> mOut;
    mOut.resize(numBins, 1);
    cmp.getOutput(0, mOut);

    // get the final output
    out.resize(numBins);
    for (u32 i = 0; i < numBins; ++i) {
        out[i] = mOut(i, 0) & 1;
    }
    return;
}

MuxSender::MuxSender(uint64_t num_, coproto::Socket *socket_) : num(num_), socket(socket_)
{
    sender = new osuCrypto::SilentOtExtSender();
    sender->configure(num);

    recver = new osuCrypto::SilentOtExtReceiver();
    recver->configure(num);

    prng = new PRNG(ZeroBlock);
}

MuxSender::~MuxSender()
{
    delete sender;
    delete prng;
}

void MuxSender::mux(std::vector<block> &u0, std::vector<block> &v0, std::vector<block> &res0)
{
    BitVector b0(num);
    ssPEQT(1, u0, b0, *socket, 1);

    coproto::sync_wait(sender->genSilentBaseOts(*prng, *socket));
    coproto::sync_wait(recver->genSilentBaseOts(*prng, *socket));

    std::vector<std::array<block, 2>> messages(num);
    coproto::sync_wait(sender->send(messages, *prng, *socket));

    std::vector<block> correctMessages(num);

    for (u64 i = 0; i < num; i++) {
        correctMessages[i] = messages[i][0] ^ messages[i][1] ^ v0[i];
    }

    coproto::sync_wait(socket->send(correctMessages));

    coproto::sync_wait(recver->receive(b0, res0, *prng, *socket));

    std::vector<block> correctMessages1(num);

    coproto::sync_wait(socket->recv(correctMessages1));

    for (u64 i = 0; i < num; i++) {
        res0[i] = res0[i] ^ (b0[i] ? correctMessages1[i] : block(0, 0));
        res0[i] = res0[i] ^ messages[i][0];
        res0[i] = res0[i] ^ (b0[i] ? v0[i] : block(0, 0));
    }
}

MuxRecver::MuxRecver(uint64_t num_, coproto::Socket *socket_) : num(num_), socket(socket_)
{
    sender = new osuCrypto::SilentOtExtSender();
    sender->configure(num);

    recver = new osuCrypto::SilentOtExtReceiver();
    recver->configure(num);

    prng = new PRNG(OneBlock);
}

MuxRecver::~MuxRecver()
{
    delete recver;
    delete prng;
}

void MuxRecver::mux(std::vector<block> &u1, std::vector<block> &v1, std::vector<block> &res1)
{
    BitVector b1(num);
    ssPEQT(0, u1, b1, *socket, 1);

    coproto::sync_wait(recver->genSilentBaseOts(*prng, *socket));
    coproto::sync_wait(sender->genSilentBaseOts(*prng, *socket));

    coproto::sync_wait(recver->receive(b1, res1, *prng, *socket));

    std::vector<block> correctMessages1(num);

    coproto::sync_wait(socket->recv(correctMessages1));

    std::vector<std::array<block, 2>> messages(num);
    coproto::sync_wait(sender->send(messages, *prng, *socket));

    std::vector<block> correctMessages(num);

    for (u64 i = 0; i < num; i++) {
        correctMessages[i] = messages[i][0] ^ messages[i][1] ^ v1[i];
    }

    coproto::sync_wait(socket->send(correctMessages));

    for (u64 i = 0; i < num; i++) {
        res1[i] = res1[i] ^ (b1[i] ? correctMessages1[i] : block(0, 0));
        res1[i] = res1[i] ^ messages[i][0];
        res1[i] = res1[i] ^ (b1[i] ? v1[i] : block(0, 0));
    }
}