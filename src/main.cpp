#include "cryptoTools/Common/CLP.h"
#include "fmap.h"
#include "fmap_prefix.h"

int main(int argc, char **argv)
{
    oc::CLP cmd(argc, argv);

    int lp = cmd.getOr("p", 0);

    if (lp != 0) {
        if (cmd.isSet("prefix")) {
            fuzzyPsiLpPrefix(cmd);
        } else {
            fuzzyPsiLp(cmd);
        }
    } else {
        if (cmd.isSet("prefix")) {
            fuzzyPsiPrefix(cmd);
        } else {
            fuzzyPsi(cmd);
        }
    }

    return 0;
}
