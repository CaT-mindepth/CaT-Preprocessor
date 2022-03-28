int state_0 = {100};
int state_1 = {-1};
struct Packet {
  int pkt_3;
  int pkt_4;
  int pkt_0;
  int pkt_1;
  int pkt_2;
  int _br_tmp0;
  int _br_tmp1;
  int _br_tmp2;
  int _br_tmp3;
  int _br_tmp4;
  int _br_tmp5;
  int _br_tmp6;
  int _br_tmp7;
  int _br_tmp8;
  int state_00;
  int state_10;
  int state_000;
  int state_100;
  int pkt_10;
  int pkt_20;
  int _br_tmp00;
  int _br_tmp10;
  int _br_tmp20;
  int _br_tmp30;
  int _br_tmp40;
  int state_001;
  int _br_tmp50;
  int _br_tmp60;
  int _br_tmp70;
  int _br_tmp80;
  int state_002;
  int state_101;
  int _br_tmp9;
  int _br_tmp11;
  int _br_tmp12;
};
void func(struct Packet p) {
  p.state_000 = state_0;
  p.state_100 = state_1;
  p._br_tmp9 = p.pkt_0 < 0;
  p.pkt_10 = p._br_tmp9 ? (0) : (p.pkt_0);
  p.pkt_20 = p._br_tmp9 ? (0) : (p.pkt_0);
  p._br_tmp11 = !(p.pkt_3 < p.state_000) && p.pkt_4 == p.state_100;
  p.state_001 = p._br_tmp11 ? (p.pkt_3) : (p.state_000);
  p._br_tmp12 = (((((p.pkt_3 < p.state_000)) && p.pkt_3 < p.state_001) &&
                  p.pkt_3 < p.state_001) &&
                 p.pkt_3 < p.state_001) &&
                p.pkt_3 < p.state_001;
  p.state_002 = p._br_tmp12 ? (p.pkt_3) : (p.state_001);
  p.state_101 = p._br_tmp12 ? (p.pkt_4) : (p.state_100);
  state_0 = p.state_002;
  state_1 = p.state_101;
}
