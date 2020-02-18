/* ****************************************************************************** *\

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012-2020 Intel Corporation. All Rights Reserved.

File Name: Program.cs

\* ****************************************************************************** */

using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using System.Diagnostics;
using System.ComponentModel;
using System.Threading;
using System.Security.AccessControl;

using Microsoft.Win32;
using System.Security.Principal;
using System.Runtime.InteropServices;
using System.Diagnostics.Eventing;

namespace msdk_analyzer
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            bool bOwn;
            Mutex globalLock = new Mutex(true, "mediasdk_tracer_lock", out bOwn);
            if (!bOwn)
            {
                globalLock.Dispose();
                return;
            }

            try
            {
                string allapplicationpackages_group = "ALL APPLICATION PACKAGES";
                using (RegistryKey rk = Registry.CurrentUser.CreateSubKey(msdk_analyzer.Properties.Resources.msdk_registry_key, RegistryKeyPermissionCheck.ReadWriteSubTree))
                {
                    RegistrySecurity rs = new RegistrySecurity();
                    rs.AddAccessRule(new RegistryAccessRule(allapplicationpackages_group, RegistryRights.FullControl, InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit, PropagationFlags.InheritOnly, AccessControlType.Allow));
                    rk.SetAccessControl(rs);

                    DirectoryInfo myDirectoryInfo = new DirectoryInfo(Directory.GetCurrentDirectory());
                    DirectorySecurity myDirectorySecurity = myDirectoryInfo.GetAccessControl();
                    myDirectorySecurity.AddAccessRule(new FileSystemAccessRule(allapplicationpackages_group, FileSystemRights.FullControl, InheritanceFlags.ObjectInherit, PropagationFlags.InheritOnly, AccessControlType.Allow));
                    myDirectoryInfo.SetAccessControl(myDirectorySecurity);
                }
            }
            catch
            {
                globalLock.Dispose();
            }
            try
            {
                //////
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);

                UInt32 sts = MsdkAnalyzerCpp.install(Path.GetDirectoryName(Application.ExecutablePath)
                                        , Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + _PATH.TRACER_PATH + _PATH.TRACER_LOG
                                        , Path.GetDirectoryName(Application.ExecutablePath));
                if (sts == 0)
                {
                    MessageBox.Show("ERROR: Install error", "ERROR");

                }
                else
                {
                    SdkAnalyzerForm form = new SdkAnalyzerForm();
                    Application.Run(form);
                }
                MsdkAnalyzerCpp.uninstall();
                GC.KeepAlive(globalLock);//prevent releasing by GC if application uses long time
            }
            catch
            {
                MessageBox.Show("ERROR: Install error", "ERROR");
                globalLock.Dispose();
            }
            finally
            {
                globalLock.Dispose();
            }
        }
    }
}
