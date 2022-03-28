struct Packet {
  int a;
  int b;
  int c;
  int d;
};

int st0;
int st1;

void func(struct Packet p){
  p.a = p.b + p.c;
  if (p.a > st0 + 3) {
    st0 = p.c + p.d;
    st1 = st0 + p.d;
  }
}

// optimized program:
// p.a = p.b + 10;
// p.d = p.b + p.a;
