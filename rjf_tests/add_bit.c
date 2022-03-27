struct Packet {
  int a;
  int b;
  int c;
  int d;
  int e;
};
void func(struct Packet p){
  p.a = 1 + 2 + 3 + 4 + p.b;
  p.a = p.b + 4 + 2 + 3 + 1;
  p.d = p.b + p.a + p.c; // D = B + A + C
  p.d = p.c + p.b + p.a; // D = C + B + A
  p.d = p.a + p.b;
  p.d = p.b + p.a;
}
