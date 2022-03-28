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
  int st00;
  int st10;
  int st000;
  int st100;
  int a0;
  int _br_tmp00;
  int _br_tmp10;
  int _br_tmp20;
  int st001;
  int st101;
  int _br_tmp3;
};
void func(struct Packet p) {
  p.st000 = st0;
  p.st100 = st1;
  p.a0 = p.b + p.c;
  p._br_tmp00 = p.a0 > 3 + p.st000;
  p._br_tmp10 = p.a0 > 3 + p.st000;
  p._br_tmp20 = 1;
  p._br_tmp3 = p.a0 > 3 + p.st000;
  p.st001 = p._br_tmp3 ? (p.c + p.d) : (p.st000);
  p.st101 = p._br_tmp3 ? (p.d + p.st001) : (p.st100);
  st0 = p.st001;
  st1 = p.st101;
}
