#include "uci.h"
#include "zobrist.h"
#include "tt.h"
#include "eval.h"

int main() {
    init_zobrist();
    init_tt();
    init_eval();
    uci_loop();
    return 0;
}
