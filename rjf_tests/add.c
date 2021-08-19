struct Packet {
  int a;
  int b;
  int c;
  int d;
};
void func(struct Packet p){
//  p.a = 1 + 2 + 3 + 4 + p.b;
//  p.a = p.b + 4 + 2 + 3 + 1;
  p.d = p.b + p.a + p.c;
  p.d = p.c + p.b + p.a;
//  p.d = p.a + p.b;
//  p.d = p.b + p.a;
}
