#ifndef ITE_SSA_H_
#define ITE_SSA_H_

#include <string>
#include <utility>
#include <set>

#include "clang/AST/Decl.h"

// ITE-SSA: SSA while doing if-conversion. Algorithm works as follows [1].



// Static Single-Assignment form for function body, excluding the final write
// in the write epilogue to state variables. This guarantees that each packet
// variable is assigned exactly once. If it is assigned more than once, perform
// simple renaming. SSA is very simple in domino because we have no branches and no phi nodes.
std::string ite_ssa_transform(const clang::TranslationUnitDecl * tu_decl);

/// Helper function that does most of the heavy lifting in SSA, by rewriting the function body into SSA form.
std::pair<std::string, std::vector<std::string>> ite_ssa_rewrite_fn_body(const clang::CompoundStmt * function_body, const std::string & pkt_name, const std::set<std::string> & id_set);

/* [1]: ITE-SSA procedure.
/*
For straight-line code not inside any ITE blocks:
  For each variable x, VN(x) -> the last used SSA index of variable x in the preceeding 
  lines, and assign lhs = VN(lhs) + 1. This way, DCE can simply use a hash map keyed on
  each line to identify any dead code.

If-conversion:
For then-blocks and else-blocks, recurse with the VN map and if-condition p. For
all LHSes in the then-block and else-block, SSA them as usual using the VN from previous
lines. After recursively SSAing the then-block and the else-block, we end up with two
sets of SSA'd statements and two VN maps. We also record the modified LHSes either in the
then-block or the else-block.

  After handling then and else-blocks, we merge their SSA'd statements with essentially
a round of DCE. For each statement lhs_x = ... where x is a SSA number and (lhs_x = ...) 
  only appears in either a then-block or an else-block but not both, we rename the statement
  to lhs_if_x = ... . For each statement L such that L apears in both blocks, we only include
  L once in the final processed code. We process this renaming scheme for then-blocks first
  and else-blocks next.

Example:
```
if (p) {
  a1 = b1 + c1; // this statement appears in both if-block and then-block
  a2 = a1 + 99;
  c2 = 3 + 8; // this one, too
} else {
  a1 = b1 + c1;
  a2 = a1 + 88;
  c2 = 3 + 8;
}
```
Gets translated into
```
a1 = b1 + c1; // we hoist all statements that appear in both branches first.
c2 = 3 + 8;
a_then_2 = a1 + 99; // next we handle remaining vars.
a_else_2 = a1 + 88;
a_3 = p ? a_then_2 : a_else_2;
```

Immediately after the if-then-else statement,
  For each modified LHS x, we insert a new assignment statement x= p ? x_i : x_j, where 
  i = VN_then(x), and j = VN_else(x).

Not all these ternaries will be needed, as some values may only be modified in the then/else
blocks. We leave these to be eliminated during DCE.

Example:
```
struct Packet { int a; int b; int some_cond; };
void func(struct Packet p) {
  p.a = 1;
  p.b = 2;
  if (p.some_cond) {
    p.a = 3 + p.b;
    p.b = 4 + p.a;
    p.b = p.b + 2;
  } else {
    p.b = 5 + p.a;
    p.a = 6 + p.b;
  }
  p.a = p.a + p.b;
}
```
Gets turned into:
```
p_a_0 = 1;
p_b_0 = 2;
  p_a_if_1 = 3 + p_b_0;
  p_b_if_1 = 4 + p_a_if_1;
  p_b_if_2 = p_b_if_1 + 2;

  p_b_then_1 = 5 + p_a_0;
  p_a_then_1 = 6 + p_b_then_1;

p_a_1 = p_some_cond ? p_a_if_1 : p_a_then_1;
p_b_1 = p_some_cond ? p_b_if_2 : p_b_then_1;

p_a_2 = p_a_1 + p_b_1; // p.a = p.a + p.b

```


## Additional Observation: Not storing branch condition into a variable might save us a stage.

Recall the RCP example:
```
struct Packet {
  int size_bytes;
  int rtt;
};
// Total number of bytes seen so far.
int input_traffic_Bytes = 0;

// Sum of rtt so far
int sum_rtt_Tr = 0;

// Number of packets with a valid RTT
int num_pkts_with_rtt = 0;

void func(struct Packet p) {
  input_traffic_Bytes = input_traffic_Bytes + p.size_bytes;
  if (p.rtt < MAX_ALLOWABLE_RTT) {
    sum_rtt_Tr = sum_rtt_Tr + p.rtt;
    num_pkts_with_rtt = num_pkts_with_rtt + 1;
  }
}
```

This gets (currently) compiled by the Python-based preprocessor down to
```
p_input_traffic_Bytes0 = input_traffic_Bytes
p_input_traffic_Bytes1 = p_input_traffic_Bytes0 + p.size_bytes0
p_br_tmp0 = (p.rtt0 < 30)
p_sum_rtt_Tr0 = sum_rtt_Tr
p_sum_rtt_Tr1 = p_sum_rtt_Tr0 + p.rtt0
p_num_pkts_with_rtt0 = num_pkts_with_rtt
p_num_pkts_with_rtt1 = p_num_pkts_with_rtt0 + 1
p_sum_rtt_Tr2 = p_br_tmp0 ? p_sum_rtt_Tr1 : p_sum_rtt_Tr0
p_num_pkts_with_rtt2 = p_br_tmp0 ? p_num_pkts_with_rtt1 : p_num_pkts_with_rtt0
input_traffic_Bytes = p_input_traffic_Bytes1
sum_rtt_Tr = p_sum_rtt_Tr2
```
The domino-based preprocessor emits something similar.

**Notice the variable `p_br_tmp0`. Every line above it belongs to the same
stateful SCC, so they will be synthesized as a singleton stateful ALU operation
without any dependencies. The remaining code has a SCC-graph structure like follows:**
```
          <p_br_tmp0 = (p.rtt0 < 30)> (stateless)
          /                             \
         /                               \
        <stateful comp>                 <another stateful comp>
```
This "inverted-V-like" shape causes our compiler to compile things in two stages, but
not one, since the DAG has depth two. But if we "expand" the RHS of `p_br_tmp0` into
both its children then we will manage to compile RCP in just one stage, matching the Chipmunk result.
```

*/
*/

#endif  // SSA_H_
