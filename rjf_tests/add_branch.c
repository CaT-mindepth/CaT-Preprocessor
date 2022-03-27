struct Packet {
  int a;
  int b;
  int c;
  int d;
};

int st0;
int st1;

void func(struct Packet p){
  p.a = 1 + 2 + 3 + 4 + p.b;
  if (p.a ==1 && p.a==1 &&p.a==1) {
  p.a = p.b + 4 + 2 + 3 + 1; // one of them can be coalesced
  }
  p.d = p.b + p.a + p.c; // D = B + A + C
  p.d = p.c + p.b + p.a; // D = C + B + A // one of them can be coalesced
  p.d = p.a + p.b;
  p.d = p.b + p.a;
}

// optimized program:
// p.a = p.b + 10;
// p.d = p.b + p.a;
