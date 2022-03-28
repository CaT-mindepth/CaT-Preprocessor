struct Packet {
  int a;
};

int st0;

void func(struct Packet p) {
  st0 += p.a + 3;
  st0 += 1;
}
