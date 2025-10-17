#include "b2a.h"
#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Common/block.h>
#include <vector>

using namespace oc;

B2aSender::B2aSender(uint64_t num_, coproto::Socket *socket_) : num(num_), socket(socket_)
{
    num *= 64;

    sender = new osuCrypto::SilentOtExtSender();
    sender->configure(num, 128);

    prng = new PRNG(ZeroBlock);

    coproto::sync_wait(sender->genSilentBaseOts(*prng, *socket));
}

B2aSender::~B2aSender()
{
    delete sender;
    delete prng;
}

void B2aSender::b2a(std::vector<block> &blk, std::vector<u64> &val)
{
    std::vector<std::array<block, 2>> messages(num);

    BitVector choiceBit(reinterpret_cast<u8 *>(blk.data()), blk.size() * 128);

    coproto::sync_wait(sender->send(messages, *prng, *socket));

    std::vector<block> correctMessages(num * 2);
    for (int i = 0; i < num; i++) {
        u64 r = prng->get<u64>();
        u8 bit = choiceBit.data()[(i / 64) * 128 + (i % 64)];
        correctMessages[i * 2] = messages[i][0] ^ block(0, r);
        correctMessages[i * 2 + 1] = messages[i][1] ^ block(0, r + bit);
        val[i / 64] = val[i / 64] | (u64(bit) << (i % 64));
    }

    coproto::sync_wait(socket->send(correctMessages));
}

B2aRecver::B2aRecver(uint64_t num_, coproto::Socket *socket_) : num(num_), socket(socket_)
{
    num *= 64;

    receiver = new osuCrypto::SilentOtExtReceiver();
    receiver->configure(num, 128);

    prng = new PRNG(OneBlock);

    coproto::sync_wait(receiver->genSilentBaseOts(*prng, *socket));
}

B2aRecver::~B2aRecver()
{
    delete receiver;
    delete prng;
}

void B2aRecver::b2a(std::vector<block> &blk, std::vector<u64> &val)
{
    std::vector<block> messages(num);
    BitVector choiceBit(reinterpret_cast<u8 *>(blk.data()), blk.size() * 128);

    coproto::sync_wait(receiver->receive(choiceBit, messages, *prng, *socket));

    std::vector<block> correctMessages(num * 2);
    coproto::sync_wait(socket->recv(correctMessages));

    // for (int i = 0; i < num; i++) {
    //     u64 r = prng->get<u64>();
    //     u8 bit = choiceBit.data()[(i / 64) * 128 + (i % 64)];
    //     block val = messages[i] ^ block(0, r + bit);
    //     val[i / 64] = val[i / 64] | (u64(bit) << (i % 64));
    // }
}