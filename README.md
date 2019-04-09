# Tomasulo's Algorithm

This project aims to implement the Tomasulo's Algorithm. The hardware is presented in the Assignment Description as follows:
1. Same cycle issue-dispatch: Not allowed
2. Same cycle free-allocate: Not allowed
3. Same cycle capture-dispatch: Not allowed
4. Dispatch precedence: RS0 > RS1 > RS2, RS3 > RS4

Additional implementational assumptions made:
1. The operational units are pipelined, ie there is an add pipeline, a sub pipeline a mul pipeline, a div pipeline.
2. At most one instruction from add/sub and one from mul/div reservation stations are allowed to dispatch in one cycle.
3. In case of multiple pipelines broadcasting in same cycle, only the highest broadcast precedence (mul > add, RS3 > RS4, RS0 > RS1 > RS2) can broadcast and the others ready-to-broadcast pipelines are stalled.

To run the code:
1. Compile as `g++ -std=c++11 hpca.cpp -o hpca`
2. Run with input in a text file (say `ip.txt`) as `./hpca < ip.txt` 