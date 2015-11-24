# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
import paramiko
import os
import sys
import time
import inspect
import logging
import shutil


dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))

import settings
from cmd import exec_cmd2
import ssh

logging.config.dictConfig(settings.LOGGING)
log = logging.getLogger('main')

ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
ch.setFormatter(formatter)
log.addHandler(ch)

VS_PATH = "C:\\Program Files (x86)\\Microsoft Visual Studio 14.0"
WDK_PATH = "C:\\Program Files (x86)\\Windows Kits\\10"
SDK_PATH = "C:\\Program Files (x86)\\Windows Kits\\10"
WDK_BIN = os.path.join(WDK_PATH, "bin\\x64")
SIGN_TOOL = os.path.join(WDK_BIN, "signtool.exe")
CERT_STORE_NAME = "PrivateCertStore"
CERT_NAME = "Contoso.com(Test)"

VS_BIN_PATH = os.path.join(VS_PATH, "VC\\bin")
VS_LIB_PATH = os.path.join(VS_PATH, "VC\\lib\\amd64")

CL_EXE = os.path.join(VS_BIN_PATH, "x86_amd64\\CL.exe")
ML_EXE = os.path.join(VS_BIN_PATH, "x86_amd64\\ml64.exe")
LIB_EXE = os.path.join(VS_BIN_PATH, "Lib.exe")
LINK_EXE = os.path.join(VS_BIN_PATH, "x86_amd64\\link.exe")

PROJ_ROOT = os.path.dirname(dir)

BUILD = os.path.join(PROJ_ROOT, "Build")
BUILD_TMP = os.path.join(PROJ_ROOT, "Build_tmp")

WDK_INC=os.path.join(WDK_PATH, "Include\\10.0.10240.0")
WDK_INC_KM=os.path.join(WDK_INC, "km")
WDK_INC_KM_CRT=os.path.join(WDK_INC, "km\\crt")
WDK_INC_SHARED=os.path.join(WDK_INC, "shared")
WDK_INC_UM=os.path.join(WDK_INC, "um")

WDK_BIN=os.path.join(WDK_PATH, "bin\\x64")

WDK_LIB=os.path.join(WDK_PATH, "Lib\\10.0.10240.0")
WDK_KM_LIB=os.path.join(WDK_LIB, "km\\x64")
WDK_UM_LIB=os.path.join(WDK_LIB, "um\\x64")

SDK_LIB = os.path.join(SDK_PATH, "Lib\\10.0.10240.0")
SDK_UCRT_LIB = os.path.join(SDK_LIB,  "ucrt\\x64")
SDK_UM_LIB = os.path.join(SDK_LIB, "um\\x64")

KCL_VER_D_OPTS="/D _WIN32_WINNT=0x0601 /D WINVER=0x0601 /D WINNT=1 /D NTDDI_VERSION=0x06010000"
KCL_ARCH_D_OPTS="/D _WIN64 /D _AMD64_ /D AMD64"
KCL_OPTS="/Zi /nologo /W3 /WX /Od /GF /Gm- /Zp8 /GS /Gy /fp:precise /Zc:wchar_t /Zc:forScope /GR- /Gz /TC /wd4603 /wd4627 /wd4986 /wd4987 /wd4996 /errorReport:prompt /kernel -cbstring -d2epilogunwind  /d1nodatetime /d1import_no_registry /d2AllowCompatibleILVersions /d2Zi+"

KLIB_ARCH_OPTS="/MACHINE:X64"
KLIB_OPTS="/NOLOGO"
KLIBS=["BufferOverflowK.lib", "ntoskrnl.lib", "hal.lib", "wmilib.lib", "netio.lib"]

KLINK_VER_OPTS="/VERSION:\"6.3\" /SUBSYSTEM:NATIVE,\"6.01\""
KLINK_ARCH_OPTS="/MACHINE:X64"
KLINK_OPTS="/ERRORREPORT:PROMPT /INCREMENTAL:NO /NOLOGO /WX /SECTION:\"INIT,d\" /NODEFAULTLIB /MANIFEST:NO /DEBUG /MERGE:\"_TEXT=.text;_PAGE=PAGE\" /PROFILE /kernel /IGNORE:4198,4010,4037,4039,4065,4070,4078,4087,4089,4221,4108,4088,4218,4218,4235 /osversion:6.3 /pdbcompress /debugtype:pdata /Driver /OPT:REF /OPT:ICF /ENTRY:\"GsDriverEntry\" /RELEASE"

UCL_D_OPTS="/D _WIN32_WINNT=0x0601 /D WINVER=0x0601 /D WINNT=1 /D NTDDI_VERSION=0x06010000 /D _WIN64 /D _AMD64_ /D AMD64 /D NDEBUG /D _CONSOLE /D _LIB /D _UNICODE /D UNICODE"
UCL_OPTS="/Zi /nologo /W3 /WX /Od /GL /Gm- /EHsc /MT /GS /Gy /fp:precise /Zc:wchar_t /Zc:forScope /Gd /TC /errorReport:prompt"

ULINK_ARCH_OPTS="/MACHINE:X64"
ULINK_OPTS="/ERRORREPORT:PROMPT /INCREMENTAL:NO /NOLOGO /MANIFEST /MANIFESTUAC:\"level='asInvoker' uiAccess='false'\" /manifest:embed /DEBUG /SUBSYSTEM:CONSOLE /OPT:REF /OPT:ICF /LTCG /TLBID:1 /DYNAMICBASE /NXCOMPAT"

UVS_LIBS=["libcmt.lib", "oldnames.lib", "libvcruntime.lib", "legacy_stdio_definitions.lib", "legacy_stdio_wide_specifiers.lib"]
UCRT_LIBS = ["libucrt.lib"]

UWDK_LIBS=["BufferOverflowU.lib", "kernel32.lib", "user32.lib", "gdi32.lib", "winspool.lib", "comdlg32.lib",
            "advapi32.lib", "shell32.lib", "ole32.lib", "oleaut32.lib", "uuid.lib", "odbc32.lib", "odbccp32.lib"]

VS_BUILD_BINS= ["mspdb140.dll", "mspdbcore.dll", "mspdbsrv.exe", "msobj140.dll", "cvtres.exe"]
WDK_BUILD_BINS= ["rc.exe", "rcdll.dll"]


BUILD_OUT_DIR=os.path.join(BUILD, "x64")

DRIVER_NAME="FBackup"
DRIVER_BUILD_TMP=os.path.join(BUILD_TMP, DRIVER_NAME + "\\x64")
DRIVER_DIR=os.path.join(PROJ_ROOT, "Driver")
DRIVER_OUT=DRIVER_NAME + ".sys"
DRIVER_OUT_PDB=DRIVER_NAME + ".pdb"

CLIENT_NAME="FBackupCtl"
CLIENT_BUILD_TMP=os.path.join(BUILD_TMP, CLIENT_NAME + "\\x64")
CLIENT_DIR=os.path.join(PROJ_ROOT, "Client")
CLIENT_OUT=CLIENT_NAME + ".exe"
CLIENT_OUT_PDB=CLIENT_NAME + ".pdb"

DRIVER_SOURCES = ["driver.c", "klog.c"]

CLIENT_SOURCES = ["client.c", "scmload.c", "main.c"]

def do_cmd(cmd, elog):
    exec_cmd2(cmd, throw = True, elog = elog)

def do_copy_file(src, dst, elog):
    elog.info("copying %s -> %s" % (src, dst))
    shutil.copy(src, dst)

def quote_path(path):
    return "\"" + path + "\""

def inc_path(path):
    return " /I" + quote_path(path)

def replace_ext(path, ext):
    return os.path.splitext(path)[0] + ext

def build_driver(elog):
    cmd = quote_path(CL_EXE) + " /c"
    cmd+= inc_path(WDK_INC_KM)
    cmd+= inc_path(WDK_INC_KM_CRT)
    cmd+= inc_path(WDK_INC_SHARED)
    cmd+= inc_path(PROJ_ROOT)
    cmd+=" /Fo" + quote_path(DRIVER_BUILD_TMP) + "\\"
#    cmd+= " /Fd" + quote_path(os.path.join(DRIVER_BUILD_TMP, "vc120.pdb"))
    cmd+=" " + KCL_OPTS + " " + KCL_VER_D_OPTS + " " + KCL_ARCH_D_OPTS
    for f in DRIVER_SOURCES:
        cmd+= " " + quote_path(os.path.join(DRIVER_DIR, f))
    do_cmd(cmd, elog)

    cmd = quote_path(LINK_EXE)
    cmd+= " /OUT:" + quote_path(os.path.join(BUILD_OUT_DIR, DRIVER_OUT)) + " " + KLINK_OPTS + " " + KLINK_ARCH_OPTS + " " + KLINK_VER_OPTS
    cmd+= " /PDB:" + quote_path(os.path.join(BUILD_OUT_DIR, DRIVER_OUT_PDB))
    for l in KLIBS:
        cmd+= " " + quote_path(os.path.join(WDK_KM_LIB, l))

    for f in DRIVER_SOURCES:
        fobj = replace_ext(f, ".obj")
        cmd+= " " + quote_path(os.path.join(DRIVER_BUILD_TMP, fobj))
    do_cmd(cmd, elog)

    cmd = quote_path(SIGN_TOOL) + " sign /v /s " + CERT_STORE_NAME + " /n " + CERT_NAME
    cmd+= " /t http://timestamp.verisign.com/scripts/timstamp.dll "
    cmd+= quote_path(os.path.join(BUILD_OUT_DIR, DRIVER_OUT))
    do_cmd(cmd, elog)

def build_client(elog):
    cmd = quote_path(CL_EXE) + " /c"
    cmd+= inc_path(WDK_INC_UM)
    cmd+= inc_path(WDK_INC_KM_CRT)
    cmd+= inc_path(WDK_INC_SHARED)
    cmd+= inc_path(PROJ_ROOT)
    cmd+=" /Fo" + quote_path(CLIENT_BUILD_TMP) + "\\"
#    cmd+= " /Fd" + quote_path(os.path.join(CLIENT_BUILD_TMP, "vc120.pdb"))
    cmd+=" " + UCL_OPTS + " " + UCL_D_OPTS
    for f in CLIENT_SOURCES:
        cmd+= " " + quote_path(os.path.join(CLIENT_DIR, f))
    do_cmd(cmd, elog)

    cmd = quote_path(LINK_EXE)
    cmd+= " /OUT:" + quote_path(os.path.join(BUILD_OUT_DIR, CLIENT_OUT)) + " " + ULINK_OPTS + " " + ULINK_ARCH_OPTS
    cmd+= " /PDB:" + quote_path(os.path.join(BUILD_OUT_DIR, CLIENT_OUT_PDB))
    for l in UWDK_LIBS:
        cmd+= " " + quote_path(os.path.join(WDK_UM_LIB, l))
    for l in UVS_LIBS:
        cmd+= " " + quote_path(os.path.join(VS_LIB_PATH, l))
    for l in UCRT_LIBS:
        cmd+= " " + quote_path(os.path.join(SDK_UCRT_LIB, l))
    for f in CLIENT_SOURCES:
        fobj = replace_ext(f, ".obj")
        cmd+= " " + quote_path(os.path.join(CLIENT_BUILD_TMP, fobj))
    do_cmd(cmd, elog)

def rebuild(elog):
    os.system("taskkill /F /im mspdbsrv.exe")
    rc = 1
    if os.path.exists(BUILD):
        do_cmd("rmdir /s /q " + BUILD, elog)
    if os.path.exists(BUILD_TMP):
        do_cmd("rmdir /s /q " + BUILD_TMP, elog)

    do_cmd("mkdir " + BUILD, elog)
    do_cmd("mkdir " + BUILD_TMP, elog)
    odir = os.getcwd()
    os.chdir(BUILD_TMP)
    try:
        for f in VS_BUILD_BINS:
            do_copy_file(os.path.join(VS_BIN_PATH, f), os.path.join(BUILD_TMP, f), elog)

        for f in WDK_BUILD_BINS:
            do_copy_file(os.path.join(WDK_BIN, f), os.path.join(BUILD_TMP, f), elog)

        do_cmd("mkdir " + BUILD_OUT_DIR, elog)
        do_cmd("mkdir " + DRIVER_BUILD_TMP, elog)
        do_cmd("mkdir " + CLIENT_BUILD_TMP, elog)

        build_driver(elog)
        build_client(elog)
        rc = 0
    except Exception as e:
        elog.exception(str(e))
    finally:
        os.chdir(odir)
    elog.info("build completed with rc %d" % (rc,))
    return rc

if __name__ == '__main__':
    rebuild(log)
