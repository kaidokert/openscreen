###

The objective is to port osp_demo to windows.

Our strategy generally is layered, full of stubs and hacks at every layer

1) get GN to execute and produce a build tree

Often this requires stubbing out chunks of dependendies and conditionals for build

2) Get autoninja to compile all code and produce object code

Stub out functions with complex dependency chains, add #ifdef _WIN32 stub block - mark it as TODO: windows
If something is exceedingly complex, put a tiny target in compile_smoketest that recreates the problem and isolates it in single file. Fix it there and then generalize the solution
Mark stubs as OSP_UNIMPLEMENTED

3) Get autoninja to link all relevant targets and produce executables / dynamic libs

Provide link stubs for missing symbols - e.g. just create a stub WindowsFooBar struct, class, function - obviously make them OSP_UNIMPLEMENTED

4) Actually try to run those executables and libs. 

Now we are going to run into real issues and can start gradually fixing code. Our key condition is that 
`autoninja -C out\debug` needs to stay fully working - even as we iterate on crummy / stub code.

Obviously we need to fix unit tests and also add bespoke compile_smoketest/ unit tests to isolate issues
one by one and plow through the stack, from bottom layers to top ones.


Feel free to always `git diff main` or look at `git diff main -- some_dir` specifically to re-assess the
extent of committed hacks