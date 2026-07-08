#include "uci.h"
#include "zobrist.h"
#include "tt.h"

int main() {
    init_zobrist();
    init_tt();
    uci_loop();
    return 0;
}
