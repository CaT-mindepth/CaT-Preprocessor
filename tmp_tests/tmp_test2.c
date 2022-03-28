int st0;
int st1;
struct Packet {
  int a;
  int b;
  int c;
  int d;
  int _br_tmp0;
};
void func(struct Packet p) {
  p.a = p.b + p.c;
  p._br_tmp0 = p.a > st0 + 3;
  st0_1 = ((p._br_tmp0) ? (p.c + p.d) : st0);
  p._br_tmp1 = p.a > st0 + 3;
  st1 = ((p._br_tmp1) ? (st0 + p.d) : st1);
}
