#!/usr/bin/perl

# Copyright (c) 2017 Intel Corporation
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

use v5.16;
use strict;
use warnings;

use Cwd;
use Getopt::Long;
use File::Path;
use File::Basename;

my $run_build        = 0;
my $clean            = 0;
my $no_warn_as_error = 0;
my %build = (
    'cc'         => '',
    'cxx'        => '',
    'ipp'        => '',
    'trace'      => '',
    'prefix'     => '',
    'toolchain'  => '',
    'target'     => 'BDW',
    'samples-dir'=> '',
    'api'        => '',
    'api-dir'    => ''
);

my @list_generator = qw(make eclipse);
my @list_arch      = qw(intel64);
my @list_config    = qw(release debug);
my @list_trace     = qw(itt all);
my @list_comp      = qw(gcc icc clang);
my @list_target    = qw(UPSTREAM BDW BSW BXT BXTMIN CFL);
my @list_ipp       = qw(
  px a6 w7 t7 v8 s8 p8 g9
  mx m7 u8 n8 y8 e9
  i7
  sx s2
);
my @list_tools     = qw(on off);

sub in_array {
    my %items;
    my ($arr, $search_for) = @_;
    map { $items{$_} = 1 } @$arr;
    return ($items{$search_for} eq 1) ? 1 : 0;
}

sub command {
    my $cmd = shift;

    local $| = 1;
    print "[ cmd ]: $cmd\n";

    open(my $ps, '-|', $cmd) || die "Failed: $!\n";
    while (<$ps>) {
        chomp;
        s/\r//g;
        say;
    }
    close($ps);

    return $?;
}

sub nativepath {
    my $path = shift;
    $path =~ tr!/!\\! if $^O =~ /Win/;
    return $path;
}

sub get_cmake_target {
    my %config = @_;

    # arch, generator, config are mandatory
    my @cmake_target = ($config{'arch'}, $config{'generator'}, $config{'config'});

    push @cmake_target, $config{'ipp'}  if $config{'ipp'};
    push @cmake_target, $config{'comp'} if $config{'comp'} ne 'gcc';

    if ($config{'trace'} eq 'itt') {
        push @cmake_target, "$config{'trace'}";
    } elsif ($config{'trace'}) {
        push @cmake_target, "trace_$config{'trace'}";
    }
    return join('.', @cmake_target);
}

sub get_cmake_gen_cmd {
    my $build_root = shift;
    my %config = @_;

    my @cmake_cmd_gen = ("--no-warn-unused-cli -Wno-dev");
    # Some sort of trick:
    # If you want to have Makefiles aligned with source code tree which is preferable for
    # Eclipse, string "-D__GENERATOR:STRING=" should be "make". (Does not "eclipse"!!)
    if ($config{'generator'} =~ /^make$/) {
        push @cmake_cmd_gen, '-G "Unix Makefiles"';
        push @cmake_cmd_gen, "-D__GENERATOR:STRING=$config{'generator'}";
    } elsif ($config{'generator'} =~ /^eclipse$/) {
        push @cmake_cmd_gen, '-G "Eclipse CDT4 - Unix Makefiles"';
        push @cmake_cmd_gen, '-D__GENERATOR:STRING=make';
    }

    push @cmake_cmd_gen, "-DSAMPLES_DIR:STRING=$config{'samples-dir'}"  if $config{'samples-dir'};
    push @cmake_cmd_gen, "-DAPI:STRING=$config{'api'}"                  if $config{'api'};
    push @cmake_cmd_gen, "-DAPI_DIR:STRING=$config{'api-dir'}"          if $config{'api-dir'};

    push @cmake_cmd_gen, '-DCMAKE_CONFIGURATION_TYPES:STRING="release;debug"';
    push @cmake_cmd_gen, "-DCMAKE_BUILD_TYPE:STRING=$config{'config'}";
    push @cmake_cmd_gen, "-D__IPP:STRING=" . ($config{'ipp'} || 'e9');
    push @cmake_cmd_gen, "-D__TARGET_PLATFORM:STRING=$config{'target'}";

    push @cmake_cmd_gen, "-D__TRACE:STRING=$config{'trace'}"           if $config{'trace'};
    push @cmake_cmd_gen, "-DENABLE_ITT=ON"                             if $config{'trace'} eq 'itt';
    push @cmake_cmd_gen, "-DCMAKE_C_COMPILER:STRING=$config{'cc'}"     if $config{'cc'};
    push @cmake_cmd_gen, "-DCMAKE_CXX_COMPILER:STRING=$config{'cxx'}"  if $config{'cxx'};
    push @cmake_cmd_gen, "-DCMAKE_TOOLCHAIN_FILE=$config{'toolchain'}" if $config{'toolchain'};
    push @cmake_cmd_gen, "-DCMAKE_INSTALL_PREFIX=$config{'prefix'} "   if $config{'prefix'};
    push @cmake_cmd_gen, "-DBUILD_TOOLS=ON"                            if $config{'tools'} eq 'on';

    my $compile_flags = "";

    if (not $no_warn_as_error) {
      $compile_flags .= "-Wall -Werror"
    }

    if (lc($config{'config'}) eq "release") {
        $compile_flags .= " -O2 -D_FORTIFY_SOURCE=2 -fstack-protector-strong -DNDEBUG";
        push @cmake_cmd_gen, "-DCMAKE_C_FLAGS_RELEASE=\"$compile_flags\"";
        push @cmake_cmd_gen, "-DCMAKE_CXX_FLAGS_RELEASE=\"$compile_flags\"";
    } else {
        $compile_flags .= " -O0 -g";
        push @cmake_cmd_gen, "-DCMAKE_C_FLAGS_DEBUG=\"$compile_flags\"";
        push @cmake_cmd_gen, "-DCMAKE_CXX_FLAGS_DEBUG=\"$compile_flags\"";
    }

    # This is the main different between "make" and "eclipse"" generators.
    # In case of "Eclipse" out-of-source build you have to point cmake
    # (1): on the current $MFX_HOME (source folder with main CMakeLists.txt)
    # this is "-H" option, and 
    # (2): to the desired binary directory 
    # this is "-B" option    
    # With this approach possible to keep separate Eclipse projects for debug/release 
    # configurations or for gcc/icc/clang compilers for one source tree.  
    if ($config{'generator'} =~ /^eclipse$/) {
        push @cmake_cmd_gen, "-B".nativepath($build_root);
        push @cmake_cmd_gen, "-H".nativepath($ENV{MFX_HOME});
    } else {
        push @cmake_cmd_gen, nativepath($build_root);
    }

    return join(' ', @cmake_cmd_gen);
}

sub usage {
    print "\n";
    print "Copyright (c) 2012-2017 Intel Corporation. All rights reserved.\n";
    print "This script performs Intel(R) Media SDK projects creation and build.\n\n";
    print "Usage: perl build.pl --cmake=ARCH.GENERATOR.CONFIG[.COMP] [--ipp=<cpu>] [--clean] [--build] [--trace=<module>] [--cross=toolchain.cmake] [--tools=(on|off)]\n";
    print "\n";
    print "Possible variants:\n";
    print "\tARCH = intel64\n";
    print "\tGENERATOR = make | eclipse\n";
    print "\tCONFIG = debug | release\n";
    print "\tCOMP = gcc | icc | clang (gcc is the default)\n";
    print "\n";
    print "Environment variables:\n";
    print "\tMFX_HOME=/path/to/mediasdk/package # required\n";
    print "\tMFX_VERSION=\"0.0.000.0000\"         # optional\n";
    print "\n";
    print "Optional flags:\n";
    print "\t--ipp    - build specificly with optimization for the specified CPU\n";
    print "\t--clean  - clean build directory before projects generation / build\n";
    print "\t--build  - try to build projects before generation\n";
    print "\t--trace  - enable MFX tracing with specified modules (itt|all)\n";
    print "\t--cross  - provide cross-compiler setings to CMake\n";
    print "\t--tools  - enable building of tools (on|off)\n";
    print "\t--target - select feature subset specific to target project. default is BDW (", @list_target, ")\n";
    print "\t--no-warn-as-error  - disable Warning As Error\n";
    print "\t--prefix - set install prefix\n";
    print "\t--samples-dir - select directory root for samples to build from\n";
    print "\t--api - specify target API version. Use <maj>.<minor> or master\n";
    print "\t--api-dir - select directory root for api build from\n";
    print "\n";
    print "Examples:\n";
    print "\tperl build_mfx.pl --cmake=intel64.make.debug                 [ only generate projects    ]\n";
    print "\tperl build_mfx.pl --cmake=intel64.make.debug --build         [ generate and then build   ]\n";
    print "\tperl build_mfx.pl --cmake=intel64.make.debug --build --clean [ generate, clean and build ]\n";
    print "\tperl build_mfx.pl --cmake=intel64.eclipse.debug              [ generate make files for the use in Eclipse.]\n";
    print "\t                                                             [ And Eclipse project by itself. This for out-of-source build.]\n";
    print "\t                                                             [ Current MFX_HOME must be defined!]\n";
    print "\t                                                             [ all other options like --build --clean remain the same ]\n";
    print "\n";

    exit;
}

sub _opt_in_list {
    my $opt_name  = shift;
    my $opt_value = shift;
    my $ok_values = shift;

    if (not in_array($ok_values, $opt_value)) {
        my @choises = ();
        map { push @choises, $_ ? $_ : '<empty>' } @$ok_values;
        my $err = "ERROR: Invalid value for --$opt_name = '$opt_value'.\n";
        $err .= "Possible options: [" . join(', ', @choises) . "]";
        die $err;
    }

    $build{$opt_name} = $opt_value;

    return;
}

sub _opt_cmake_handler {
    my $opt_name = shift;
    my $cmake    = shift;

    die "ERROR: --$opt_name is mandatory" if not $cmake;

    ($build{'arch'}, $build{'generator'}, $build{'config'}, $build{'comp'}) = split /,|\./, $cmake;
    if (not $build{'comp'}) {
        $build{'comp'} = 'gcc';
    } elsif ($build{'comp'} eq 'icc') {
        $build{'cc'}  = 'icc';
        $build{'cxx'} = 'icpc';
    } elsif ($build{'comp'} eq 'clang') {
        $build{'cc'}  = 'clang';
        $build{'cxx'} = 'clang++';
    }

    _opt_in_list('cmake[0]', $build{'arch'}, \@list_arch);
    _opt_in_list('cmake[1]', $build{'generator'}, \@list_generator);
    _opt_in_list('cmake[2]', $build{'config'}, \@list_config);
    _opt_in_list('cmake[3]', $build{'comp'}, \@list_comp);
}

usage() unless @ARGV;

GetOptions(
    '--no-warn-as-error' => \$no_warn_as_error,
    '--build'            => \$run_build,
    '--clean'            => \$clean,
    '--prefix=s'         => \$build{'prefix'},
    '--cross=s'          => \$build{'toolchain'},
    '--samples-dir=s'    => \$build{'samples-dir'},
    '--api=s'            => \$build{'api'},
    '--api-dir=s'        => \$build{'api-dir'},

    '--cmake=s' => \&_opt_cmake_handler,

    '--ipp=s'    => sub { my ($k, $v) = @_; _opt_in_list($k, $v, ['', @list_ipp]);    },
    '--trace=s'  => sub { my ($k, $v) = @_; _opt_in_list($k, $v, ['', @list_trace]);  },
    '--target=s' => sub { my ($k, $v) = @_; _opt_in_list($k, $v, ['', @list_target]); },
	'--tools=s'  => sub { my ($k, $v) = @_; _opt_in_list($k, $v, ['', @list_tools]); },
) or usage();

my $build_root = getcwd;
my $target_path = "$build_root/__cmake/" . get_cmake_target(%build);

my $cmake_gen_cmd;
# "make" and "eclipse"" generators output directory is different
if ($build{'generator'} =~ /^eclipse$/) {
    my $mfx_home_path = nativepath($ENV{MFX_HOME});    
    $target_path = "$build_root/" . get_cmake_target(%build);
    rmtree($target_path) if -d $target_path  and $clean;
    mkpath($target_path) if !-d $target_path and -d $build_root;
    #chdir($target_path) or die "can't chdir -> $target_path: $!";
    # Should be used ($mfx_home_path)!
    chdir($mfx_home_path) or die "can't chdir -> $target_path: $!";
    $cmake_gen_cmd = get_cmake_gen_cmd($target_path, %build);
} else {
    rmtree($target_path) if -d $target_path  and $clean;
    mkpath($target_path) if !-d $target_path and -d $build_root;
    chdir($target_path) or die "can't chdir -> $target_path: $!";
    $cmake_gen_cmd = get_cmake_gen_cmd($build_root, %build);
}

my $exit_code = 0;
say "I am going to execute:\n  cmake $cmake_gen_cmd";
$exit_code = command("cmake $cmake_gen_cmd");

if ($run_build) {
    $exit_code = command("cmake --build $target_path --use-stderr --config $build{'config'}");

    my $status_target = "$build{'arch'}.$build{'generator'}.$build{'config'}";
    my $status_state = (!$exit_code) ? "OK" : "FAIL";

    printf "\n[ %-50s State: %-4s ] \n\n\n", $status_target, $status_state;
}

exit $exit_code;
