/*
Copyright (c) 2024 by Andrei Cravtov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include "neighbour_test.h"
#include "test_utilities.h"

#include "../babeld.h"
#include "../interface.h"
#include "../neighbour.h"

static void
setup_neighbour(struct neighbour *neigh, struct interface *ifp)
{
    memset(neigh, 0, sizeof(struct neighbour));
    memset(ifp, 0, sizeof(struct interface));

    now.tv_sec = 1000;
    now.tv_usec = 0;

    ifp->flags = IF_UP;
    ifp->cost = 96;

    neigh->ifp = ifp;
    neigh->txcost = 96;
    neigh->hello.reach = 0xffff;
    neigh->hello.time = now;
    neigh->uhello.time = now;
    neigh->rtt_time = now;
    neigh->external_bias_256 = 0;
    neigh->external_coef_256 = 256;
}

static void
neighbour_cost_transform_test(void)
{
    int i;
    struct interface ifp;
    struct neighbour neigh;

    struct test_case {
        unsigned txcost;
        int bias_256;
        unsigned coef_256;
        unsigned rtt;
        unsigned max_rtt_penalty;
        unsigned expected;
    } tcs[] = {
        {96, 0, 256, 0, 0, 96},
        {96, 40960, 256, 0, 0, 256},
        {96, 0, 128, 0, 0, 48},
        {96, 300 * 256, 0, 0, 0, 300},
        {96, -200 * 256, 256, 0, 0, 1},
        {INFINITY - 1, 0, 256, 0, 0, INFINITY - 1},
        {INFINITY - 1, 128, 256, 0, 0, INFINITY},
        {96, 0, 256, 1000, 50, 146},
        {96, 0, 128, 1000, 50, 98},
        {INFINITY, -200 * 256, 0, 0, 0, INFINITY},
    };

    for(i = 0; i < sizeof(tcs) / sizeof(tcs[0]); i++) {
        unsigned cost;

        setup_neighbour(&neigh, &ifp);
        neigh.txcost = tcs[i].txcost;
        neigh.rtt = tcs[i].rtt;
        ifp.rtt_min = 0;
        ifp.rtt_max = 1000;
        ifp.max_rtt_penalty = tcs[i].max_rtt_penalty;
        neighbour_external_cost_configure(&neigh, tcs[i].bias_256,
                                          tcs[i].coef_256);

        cost = neighbour_cost(&neigh);

        if(!babel_check(cost == tcs[i].expected)) {
            fprintf(stderr,
                    "neighbour_cost case %d = %u, expected %u.\n",
                    i, cost, tcs[i].expected);
        }
    }
}

void
neighbour_test_suite(void)
{
    run_test(neighbour_cost_transform_test, "neighbour_cost_transform_test");
}
