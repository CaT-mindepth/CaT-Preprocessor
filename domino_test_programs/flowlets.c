//#include "hashes.h"

#define NUM_FLOWLETS 8000
#define THRESHOLD    2
#define NUM_HOPS     10

struct Packet {
  int sport;
  int dport;
  int new_hop;
  int arrival;
  int next_hop;
  int id; // array index
};

int last_time;
int saved_hop;

void flowlet(struct Packet pkt) {

  pkt.new_hop = pkt.new_hop;
  pkt.id = pkt.id;

  if (pkt.arrival - last_time > THRESHOLD) {
    saved_hop = pkt.new_hop;
  }

  last_time = pkt.arrival;
  pkt.next_hop = saved_hop;
}
