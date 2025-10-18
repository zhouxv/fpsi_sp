#include "b2a.h"
#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Common/block.h>
#include <cstring>
#include <vector>
#include "utils.h"

using namespace oc;

B2aSender::B2aSender(uint64_t num_, coproto::Socket *socket_) : num(num_), socket(socket_)
{
    sender = new osuCrypto::SilentOtExtSender();
    sender->configure(num * 64);

    prng = new PRNG(ZeroBlock);
}

B2aSender::~B2aSender()
{
    delete sender;
    delete prng;
}

void B2aSender::b2a(std::vector<block> &blk, std::vector<u64> &val)
{
    coproto::sync_wait(sender->genSilentBaseOts(*prng, *socket));

    u64 numOts = num * 64;
    std::vector<std::array<block, 2>> messages(numOts);

    std::vector<u8> bits(numOts);

    for (int i = 0; i < num; i++) {
        u64 lowbits = low(blk[i]);
        for (int j = 0; j < 64; j++) {
            bits[i * 64 + j] = (lowbits >> j) & 1;
        }
    }

    coproto::sync_wait(sender->send(messages, *prng, *socket));

    std::vector<u64> correctMessages(numOts);

    for (int i = 0; i < numOts; i++) {
        u8 bit = bits[i];
        u64 mask = low(messages[i][0]) + u64(bit);
        correctMessages[i] = low(messages[i][1]) ^ mask;
    }

    for (int i = 0; i < numOts; i++) {
        u8 bit = bits[i];
        val[i / 64] += (u64(bit) + 2 * low(messages[i][0])) << (i % 64);
    }

    coproto::sync_wait(socket->send(correctMessages));
}

B2aRecver::B2aRecver(uint64_t num_, coproto::Socket *socket_) : num(num_), socket(socket_)
{
    receiver = new osuCrypto::SilentOtExtReceiver();
    receiver->configure(num * 64);

    prng = new PRNG(OneBlock);
}

B2aRecver::~B2aRecver()
{
    delete receiver;
    delete prng;
}

void B2aRecver::b2a(std::vector<block> &blk, std::vector<u64> &val)
{
    coproto::sync_wait(receiver->genSilentBaseOts(*prng, *socket));

    u64 numOts = num * 64;
    std::vector<block> messages(numOts);

    std::vector<u8> bytes(numOts / 8);

    for (int i = 0; i < num; i++) {
        u64 lowbits = low(blk[i]);
        for (int j = 0; j < 8; j++) {
            bytes[i * 8 + j] = (lowbits >> (8 * j)) & 0xFF;
        }
    }

    BitVector choiceBit(bytes.data(), numOts);

    coproto::sync_wait(receiver->receive(choiceBit, messages, *prng, *socket));

    std::vector<u64> correctMessages(numOts);

    coproto::sync_wait(socket->recv(correctMessages));

    for (int i = 0; i < numOts; i++) {
        if (choiceBit[i] & 1) {
            val[i / 64] += (u64(choiceBit[i]) - 2 * (low(messages[i]) ^ correctMessages[i])) << (i % 64);
        } else {
            val[i / 64] += (u64(choiceBit[i]) - 2 * low(messages[i])) << (i % 64);
        }
    }
}