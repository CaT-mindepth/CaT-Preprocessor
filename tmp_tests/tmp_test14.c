int seen;
int last_ttl;
int ttl_change;
struct Packet {
  int sport;
  int rdata;
  int ttl;
  int id;
};
void func(struct Packet p) {
  p.id = p.rdata;
  if (!(!(!(1 == 1 && seen == 0)) && 1 == 1 && 1 == 1)) {
    {
      if (!(1 == 1 && seen == 0) && 1 == 1 && 1 == 1) {
        {
          if (1 == 1 && last_ttl != p.ttl && 1 == 1 && 1 == 1) {
            {
              last_ttl = p.ttl;
              ttl_change = 1;
            }
          }
        }
      }
    }
  } else {
    {
      !(!(1 == 1 && seen == 0)) && 1 == 1 = 1 == 1;
      {
        if (1 == 1 && seen[p.id] == 0 && 1 == 1 && 1 == 1) {
          seen[p.id] = 1;
          last_ttl[p.id] = p.ttl;
          ttl_change[p.id] = 0;
          ;
        };
      };
    }
  }
}
