 int st0;int st1;struct Packet{
int a;int b;int c;int d;int tmp0;int st00;int st10;int st000;int st100;int a0;int tmp00;int st001;int st101;};void func( struct Packet p) {p.st000 = st0;p.st100 = st1;p.a0 = ((p.b+p.c));p.tmp00 = ((p.a0>p.st000+3));p.st001 = ((p.tmp00) ? (p.c+p.d) : p.st000);p.st101 = ((p.tmp00) ? (p.st001+p.d) : p.st100);st0 = p.st001;st1 = p.st101;}

