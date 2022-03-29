struct Packet {
  int a;
  int b;
  int c;
};
int st0;

void func(struct Packet p) {
  if (p.a != p.b && p.a != p.b && p.a != p.b && p.a != p.b) {
    st0 = p.c;
  }
}
