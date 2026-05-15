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

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include "configuration_test.h"
#include "test_utilities.h"

#include "../babeld.h"
#include "../configuration.h"
#include "../interface.h"
#include "../neighbour.h"

static int
parse_command(const char *command, const char **message_return)
{
    char buf[512];
    int n;

    if(message_return)
        *message_return = NULL;

    n = snprintf(buf, sizeof(buf), "%s", command);
    if(n < 0 || n >= (int)sizeof(buf))
        return -1;

    return parse_config_from_string(buf, strlen(buf), message_return);
}

static int
message_is(const char *message, const char *expected)
{
    return message != NULL && strcmp(message, expected) == 0;
}

void
neighbour_cost_config_command_test(void)
{
    const char *message;
    struct interface *ifp;
    struct neighbour *neigh;
    unsigned char address[16] =
        {0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x42};

    local_server_write = 1;
    if(!babel_check(parse_command("neighbour-cost config_if fe80::42 "
                                  "bias-256 0 coef-256 256",
                                  &message) == -1)) {
        fprintf(stderr, "neighbour-cost unexpectedly parsed before "
                        "configuration finalisation.\n");
    }

    ifp = add_interface("config_if", NULL);
    if(!babel_check(ifp != NULL)) {
        fprintf(stderr, "add_interface failed.\n");
        return;
    }
    ifp->buf.size = 1500;
    neigh = find_neighbour(address, ifp);
    if(!babel_check(neigh != NULL))
        return;

    if(!babel_check(finalise_config() > 0)) {
        fprintf(stderr, "finalise_config failed.\n");
        return;
    }

    local_server_write = 0;
    if(!babel_check(parse_command("neighbour-cost config_if fe80::42 "
                                  "bias-256 0 coef-256 256",
                                  &message) == -1)) {
        fprintf(stderr, "neighbour-cost unexpectedly parsed on read-only "
                        "local control.\n");
    }

    local_server_write = 1;
    if(!babel_check(parse_command("neighbour-cost missing_if fe80::42 "
                                  "bias-256 0 coef-256 256",
                                  &message) == CONFIG_ACTION_NO &&
                    message_is(message, "No such interface"))) {
        fprintf(stderr, "expected no No such interface, got %s.\n",
                message ? message : "(null)");
    }

    if(!babel_check(parse_command("neighbour-cost config_if 2001:db8::1 "
                                  "bias-256 0 coef-256 256",
                                  &message) == CONFIG_ACTION_NO &&
                    message_is(message, "Address is not link-local"))) {
        fprintf(stderr, "expected no Address is not link-local, got %s.\n",
                message ? message : "(null)");
    }

    if(!babel_check(parse_command("neighbour-cost config_if fe80::43 "
                                  "bias-256 0 coef-256 256",
                                  &message) == CONFIG_ACTION_NO &&
                    message_is(message, "No such neighbour"))) {
        fprintf(stderr, "expected no No such neighbour, got %s.\n",
                message ? message : "(null)");
    }

    if(!babel_check(parse_command("neighbour-cost config_if fe80::42 "
                                  "bias-256 0 coef-256",
                                  &message) == -1)) {
        fprintf(stderr, "malformed neighbour-cost command parsed.\n");
    }

    if(!babel_check(parse_command("neighbour-cost config_if fe80::42 "
                                  "bias-256 0 coef-256 0x100000000",
                                  &message) == -1)) {
        fprintf(stderr, "oversized coef-256 parsed.\n");
    }

    if(!babel_check(parse_command("neighbour-cost config_if fe80::42 "
                                  "bias-256 0 coef-256 65536",
                                  &message) == -1)) {
        fprintf(stderr, "out-of-range coef-256 parsed.\n");
    }

    if(!babel_check(parse_command("neighbour-cost config_if fe80::42 "
                                  "bias-256 0x100000000 coef-256 256",
                                  &message) == -1)) {
        fprintf(stderr, "oversized bias-256 parsed.\n");
    }

    if(!babel_check(parse_command("neighbour-cost config_if fe80::42 "
                                  "bias-256 16776705 coef-256 256",
                                  &message) == -1)) {
        fprintf(stderr, "out-of-range bias-256 parsed.\n");
    }

    if(!babel_check(parse_command("neighbour-cost config_if fe80::42 "
                                  "bias-256 40960 coef-256 256 # comment",
                                  &message) == CONFIG_ACTION_DONE)) {
        fprintf(stderr, "valid neighbour-cost command failed.\n");
    }
    if(!babel_check(neighbour_external_bias_256(neigh) == 40960 &&
                    neighbour_external_coef_256(neigh) == 256)) {
        fprintf(stderr, "neighbour-cost did not update neighbour state.\n");
    }

    if(!babel_check(parse_command("neighbour-cost config_if fe80::42 "
                                  "bias-256 0 coef-256 256",
                                  &message) == CONFIG_ACTION_DONE)) {
        fprintf(stderr, "neutral neighbour-cost reset failed.\n");
    }
    if(!babel_check(neighbour_external_bias_256(neigh) == 0 &&
                    neighbour_external_coef_256(neigh) == 256)) {
        fprintf(stderr, "neutral neighbour-cost reset did not restore state.\n");
    }
}

void
configuration_test_suite(void)
{
    run_test(neighbour_cost_config_command_test,
             "neighbour_cost_config_command_test");
}
