int state_0 = {100};int state_1 = {-1};struct Packet{
int pkt_3;int pkt_4;int pkt_0;int state_000;int state_100;int _br_tmp00;int pkt_10;int _br_tmp10;int pkt_20;int _br_tmp20;int state_001;int _br_tmp30;int state_002;int _br_tmp40;int state_101;int tmp1;int tmp0;};void func( struct Packet p) {p.state_000 = state_0;p.state_100 = state_1;p._br_tmp00 = p.pkt_0<0;p.pkt_10 = (p._br_tmp00) ? (0) : (p.pkt_0);p._br_tmp10 = p.pkt_0<0;p.pkt_20 = (p._br_tmp10) ? (0) : (p.pkt_0);p._br_tmp20 = p.tmp0_br_&&p.tmp1_br_;p.state_001 = (p._br_tmp20) ? (p.pkt_3) : (p.state_000);p._br_tmp30 = p.pkt_3<p.state_001;p.state_002 = (p._br_tmp30) ? (p.pkt_3) : (p.state_001);p._br_tmp40 = p.pkt_3<p.state_002;p.state_101 = (p._br_tmp40) ? (p.pkt_4) : (p.state_100);state_0 = p.state_002;state_1 = p.state_101;}