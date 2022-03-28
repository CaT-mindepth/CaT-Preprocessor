struct Packet {
  int b;
  int c;
  int a0;
  int a1;
  int d0;
  int d1;
  int d2;
  int d3;
};
void func(struct Packet p) {
  p.a1 = 10 + p.b;
  p.d3 = p.a1 + p.b;
}
