struct Packet {
  int util;
  int path_id;
  int src;
  int best_path_util_idx;
  int best_path_idx;
};

int best_path_util;
int best_path;

void func(struct Packet p) {
  p.best_path_util_idx = p.best_path_util_idx;
  p.best_path_idx      = p.best_path_idx;
  if (p.util < best_path_util) {
    best_path_util = p.util;
    best_path = p.path_id;
  } else if (p.path_id == best_path) {
    best_path_util = p.util;
  }
}
