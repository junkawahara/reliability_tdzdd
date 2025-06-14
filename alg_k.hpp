
// replace var1 for target
bddp replace(bddp bdd, int var1, bddp target)
{
    bddp b0 = bddat0(bdd, var1);
    bddp b1 = bddat1(bdd, var1);

    bddp tnot = bddnot(target);
    bddp m0 = bddand(tnot, b0);
    bddfree(tnot);
    bddfree(b0);

    bddp m1 = bddand(target, b1);
    bddfree(b1);
    
    bddp r = bddor(m0, m1);
    bddfree(m0);
    bddfree(m1);

    return r;
}

bddp alg_k(bddp f, const tdzdd::Graph& graph, int m, int n, int* e_list, int* v_list)
{
    f = bddcopy(f);
    //for (int i = 0; i < m; ++i) {
    for (int i = m - 1; i >= 0; --i) {
        const tdzdd::Graph::EdgeInfo& edge = graph.edgeInfo(i);
        int v1 = edge.v1;
        int v2 = edge.v2;
        bddp q1 = bddprime(e_list[i]);
        bddp q2 = bddprime(v_list[v1]);
        bddp q3 = bddprime(v_list[v2]);
        bddp q4 = bddand(q1, q2);
        bddfree(q1);
        bddfree(q2);
        bddp q5 = bddand(q4, q3);
        bddfree(q3);
        bddfree(q4);
        bddp g = replace(f, e_list[i], q5);
        bddfree(f);
        bddfree(q5);
        f = g;
    }
    return f;
}
