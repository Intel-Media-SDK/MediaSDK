# -*- coding: utf-8 -*-

# Copyright (c) 2017-2020 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import re
import sys
import argparse
from pathlib import Path

from ted import discover


if __name__ == '__main__':
    print('Intel(R) Media SDK Open Source TEst Driver')
    print('Copyright (c) Intel Corporation\n')

    parser = argparse.ArgumentParser()

    parser.add_argument(
        'test',
        nargs='*',
        help='tests to run, if not specified all tests will be executed'
    )
    parser.add_argument(
        '--device', action='store', default='/dev/dri/renderD128',
        help='provide device on which to run tests (default: /dev/dri/renderD128)'
    )

    args = parser.parse_args()

    base_dir = Path(__file__).parent.absolute()


    try:
        print("Setting up test environment...")
        cfg = discover.config(base_dir)
    except Exception as ex:
        msg = "Can't load configuration - {}".format(ex)
        sys.exit(msg)

    test_re = None
    if args.test:
        test_re = re.compile('|'.join(args.test))

    tests_to_run = []
    print("Disovering tests...")
    for test in discover.tests(base_dir, cfg, args):
        if test_re and not test_re.search(test.name):
            print('  {} - skipped'.format(test.name))
            continue

        tests_to_run.append(test)
        print('  {} - {} cases'.format(test.name, len(test.cases)))

    if not tests_to_run:
        sys.exit("Nothing to run")

    n = len(tests_to_run)
    print("\nRunning {} test{}...".format(n, 's' if n > 1 else ''))

    results = []
    total = passed = 0
    for test in tests_to_run:
        print('  {}'.format(test.name))
        total_, passed_, details = test.run()

        results.append(details)


        total += total_
        passed += passed_

    print("\n{} of {} cases passed".format(passed, total))

    # return code is number of failed cases
    sys.exit(total - passed)
