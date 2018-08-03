#!/usr/bin/perl

##******************************************************************************
##  Copyright(C) 2012-2017 Intel Corporation. All Rights Reserved.
##  
##  The source code, information  and  material ("Material") contained herein is
##  owned  by Intel Corporation or its suppliers or licensors, and title to such
##  Material remains  with Intel Corporation  or its suppliers or licensors. The
##  Material  contains proprietary information  of  Intel or  its  suppliers and
##  licensors. The  Material is protected by worldwide copyright laws and treaty
##  provisions. No  part  of  the  Material  may  be  used,  copied, reproduced,
##  modified, published, uploaded, posted, transmitted, distributed or disclosed
##  in any way  without Intel's  prior  express written  permission. No  license
##  under  any patent, copyright  or  other intellectual property rights  in the
##  Material  is  granted  to  or  conferred  upon  you,  either  expressly,  by
##  implication, inducement,  estoppel or  otherwise.  Any  license  under  such
##  intellectual  property  rights must  be express  and  approved  by  Intel in
##  writing.
##  
##  *Third Party trademarks are the property of their respective owners.
##  
##  Unless otherwise  agreed  by Intel  in writing, you may not remove  or alter
##  this  notice or  any other notice embedded  in Materials by Intel or Intel's
##  suppliers or licensors in any way.
##
##******************************************************************************
##  Content: Intel(R) Media SDK Samples projects creation and build
##******************************************************************************

$| = 1;

use strict;

use Cwd;
use Getopt::Long;
use File::Path;
use File::Basename;

my %build = ();
my $cmake = "";
my $comp  = "";
my $build = "";
my $clean = "";
my $msdk  = "";
my $toolchain = "";
my $mfx_home = "";
my $vtune_home = "";
my $enable_itt = "no";
my $enable_sw  = "yes";
my $enable_drm = "yes";
my $enable_x11 = "yes";
my $enable_x11_dri3="no";
my $enable_wayland = "no";
my $enable_v4l2 = "no";
my $enable_mondello = "no";
my $enable_ffmpeg = "yes";
my $enable_opencl = "yes";
my $enable_ps = "no";
my $enable_ff = "no";
my $prefix = "";
my $test  = "";
my $verb  = "";

sub in_array {
  my %items;
  my ($arr, $search_for) = @_;
  map {$items{$_} = 1 } @$arr;
  return ($items{$search_for} eq 1) ? 1 : 0;
}

sub command {
  print "[ cmd ]: $_[0]\n" if $verb;
  open( PS,"$_[0] |" ) || die "Failed: $!\n";
    while ( <PS> ) {
      chomp( $_ );
      s/\r//g;
      print "$_\n";
    }
  close( PS );
  return $?;
}

sub nativepath {
  my $path = shift;
  $path =~ tr!/!\\! if $^O =~ /Win/;
  return $path;
}

my @list_yesno     = qw(yes no);
my @list_generator = qw(make);
my @list_arch      = qw(intel64);
my @list_config    = qw(release debug);


sub usage {
  print "\n";
  print "Copyright (c) 2012-2017 Intel Corporation. All rights reserved.\n";
  print "This script performs Intel(R) Media SDK Samples projects creation and build.\n\n";
  print "Usage: perl build.pl --cmake=ARCH.GENERATOR.CONFIG [options]\n";
  print "\n";
  print "Possible variants:\n";
  print "\tARCH = intel64\n";
  print "\tGENERATOR = make\n";
  print "\tCONFIG = debug | release\n";
  print "\n";
  print "Environment variables:\n";
  print "\tMFX_HOME=/path/to/mediasdk/package # required, can be overwritten by --mfx-home option\n";
  print "\tVTUNE_HOME=/path/to/vtune/package  # optional, can be overwritten by --vtune-home option\n";
  print "\tMFX_VERSION=\"0.0.000.0000\"       # optional\n";
  print "\n";
  print "Optional flags:\n";
  print "\t--clean - clean build directory before projects generation / build\n";
  print "\t--build - try to build projects after generation (requires cmake>=2.8.0)\n";
  print "\t--mfx-home=/path/to/mediasdk/package - Media SDK package location [default: <none>]\n";
  print "\t--vtune-home=/path/to/vtune/package - VTune package location [default: <none>]\n";
  print "\t--enable-itt=yes|no     - enable ITT instrumentation support [default: $enable_itt]\n";
  print "\t--enable-sw=yes|no      - enable SW backend support [default: $enable_sw]\n";
  print "\t--enable-drm=yes|no     - enable DRM backend support [default: $enable_drm]\n";
  print "\t--enable-x11=yes|no     - enable X11 backend support [default: $enable_x11]\n";
  print "\t--enable-x11-dri3=yes|no - enable X11 DRI3 backend support [default: $enable_x11_dri3]\n";
  print "\t--enable-wayland=yes|no - enable Wayland backend support [default: $enable_wayland]\n";
  print "\t--enable-v4l2=yes|no - enable v4l2 support [default: $enable_v4l2]\n";
  print "\t--enable-mondello=yes|no - enable MOndello/v4l2 support [default: $enable_mondello]\n";
  print "\t--enable-ffmpeg=yes|no  - build ffmpeg dependent targets [default: $enable_ffmpeg]\n";
  print "\t--enable-opencl=yes|no  - build OpenCL dependent targets [default: $enable_opencl]\n";

  print "\t--prefix=PATH - set install prefix\n";
  print "\n";
  print "Examples:\n";
  print "\tperl build.pl --cmake=intel64.make.debug                 [ only generate projects    ]\n";
  print "\tperl build.pl --cmake=intel64.make.debug --build         [ generate and then build   ]\n";
  print "\tperl build.pl --cmake=intel64.make.debug --build --clean [ generate, clean and build ]\n";
  print "\n";

  exit;
}

usage( ) unless @ARGV;

GetOptions (
  '--cmake=s' => \$cmake,
  '--build'   => \$build,
  '--clean'   => \$clean,
  '--verbose' => \$verb,
  '--cross=s' => \$toolchain,
  '--mfx-home=s' => \$mfx_home,
  '--vtune-home=s' => \$vtune_home,
  '--enable-sw=s' => \$enable_sw,
  '--enable-itt=s' => \$enable_itt,
  '--enable-drm=s' => \$enable_drm,
  '--enable-x11=s' => \$enable_x11,
  '--enable-x11-dri3=s' => \$enable_x11_dri3,
  '--enable-wayland=s' => \$enable_wayland,
  '--enable-v4l2=s' => \$enable_v4l2,
  '--enable-mondello=s' => \$enable_mondello,
  '--enable-ffmpeg=s' => \$enable_ffmpeg,
  '--enable-opencl=s' => \$enable_opencl,
  '--enable-ps=s' => \$enable_ps,
  '--enable-ff=s' => \$enable_ff,
  '--prefix=s' => \$prefix
);

(
  $build{'arch'},
  $build{'generator'},
  $build{'config'}
) = split /,|\./,$cmake;

my $configuration_valid = 0;

if(in_array(\@list_arch, $build{'arch'}) and
   in_array(\@list_generator, $build{'generator'}) and
   in_array(\@list_config, $build{'config'}) and
   in_array(\@list_yesno, $enable_sw) and
   in_array(\@list_yesno, $enable_drm) and
   in_array(\@list_yesno, $enable_x11) and
   in_array(\@list_yesno, $enable_x11_dri3) and
   in_array(\@list_yesno, $enable_wayland) and
   in_array(\@list_yesno, $enable_v4l2) and
   in_array(\@list_yesno, $enable_mondello) and
   in_array(\@list_yesno, $enable_ffmpeg) and
   in_array(\@list_yesno, $enable_opencl) and
   in_array(\@list_yesno, $enable_ps) and 
   in_array(\@list_yesno, $enable_ff)) {
   $configuration_valid = 1;
}

unless($configuration_valid) {
  print "ERROR: invalid configuration! Please call script without argument for usage.\n\n";
  usage( );
  exit 1;
}

my $builder = File::Spec->rel2abs(dirname(__FILE__));

my $cmake_target = "$build{'arch'}.$build{'generator'}";
$cmake_target.= ".$build{'config'}" if $build{'generator'} =~ /make|eclipse/;

my $target_path = "$builder/__cmake/$cmake_target";
my $sample_path = "$builder";

my $cmake_cmd_bld = "--build $target_path --use-stderr --config $build{'config'}";
my $cmake_cmd_gen = "--no-warn-unused-cli -Wno-dev -G\"__generator__\" -DCMAKE_CONFIGURATION_TYPES:STRING='release;debug' ";
$cmake_cmd_gen.= "-DCMAKE_BUILD_TYPE:STRING=$build{'config'} " if($build{'generator'} =~ /make|eclipse/);
$cmake_cmd_gen.= "-D__GENERATOR:STRING=$build{'generator'} -D__ARCH:STRING=$build{'arch'} -D__CONFIG:STRING=$build{'config'} ";
$cmake_cmd_gen.= "-DCMAKE_TOOLCHAIN_FILE=$toolchain " if $toolchain ne "";

$cmake_cmd_gen.= "-DENABLE_ITT:STRING=" . (($enable_itt eq "yes") ? "ON": "OFF") . " ";
$cmake_cmd_gen.= "-DENABLE_SW:STRING=" . (($enable_sw eq "yes") ? "ON": "OFF") . " ";
$cmake_cmd_gen.= "-DENABLE_DRM:STRING=" . (($enable_drm eq "yes") ? "ON": "OFF") . " ";
$cmake_cmd_gen.= "-DENABLE_X11:STRING=" . (($enable_x11 eq "yes") ? "ON": "OFF") . " ";
$cmake_cmd_gen.= "-DENABLE_X11_DRI3:STRING=" . (($enable_x11_dri3 eq "yes") ? "ON": "OFF") . " ";
$cmake_cmd_gen.= "-DENABLE_WAYLAND:STRING=" . (($enable_wayland eq "yes") ? "ON": "OFF") . " ";
$cmake_cmd_gen.= "-DENABLE_V4L2:STRING=" . (($enable_v4l2 eq "yes") ? "ON": "OFF") . " ";
$cmake_cmd_gen.= "-DENABLE_MONDELLO:STRING=" . (($enable_mondello eq "yes") ? "ON": "OFF") . " ";
$cmake_cmd_gen.= "-DENABLE_FFMPEG:STRING=" . (($enable_ffmpeg eq "yes") ? "ON": "OFF") . " ";
$cmake_cmd_gen.= "-DENABLE_OPENCL:STRING=" . (($enable_opencl eq "yes") ? "ON": "OFF") . " ";
$cmake_cmd_gen.= "-DENABLE_PS:STRING=" . (($enable_ps eq "yes") ? "ON": "OFF") . " ";
$cmake_cmd_gen.= "-DENABLE_FF:STRING=" . (($enable_ff eq "yes") ? "ON": "OFF") . " ";

$cmake_cmd_gen.= "-DCMAKE_INSTALL_PREFIX=$prefix " if $prefix ne "";

my $mfx_home_abs = "";

$mfx_home_abs = File::Spec->rel2abs($mfx_home) if $mfx_home ne "";
$cmake_cmd_gen.= "-DCMAKE_MFX_HOME:STRING=$mfx_home_abs " if $mfx_home_abs ne "";

my $vtune_home_abs = "";

$vtune_home_abs = File::Spec->rel2abs($vtune_home) if $vtune_home ne "";
$cmake_cmd_gen.= "-DCMAKE_VTUNE_HOME:STRING=$vtune_home_abs " if $vtune_home_abs ne "";

$cmake_cmd_gen.= nativepath($sample_path);

$cmake_cmd_gen =~ s/__generator__/Unix Makefiles/g if($build{'generator'} =~ /^make$/);

rmtree($target_path) if   -d $target_path and $clean;
mkpath($target_path) if ! -d $target_path and -d $sample_path;
chdir ($target_path) or die "can't chdir -> $target_path: $!";

my $exit = "";
$exit = command ("cmake $cmake_cmd_gen");

if($build){
  $exit = command ("cmake $cmake_cmd_bld");

  my $status_target = "$build{'arch'}.$build{'generator'}.$build{'config'}";
  my $status_state  = (!$exit) ? "OK" : "FAIL";

  printf "\n[ %-50s State: %-4s ] \n\n\n", $status_target, $status_state;
}

exit $exit;
