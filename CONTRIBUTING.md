We welcome community contributions to Intel® Media SDK. Thank you for your time!

Please note that review and merge might take some time at this point.

Intel Media SDK is licensed under MIT license. By contributing to the project, you agree to the license and copyright terms therein and release your contribution under these terms.

Steps:
 - Please follow guidelines in your code (see [MSDK Coding Guide](./doc/Coding_guidelines.md))
 - Validate that your changes don't break a build. See [build instruction](./README.md#how-to-build)
 - Pass [testing](#testing)
 - Wait while your patchset is reviewed and tested by our internal validation cycle

# Testing

## How to test your changes

1. Test Driver

    Run test driver for any changes inside library and samples (see [Test Driver instruction](./tests/README.md#Test-driver))

1. Unit tests

    If you make changes in mfx_dispatch or tracer, you need to launch unit tests (see [Unit Tests instruction](./tests/README.md#Unit-tests))
