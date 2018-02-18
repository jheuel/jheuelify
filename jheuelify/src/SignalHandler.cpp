#include <signal.h>

#include "SignalHandler.h"

bool SignalHandler::got_sig_int = false;

SignalHandler::SignalHandler() { 
    signal(SIGINT, SignalHandler::exit);
}

bool SignalHandler::running() {
    return !got_sig_int;
}

void SignalHandler::exit(int _i) {
    got_sig_int = true;
}

