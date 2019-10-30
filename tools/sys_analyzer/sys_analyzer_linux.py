#!/usr/bin/python3
"""

/***********************************************************************************

Copyright (C) 2019 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***********************************************************************************/

File Name: sys_analyzer_linux.py

Abstract: Script for checking the environment for media-driver/Media SDK

Notes: Run ./sys_analyzer_linux.py

"""

import os, sys, platform
import os.path
import pkgconfig # To install: "apt install python3-pip && pip3 install pkgconfig" or "python3 -m pip install pkgconfig"

class diagnostic_colors:
    ERROR   = '\x1b[31;1m'  # Red/bold
    SUCCESS = '\x1b[32;1m'  # green/bold
    RESET   = '\x1b[0m'     # Reset attributes
    INFO    = '\x1b[34;1m'  # info
    OUTPUT  = ''            # command's coutput printing
    STDERR  = '\x1b[36;1m'  # cyan/bold
    WARNING = '\x1b[33;1m'  # yellow/bold

class loglevelcode:
    ERROR   = 0
    SUCCESS = 1
    INFO    = 2
    WARNING = 3

def print_info( msg, loglevel ):

    """ printing information """

    if loglevel==loglevelcode.ERROR:
        color = diagnostic_colors.ERROR
        msgtype=" [ ERROR ] "
        print( color + msgtype + diagnostic_colors.RESET + msg )
    elif loglevel==loglevelcode.SUCCESS:
        color = diagnostic_colors.SUCCESS
        msgtype=" [ OK ] "
        print( color + msgtype + diagnostic_colors.RESET + msg )
    elif loglevel==loglevelcode.INFO:
        color = diagnostic_colors.INFO
        msgtype=" [ INFO ] "
        print( color + msgtype + diagnostic_colors.RESET + msg )
    elif loglevel==loglevelcode.WARNING:
        color = diagnostic_colors.WARNING
        msgtype=" [ WRN ] "
        print( color + msgtype + diagnostic_colors.RESET + msg )

    return

def run_cmd(cmd):
    output=""
    fin=os.popen(cmd+" 2>&1","r")
    for line in fin:
        output+=line
    fin.close()
    return output

def parse_pciid(pciid):
    sts=loglevelcode.ERROR
    gpustr="unknown"

    id_directory={
    '0402':'HSW GT1 desktop',
    '0412':'HSW GT2 desktop',
    '0422':'HSW GT3 desktop',
    '040a':'HSW GT1 server',
    '041a':'HSW GT2 server',
    '042a':'HSW GT3 server',
    '040B':'HSW GT1 reserved',
    '041B':'HSW GT2 reserved',
    '042B':'HSW GT3 reserved',
    '040E':'HSW GT1 reserved',
    '041E':'HSW GT2 reserved',
    '042E':'HSW GT3 reserved',
    '0C02':'HSW SDV GT1 desktop',
    '0C12':'HSW SDV GT2 desktop',
    '0C22':'HSW SDV GT3 desktop',
    '0C0A':'HSW SDV GT1 server',
    '0C1A':'HSW SDV GT2 server',
    '0C2A':'HSW SDV GT3 server',
    '0C0B':'HSW SDV GT1 reserved',
    '0C1B':'HSW SDV GT2 reserved',
    '0C2B':'HSW SDV GT3 reserved',
    '0C0E':'HSW SDV GT1 reserved',
    '0C1E':'HSW SDV GT2 reserved',
    '0C2E':'HSW SDV GT3 reserved',
    '0A02':'HSW ULT GT1 desktop',
    '0A12':'HSW ULT GT2 desktop',
    '0A22':'HSW ULT GT3 desktop',
    '0A0A':'HSW ULT GT1 server',
    '0A1A':'HSW ULT GT2 server',
    '0A2A':'HSW ULT GT3 server',
    '0A0B':'HSW ULT GT1 reserved',
    '0A1B':'HSW ULT GT2 reserved',
    '0A2B':'HSW ULT GT3 reserved',
    '0D02':'HSW CRW GT1 desktop',
    '0D12':'HSW CRW GT2 desktop',
    '0D22':'HSW CRW GT3 desktop',
    '0D0A':'HSW CRW GT1 server',
    '0D1A':'HSW CRW GT2 server',
    '0D2A':'HSW CRW GT3 server',
    '0D0B':'HSW CRW GT1 reserved',
    '0D1B':'HSW CRW GT2 reserved',
    '0D2B':'HSW CRW GT3 reserved',
    '0D0E':'HSW CRW GT1 reserved',
    '0D1E':'HSW CRW GT2 reserved',
    '0D2E':'HSW CRW GT3 reserved',
    '0406':'HSW GT1 mobile',
    '0416':'HSW GT2 mobile',
    '0426':'HSW GT2 mobile',
    '0C06':'HSW SDV GT1 mobile',
    '0C16':'HSW SDV GT2 mobile',
    '0C26':'HSW SDV GT3 mobile',
    '0A06':'HSW ULT GT1 mobile',
    '0A16':'HSW ULT GT2 mobile',
    '0A26':'HSW ULT GT3 mobile',
    '0A0E':'HSW ULX GT1 mobile',
    '0A1E':'HSW ULX GT2 mobile',
    '0A2E':'HSW ULT GT3 reserved',
    '0D06':'HSW CRW GT1 mobile',
    '0D16':'HSW CRW GT2 mobile',
    '0D26':'HSW CRW GT3 mobile',
    '1602':'BDW GT1 ULT',
    '1606':'BDW GT1 ULT',
    '160B':'BDW GT1 Iris',
    '160E':'BDW GT1 ULX',
    '1612':'BDW GT2 Halo',
    '1616':'BDW GT2 ULT',
    '161B':'BDW GT2 ULT',
    '161E':'BDW GT2 ULX',
    '160A':'BDW GT1 Server',
    '160D':'BDW GT1 Workstation',
    '161A':'BDW GT2 Server',
    '161D':'BDW GT2 Workstation',
    '1622':'BDW GT3 ULT',
    '1626':'BDW GT3 ULT',
    '162B':'BDW GT3 Iris',
    '162E':'BDW GT3 ULX',
    '162A':'BDW GT3 Server',
    '162D':'BDW GT3 Workstation',
    '1632':'BDW RSVD ULT',
    '1636':'BDW RSVD ULT',
    '163B':'BDW RSVD Iris',
    '163E':'BDW RSVD ULX',
    '163A':'BDW RSVD Server',
    '163D':'BDW RSVD Workstation',
    '1906':'SKL ULT GT1',
    '190E':'SKL ULX GT1',
    '1902':'SKL DT GT1',
    '190B':'SKL Halo GT1',
    '190A':'SKL SRV GT1',
    '1916':'SKL ULT GT2',
    '1921':'SKL ULT GT2F',
    '191E':'SKL ULX GT2',
    '1912':'SKL DT GT2',
    '191B':'SKL Halo GT2',
    '191A':'SKL SRV GT2',
    '191D':'SKL WKS GT2',
    '1923':'SKL ULT GT3',
    '1926':'SKL ULT GT3',
    '1927':'SKL ULT GT3',
    '192B':'SKL Halo GT3',
    '192D':'SKL SRV GT3',
    '1932':'SKL DT GT4',
    '193B':'SKL Halo GT4',
    '193D':'SKL WKS GT4',
    '192A':'SKL SRV GT4',
    '193A':'SKL SRV GT4e',
    '5A84':'APL HD Graphics 505',
    '5A85':'APL HD Graphics 500',
    '5913':'KBL ULT GT1.5',
    '5915':'KBL ULX GT1.5',
    '5917':'KBL DT GT1.5',
    '5906':'KBL ULT GT1',
    '590E':'KBL ULX GT1',
    '5902':'KBL DT GT1',
    '5908':'KBL Halo GT1',
    '590B':'KBL Halo GT1',
    '590A':'KBL SRV GT1',
    '5916':'KBL ULT GT2',
    '5921':'KBL ULT GT2F',
    '591E':'KBL ULX GT2',
    '5912':'KBL DT GT2',
    '591B':'KBL Halo GT2',
    '591A':'KBL SRV GT2',
    '591D':'KBL WKS GT2',
    '5923':'KBL ULT GT3',
    '5926':'KBL ULT GT3',
    '5927':'KBL ULT GT3',
    '593A':'KBL SERV GT4',
    '593B':'KBL Halo GT4',
    '593D':'KBL WKS GT4',
    '592A':'KBL SERV GT3',
    '592B':'KBL HALO GT3',
    '5932':'KBL DT GT4',
    '3E90':'CFL GT1',
    '3E91':'CFL GT2',
    '3E92':'CFL GT2',
    '3E93':'CFL GT1',
    '3E94':'CFL GT2',
    '3E96':'CFL GT2',
    '3E98':'CFL GT2',
    '3E99':'CFL GT1',
    '3E9A':'CFL GT2',
    '3E9C':'CFL GT1',
    '3E9B':'CFL GT2',
    '3EA5':'CFL GT3',
    '3EA6':'CFL GT3',
    '3EA7':'CFL GT3',
    '3EA8':'CFL GT3',
    '3EA9':'CFL GT2',
    '3EA0':'CFL GT2',
    '3EA1':'CFL GT1',
    '3EA2':'CFL GT3',
    '3EA3':'CFL GT2',
    '3EA4':'CFL GT1',
    '9B21':'CFL GT1',
    '9BAA':'CFL GT1',
    '9BAB':'CFL GT1',
    '9BAC':'CFL GT1',
    '9BA0':'CFL GT1',
    '9BA5':'CFL GT1',
    '9BA8':'CFL GT1',
    '9BA4':'CFL GT1',
    '9BA2':'CFL GT1',
    '9B41':'CFL GT2',
    '9BCA':'CFL GT2',
    '9BCB':'CFL GT2',
    '9BCC':'CFL GT2',
    '9BC0':'CFL GT2',
    '9BC5':'CFL GT2',
    '9BC8':'CFL GT2',
    '9BC4':'CFL GT2',
    '9BC2':'CFL GT2',
    '5A51':'CNL GT2',
    '5A52':'CNL GT2',
    '5A5A':'CNL GT2',
    '5A42':'CNL GT2',
    '5A4A':'CNL GT2',
    '5A59':'CNL GT2',
    '5A41':'CNL GT2',
    '5A49':'CNL GT2',
    'FF05':'ICL GT1',
    '8A50':'ICL GT2',
    '8A51':'ICL GT2',
    '8A52':'ICL GT2',
    '8A53':'ICL GT2',
    '8A56':'ICL GT1',
    '8A57':'ICL GT1',
    '8A58':'ICL GT1',
    '8A59':'ICL GT1',
    '8A5A':'ICL GT1',
    '8A5B':'ICL GT1',
    '8A5C':'ICL GT1',
    '8A5D':'ICL GT1',
    '8A71':'ICL GT1',
    '4500':'EHL GT2',
    '4541':'EHL GT2',
    '4551':'EHL GT2',
    '4569':'EHL GT2',
    '4571':'EHL GT2',
    '9A40':'TGL GT2',
    '9A49':'TGL GT2',
    '9A59':'TGL GT2',
    '9A60':'TGL GT2',
    '9A68':'TGL GT2',
    '9A70':'TGL GT2',
    '9A78':'TGL GT2',
    }

    try:
        gpustr=id_direcotry[pciid]
        sts=loglevelcode.SUCCESS
    except:
        pass

    return (sts,gpustr)

def get_processor():
    processor_name="unknown"
    f=open("/proc/cpuinfo","r")
    for line in f:
        if line.find("model name")<0: continue
        line=line.strip()
        (var,val)=line.split(":")
        processor_name=val
        break

    return processor_name.strip()

def does_processor_have_gen_graphics():
    processor_name=get_processor()
    print_info("Processor name: "+processor_name,loglevelcode.SUCCESS)
    processor_name=processor_name.upper()

    if (processor_name.find("INTEL")>=0):
        print_info("Intel Processor",loglevelcode.SUCCESS)
    else:
        print_info("Not an Intel processor.  No Media GPU capabilities supported.",loglevelcode.ERROR)

    if (processor_name.find("CORE")>=0):
        print_info("Processor brand: Core",loglevelcode.INFO)

        pos=-1
        pos=processor_name.find("I7-")
        if (pos<0): pos=processor_name.find("I5-")
        if (pos<0): pos=processor_name.find("I3-")

        core_vnum=processor_name[pos+3:pos+7]
        try:
            procnum=int(core_vnum)
            archnum=procnum/1000
            if (archnum==4):
                print_info("Processor arch: Haswell",loglevelcode.INFO)
            elif (archnum==5):
                print_info("Processor arch: Broadwell",loglevelcode.INFO)
            elif (archnum==6):
                print_info("Processor arch: Skylake",loglevelcode.INFO)
        except:
            pass

    elif (processor_name.find("XEON")>=0):
        print_info("Processor brand: Xeon",loglevelcode.INFO)
        pos=processor_name.find(" V")
        if pos>0:
            xeon_vnum=processor_name[pos+1:pos+3]
            if ("V3" in xeon_vnum):
                print_info("Processor arch: Haswell",loglevelcode.INFO)
            elif ("V4" in xeon_vnum):
                print_info("Processor arch: Broadwell",loglevelcode.INFO)
            elif ("V5" in xeon_vnum):
                print_info("Processor arch: Skylake",loglevelcode.INFO)

    else:
        print_info("Processor not Xeon or Core.",loglevelcode.INFO)

    return loglevelcode.SUCCESS

def is_OS_media_ready():

    #check GPU PCIid
    lspci_output=run_cmd("lspci -nn -s 0:02.0")
    pos=lspci_output.rfind("[8086:")
    pciid=lspci_output[pos+6:pos+10].upper()
    (sts,gpustr)=parse_pciid(pciid)
    print_info("GPU PCI id     : " +pciid,loglevelcode.INFO)
    print_info("GPU description: "+gpustr,loglevelcode.INFO)

    #check for nomodeset
    grub_cmdline_output=run_cmd("cat /proc/cmdline")
    if (grub_cmdline_output.find("nomodeset")>0):
        print_info("nomodeset detected in GRUB cmdline",loglevelcode.ERROR)
        return loglevelcode.ERROR
    else:
        print_info("no nomodeset in GRUB cmdline (good)",loglevelcode.INFO)

    #Linux distro
    (linux_distro_name,linux_distro_version,linux_distro_details)=platform.linux_distribution()
    linux_distro=linux_distro_name+" "+linux_distro_version
    print_info("Linux distro   : "+linux_distro,loglevelcode.INFO)

    #kernel
    uname_output=run_cmd("uname -r")
    print_info("Linux kernel   : "+uname_output.strip(),loglevelcode.INFO)

    #glibc version
    ldd_version_output=run_cmd("ldd --version")
    pos=ldd_version_output.find("Copyright")
    ldd_version_output=ldd_version_output[0:pos-1]
    tmp=ldd_version_output.split()

    try:
        ldd_version=float(tmp.pop())
    except:
        ldd_version=0

    if (ldd_version>=2.12):
        outstr="glibc version  : %4.2f"%(ldd_version)
        print_info(outstr,loglevelcode.INFO)
    else:
        outstr="glibc version  : %4.2f >= 2.12 required!"%(ldd_version)
        print_info(outstr,loglevelcode.ERROR)
        return loglevelcode.ERROR

    #gcc version
    gcc_version_output=run_cmd("gcc --version")
    if ("not found" in gcc_version_output):
        print_info("gcc not found",loglevelcode.ERROR)
        sys.exit(1)
    else:
        pos=gcc_version_output.find("Copyright")
        gcc_version_output=gcc_version_output[0:pos-1]
        tmp=gcc_version_output.split()
        gcc_version=tmp.pop()
        print_info("gcc version    : "+gcc_version + " (>=6 suggested)",loglevelcode.INFO)

    return loglevelcode.SUCCESS

def check_kmd_caps():
    codestr1="""#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DRM_DRIVER_NAME_LEN 4

struct drm_version_t {
        int version_major;        /**< Major version */
        int version_minor;        /**< Minor version */
        int version_patchlevel;   /**< Patch level */
        size_t name_len;          /**< Length of name buffer */
        char      *name;          /**< Name of driver */
        size_t date_len;          /**< Length of date buffer */
        char      *date;          /**< User-space buffer to hold date */
        size_t desc_len;          /**< Length of desc buffer */
        char      *desc;          /**< User-space buffer to hold desc */
};

#define DRM_IOCTL_BASE          'd'
#define DRM_IO(nr)          _IO(DRM_IOCTL_BASE,nr)
#define DRM_IOWR(nr,type)       _IOWR(DRM_IOCTL_BASE,nr,type)

typedef struct drm_i915_getparam {
    int param;
    int *value;
} drm_i915_getparam_t;

int main(int argc,char **argv)
{
  char adapterpath[256];
  snprintf(adapterpath,sizeof(adapterpath),"%s",argv[1]);

  int fd = open(adapterpath, O_RDWR);
  if (fd < 0) {
    return -1;
  }

  char name[DRM_DRIVER_NAME_LEN + 1] = "none";
  drm_version_t version = {};
  version.name_len = DRM_DRIVER_NAME_LEN + 1;
  version.name = name;

  int res = ioctl(fd, DRM_IOWR(0, struct drm_version_t), &version);
  if(strcmp(name, "i915") == 0) {
    return 0;
  }

  return -1;
}
"""

    codestr2="""#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <i915_drm.h>
#include <xf86drm.h>
#include <stdio.h>
static bool has_param(int fd, int param)
{
  drm_i915_getparam_t gp;
  int tmp = 0;

  memset(&gp, 0, sizeof(gp));
  gp.value = &tmp;
  gp.param = param;

  if (drmIoctl(fd, DRM_IOCTL_I915_GETPARAM, &gp))
  {
      return false;
  }

  return tmp > 0;
}

static int get_value(int fd, int param)
{
  drm_i915_getparam_t gp;
  int tmp = 0;

  memset(&gp, 0, sizeof(gp));
  gp.value = &tmp;
  gp.param = param;

  if (drmIoctl(fd, DRM_IOCTL_I915_GETPARAM, &gp))
  {
      return false;
  }

  return tmp;
}

int main(int argc,char **argv)
{
  int fd = -1;

  //search for valid graphics device
  char adapterpath[256];
  snprintf(adapterpath,sizeof(adapterpath),"%s",argv[1]);

  fd = open(adapterpath, O_RDWR);
  if (fd < 0) {
    return -1;
  }

  printf("HUC_STATUS=%d\\n", get_value(fd, I915_PARAM_HUC_STATUS));
}
"""

    if not pkgconfig.exists('libdrm'):
        print_info("no libdrm pkg-config file. Are Intel components installed?",loglevelcode.ERROR)
        return

    if not os.path.exists("/usr/include/xf86drm.h"):
        print_info("no libdrm include files. Are Intel components installed?",loglevelcode.ERROR)
        return

    f=open("device_path_tmp.c","w")
    f.write(codestr1)
    f.close()

    f=open("kmdcheck_tmp.c","w")
    f.write(codestr2)
    f.close()

    cmd='g++ -o device_path device_path_tmp.c {0} {1} '.format(pkgconfig.cflags('libdrm'), pkgconfig.libs('libdrm'))
    sts=os.system(cmd)
    if not sts==0:
        print_info("Fatal error",loglevelcode.ERROR)
        return;

    cmd='g++ -o kmdcheck kmdcheck_tmp.c {0} {1} '.format(pkgconfig.cflags('libdrm'), pkgconfig.libs('libdrm'))
    sts=os.system(cmd)
    if not sts==0:
        print_info("Fatal error",loglevelcode.ERROR)
        return;

    found=False;
    if os.path.exists("/dev/dri/renderD128"):
        f=os.popen("ls -1 /dev/dri/renderD*")
        for line in f:
            interface_str=line.strip()
            sts=os.system("./device_path %s"%(interface_str))
            if sts==0:
                found=True
                print_info(interface_str+" connects to Intel i915",loglevelcode.SUCCESS)
                out=run_cmd("./kmdcheck %s"%(interface_str))
                pos=out.find("HUC_STATUS=")
                huc=out[pos+11]
                try:
                    huc_value=float(huc.pop())
                except:
                    huc_value=0
                if huc_value==1:
                  print_info("HuC enabled",loglevelcode.SUCCESS)
                else:
                  print_info("HuC not enabled. Some HuC dependent features might be missing, e.g. BRC in VDEnc encoders",loglevelcode.WARNING)
    else:
        print_info("no /dev/dri/renderD* interfaces found",loglevelcode.ERROR)

    if found==False:
      print_info("no /dev/dri/renderD* interfaces belong to Intel adapter",loglevelcode.ERROR)

    os.remove("device_path_tmp.c")
    os.remove("device_path")
    os.remove("kmdcheck_tmp.c")
    os.remove("kmdcheck")

def check_msdk_api():
    codestr="""
#include <mfxvideo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv)
{
  mfxVersion ver = { {0, 1} };
  mfxSession session;
  if (argc<2) return 0;
  if (strncmp(argv[1],"HW",2)==0) {
     MFXInit(MFX_IMPL_HARDWARE_ANY,&ver,&session);
  } else {
      MFXInit(MFX_IMPL_SOFTWARE,&ver,&session);
  }
  MFXQueryVersion(session,&ver);
  printf("%d.%d\\n",ver.Major,ver.Minor);
  return 0;
}
"""

    if pkgconfig.exists('libmfx'):
        print_info("libmfx package found",loglevelcode.SUCCESS)
    else:
        print_info("libmfx package not found in default PKG_CONFIG locations",loglevelcode.WARNING)
        os.environ["PKG_CONFIG_PATH"] = "/opt/intel/mediasdk/lib64/pkgconfig/"
        print_info("Trying again with PKG_CONFIG_PATH=/opt/intel/mediasdk/lib64/pkgconfig/",loglevelcode.INFO)
        if pkgconfig.exists('libmfx'):
            print_info("libmfx package now found",loglevelcode.SUCCESS)
        else:
            print_info("libmfx package still not found",loglevelcode.ERROR)
            return


    if not pkgconfig.exists("libmfx"):
        print_info("no Media SDK pkg-config file. Are Intel components installed?",loglevelcode.ERROR)
        return

    if not os.path.exists(pkgconfig.parse('libmfx')['include_dirs'][0] +"/mfx/mfxvideo.h"):
        print_info("no Media SDK headers. Are Intel components installed?",loglevelcode.ERROR)
        return

    if not pkgconfig.exists("libva"):
        print_info("no libva pkg-config file. Are Intel components installed?",loglevelcode.ERROR)
        return

    f=open("msdkcheck_tmp.c","w")
    f.write(codestr)
    f.close()

    cmd='g++ -o msdkcheck msdkcheck_tmp.c {0} {1} {2} -ldl'.format(pkgconfig.cflags('libmfx'), pkgconfig.libs('libmfx'), pkgconfig.libs('libva'))
    sts=os.system(cmd)
    if (sts>0):
        print_info("could not compile Media SDK test",loglevelcode.ERROR)

    out=run_cmd("./msdkcheck HW")
    print_info("Media SDK HW API level:"+out.strip(),loglevelcode.SUCCESS)

    #out=run_cmd("./msdkcheck SW")
    #print_info("Media SDK SW API level:"+out.strip(),loglevelcode.SUCCESS)

    os.remove("msdkcheck")
    os.remove("msdkcheck_tmp.c")

if __name__ == "__main__":

    if len(sys.argv)>1:
        if (sys.argv[1]=="--help"):
            print( "usage: python3 sys_analyzer_linux.py")
            sys.exit(0)

    #HW media ready: processor,gpu ID (yes,no,advice)
    print("--------------------------")
    print("Hardware readiness checks:")
    print("--------------------------")
    sts=does_processor_have_gen_graphics()
    if (sts<loglevelcode.SUCCESS): sys.exit(1)

    #OS media ready: OS,glibc version,gcc version,nomodeset,gpuID (yes, no, advice)
    print("--------------------------")
    print("OS readiness checks:")
    print("--------------------------")
    sts=is_OS_media_ready()
    if (sts<loglevelcode.SUCCESS): sys.exit(1)

    #Media stack install correctness: vainfo, /dev/dri, check MSDK dirs, check OCL dirs, check MSDK/OCL funcs
    print("--------------------------")
    print("User and kernel mode drivers health checks:")
    print("--------------------------")
    #in video group
    out=run_cmd("groups")
    if ("video" in out):
        print_info("user in 'video' group",loglevelcode.SUCCESS)
    else:
        print_info("user not in the 'video' group. Add with 'usermod -a -G video {user}'",loglevelcode.WARNING)

    #check i915 use
    out=run_cmd("lspci -v -s 0:02.0")
    if ("Kernel driver in use: i915" in out):
        print_info("i915 driver in use by Intel video adapter",loglevelcode.SUCCESS)
    else:
        print_info("Intel video adapter not using i915",loglevelcode.ERROR)

    check_kmd_caps()

    if pkgconfig.exists('libva'):
        print_info("libva pkg-config found",loglevelcode.SUCCESS)
    else:
        print_info("libva package not found in default PKG_CONFIG locations. Trying again with PKG_CONFIG_PATH=/opt/intel/mediasdk/lib64/pkgconfig/",loglevelcode.INFO)
        os.environ["PKG_CONFIG_PATH"] = "/opt/intel/mediasdk/lib64/pkgconfig/"
        print_info("Trying again with PKG_CONFIG_PATH=/opt/intel/mediasdk/lib64/pkgconfig/",loglevelcode.INFO)
        if pkgconfig.exists('libva'):
            print_info("libva found with PKG_CONFIG_PATH=/opt/intel/mediasdk/lib64/pkgconfig/",loglevelcode.SUCCESS)
        else:
            print_info("libva not found with PKG_CONFIG_PATH=/opt/intel/mediasdk/lib64/pkgconfig/",loglevelcode.ERROR)

    #vainfo: exists,correct iHD used
    try:
      os.environ["LIBVA_DRIVER_NAME"]
    except KeyError:
      print_info("No LIBVA_DRIVER_NAME environment variable. Consider setting 'export LIBVA_DRIVER_NAME=iHD",loglevelcode.WARNING)

    try:
      os.environ["LIBVA_DRIVERS_PATH"]
    except KeyError:
      print_info("No LIBVA_DRIVERS_PATH environment variable. Consider setting 'export LIBVA_DRIVER_NAME=/path/to/iHD_drv_video.so",loglevelcode.WARNING)

    out=run_cmd("vainfo 2>&1")
    if ("iHD_drv_video.so" in out):
        print_info("Intel iHD used by libva",loglevelcode.SUCCESS)
    else:
        print_info("libva not loading Intel iHD",loglevelcode.ERROR)

    if ("VAEntrypoint" in out):
        print_info("vainfo reports valid codec entry points",loglevelcode.SUCCESS)
    else:
        print_info("vainfo not reporting codec entry points",loglevelcode.ERROR)

    print("--------------------------")
    print("Component Smoke Tests:")
    print("--------------------------")
    check_msdk_api()
