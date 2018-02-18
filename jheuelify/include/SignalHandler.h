#ifndef __SIGNALHANDLER_H__
#define __SIGNALHANDLER_H__

class SignalHandler {
public:
    SignalHandler();

    static void exit(int _i);

    bool running();

private:
    static bool got_sig_int;
};

#endif
