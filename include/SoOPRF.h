#pragma once

// so-OPRF with input \mathbb{F}_3 and output \mathbb{F}_2
class SoOPRFSender {
public:
    SoOPRFSender();
    ~SoOPRFSender();

    void setup();
    void OPRF();
};

class SoOPRFReceiver {
public:
    SoOPRFReceiver();
    ~SoOPRFReceiver();

    void setup();
    void OPRF();
};
