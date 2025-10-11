#include "fmap.h"
#include <cstddef>
#include <map>
#include "Defines.h"

// to be fixed
void LocalMap(std::vector<std::vector<u64>> &inputs, std::vector<block> &pid, std::vector<block> &list_key, std::vector<block> &list_val, int delta)
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
            list_key.push_back(kv.first);
            list_val.push_back(kv.second);
        }
    }
    while (list_key.size() < (2 * delta + 1) * d * m) {
        list_key.push_back(prng.get<block>());
        list_val.push_back(prng.get<block>());
    }
}