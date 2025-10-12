#include "fmap.h"
#include <map>
#include "Defines.h"

// to be fixed
void LocalMap(std::vector<std::vector<u64>> &inputs, std::vector<block> &pid, std::vector<block> &listKey, std::vector<block> &listVal, int delta)
{
    PRNG prng(ZeroBlock);

    u64 m = inputs.size();
    u64 d = inputs[0].size();

    pid.resize(m);

    for (u64 i = 0; i < d; i++) {
        std::map<block, block> mp_d;
        for (u64 j = 0; j < m; j++) {
            for (int t = -delta; t <= delta; t++) {
                block key = block(i, inputs[j][i] + t);
                block val = prng.get<block>();
                mp_d[key] = val;
                if (t == 0) {
                    pid[j] ^= val;
                }
            }
        }
        for (auto &kv : mp_d) {
            listKey.push_back(kv.first);
            listVal.push_back(kv.second);
        }
    }
    while (listKey.size() < (2 * delta + 1) * d * m) {
        listKey.push_back(prng.get<block>());
        listVal.push_back(prng.get<block>());
    }
}