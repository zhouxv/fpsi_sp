#include "mul.h"
#include "utils.h"

using namespace osuCrypto;

MulSender::MulSender(uint64_t num_, coproto::Socket *socket_) : num(num_), socket(socket_)
{
    sender = new osuCrypto::SilentOtExtSender();
    sender->configure(num * 64);

    prng = new PRNG(ZeroBlock);
}

MulSender::~MulSender()
{
    delete sender;
    delete prng;
}

void MulSender::mul(std::vector<uint64_t> &inputs, std::vector<uint64_t> &val)
{
    coproto::sync_wait(sender->genSilentBaseOts(*prng, *socket));

    u64 numOts = num * 64;
    std::vector<std::array<block, 2>> messages(numOts);

    coproto::sync_wait(sender->send(messages, *prng, *socket));

    std::vector<u64> correctMessages(numOts);

    for (int i = 0; i < numOts; i++) {
        u64 mask = low(messages[i][0]) + inputs[i / 64];
        correctMessages[i] = low(messages[i][1]) ^ mask;
    }

    for (int i = 0; i < numOts; i++) {
        val[i / 64] += 0 - (low(messages[i][0]) << (i % 64));
    }

    coproto::sync_wait(socket->send(correctMessages));
}

MulRecver::MulRecver(uint64_t num_, coproto::Socket *socket_) : num(num_), socket(socket_)
{
    receiver = new osuCrypto::SilentOtExtReceiver();
    receiver->configure(num * 64);

    prng = new PRNG(OneBlock);
}

MulRecver::~MulRecver()
{
    delete receiver;
    delete prng;
}

void MulRecver::mul(std::vector<uint64_t> &inputs, std::vector<uint64_t> &val)
{
    coproto::sync_wait(receiver->genSilentBaseOts(*prng, *socket));

    u64 numOts = num * 64;
    std::vector<u8> bytes(numOts / 8);

    for (int i = 0; i < num; i++) {
        u64 lowbits = inputs[i];
        for (int j = 0; j < 8; j++) {
            bytes[i * 8 + j] = (lowbits >> (8 * j)) & 0xFF;
        }
    }

    std::vector<block> messages(numOts);

    BitVector choiceBit(bytes.data(), numOts);

    coproto::sync_wait(receiver->receive(choiceBit, messages, *prng, *socket));

    std::vector<u64> correctMessages(numOts);

    coproto::sync_wait(socket->recv(correctMessages));

    for (int i = 0; i < numOts; i++) {
        if (choiceBit[i] & 1) {
            val[i / 64] += (low(messages[i]) ^ correctMessages[i]) << (i % 64);
        } else {
            val[i / 64] += low(messages[i]) << (i % 64);
        }
    }
}