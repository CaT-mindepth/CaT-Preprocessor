struct Packet {
  int pkt_0;
};
int state_0 = 0;
void func(struct Packet p) {
  if (!(!(!(!(state_0 == 29 && 1 == 1)) && 1 == 1))) {
    if (1 == 1 && !(!(state_0 == 29 && 1 == 1))) {
      if (1 == 1 && 1 == 1 && state_0 == 29) {
        p.pkt_0 = 1;
        state_0 = 0;
      }
    }
  } else {
    if (!(!(!(state_0 == 29 && 1 == 1)) && 1 == 1)) {
      if (1 == 1 && !(state_0 == 29 && 1 == 1)) {
        p.pkt_0 = 0;
        state_0 = state_0 + 1;
      }
    }
  }
}
