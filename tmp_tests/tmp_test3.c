int st0;
int st1;
struct Packet {
  int a;
  int b;
  int c;
  int d;
  int _br_tmp0;
  int _br_tmp1;
  int _br_tmp2;
};
void func(struct Packet p) {
  p.a = (1 ? (p.b + p.c) : p.a);
  p._br_tmp0 = p.a > st0 + 3;
  p._br_tmp1 = p.a > st0 + 3;
  p._br_tmp2 = 1 == 1;
  st0 = ((((1 && p._br_tmp0) && p._br_tmp1) && p._br_tmp2) ? (p.c + p.d) : st0);
  st1 = ((((1 && p._br_tmp0) && p._br_tmp1) && p._br_tmp2) ? (st0 + p.d) : st1);
}
